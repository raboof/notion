/* Copyright (C) 2019 Moritz Wilhelmy

   This utility is part of the notion window manager, but uses a different
   license because GNU readline requires it.

   notionflux is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License (to be compatible with
   GNU readline) as well as Lesser General Public License (to be more compatible
   with notion itself) as published by the Free Software Foundation, either
   version 2 of the Licenses, or (at your option) any later version.

   By linking against readline, the created binary will be subject to the terms
   of GPL version 3.

   notionflux is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with notionflux.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lua.h>
#include <lauxlib.h>

#include "../notionflux.h"

#define die(...) fprintf(stderr, __VA_ARGS__), exit(1)

#define SOCK_ATOM "_NOTION_MOD_NOTIONFLUX_SOCKET"

char const *sockfilename = NULL;

char const *get_socket_filename(void)
{
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy)
		die("Failed to open display\n");

	Window root = DefaultRootWindow(dpy);
	Atom sockname_prop = XInternAtom(dpy, SOCK_ATOM, True);
	if (sockname_prop == None)
		die("No %s property set on the root window... is notion "
		    "running with mod_notionflux loaded?\n", SOCK_ATOM);

	Atom type;
	int d1; /* dummy */
	unsigned long d2, d3; /* dummy */
	unsigned char *ret;
	int pr = XGetWindowProperty(dpy, root, sockname_prop, 0L, 512L, False,
				    XA_STRING, &type, &d1, &d2, &d3, &ret);

	if (pr != Success || type != XA_STRING)
		 die("XGetWindowProperty failed\n");

	char *rv = strdup((char*)ret);

	XFree(ret);
	XCloseDisplay(dpy);
	return rv;
}

FILE *sock_connect(void)
{
	struct sockaddr_un serv;
	if (strlen(sockfilename) >= sizeof(serv.sun_path))
		die("Socket file name too long\n");
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);

	serv.sun_family = AF_UNIX;
	strcpy(serv.sun_path, sockfilename);
	if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) == -1)
		die("Failed to connect to socket %s\n", sockfilename);

	FILE *f = fdopen(sock, "w+");
	if (!f)
		die("fdopen failed\n");

	return f;
}

/* read from f until EOF */
size_t slurp(FILE *f, char *buf, size_t n)
{
	size_t bytes = fread(buf, 1, n-1, f);
	buf[bytes]='\0';
	return bytes+1;
}

/* fputs, but with trailing NUL character - expected by mod_notionflux */
void barf(FILE *f, char *str)
{
	fputs(str, f);
	fputc('\0', f);
	fflush(f);
}

bool request(char buf[MAX_DATA], char *type) /* not reentrant */
{
	static FILE *sock = NULL;

	if (*type && !sock) { /* last call after socket was closed */
		*type = '\0';
		return false;
	}

	if (!*type) { /* first call, send request */
		sock = sock_connect();
		barf(sock, buf);

		fread(type, 1, 1, sock);
		if (feof(sock) || ferror(sock))
			die("Short response from mod_notionflux.\n");

		if (*type != 'S' && *type != 'E')
			die("Unknown response type (%x)\n", (int)*type);
	}

	size_t bytes = slurp(sock, buf, MAX_DATA);
	if (bytes == MAX_DATA)
		return true;

	if (ferror(sock))
		die("Error condition on mod_notionflux response.\n");

	// must be feof
	fclose(sock);
	sock = NULL;
	return true;
}

int initialize_readline(void)
{
	rl_readline_name = "notionflux";
	return 0;
}

struct {
	size_t pos;
	char buf[MAX_DATA];
} input = {0};

#if 1 /* Taken from lua/lua.c, because they don't export it */

/* mark in error messages for incomplete statements */
#define EOFMARK		"<eof>"
#define marklen		(sizeof(EOFMARK)/sizeof(char) - 1)

bool is_lua_incomplete(char *lua, size_t len)
{
	static lua_State *L = NULL;
	if (!L) L=luaL_newstate();

	lua_settop(L, 0); /* clear stack */
	int status = luaL_loadbuffer(L, lua, len, "");

	if (status == LUA_ERRSYNTAX) {
		size_t lmsg;
		const char *msg = lua_tolstring(L, -1, &lmsg);
		if (lmsg >= marklen && strcmp(msg+lmsg-marklen, EOFMARK) == 0) {
			lua_pop(L, 1);
			return true;
		}
	}
	return false;
}
#endif

bool input_append(char const *str)
{
	size_t len = strlen(str);

	if (input.pos+len >= sizeof(input.buf))
		return false;

	memcpy(input.buf+input.pos, str, len+1);
	input.pos += len;
	return true;
}

#define input_reset() prompt="lua> ", input.pos=0, input.buf[0]=0

int repl_main(void)
{
	char *line;
	char const *prompt = "lua> ";
	rl_startup_hook = initialize_readline;

	while (1) {
		line = readline(prompt);

		if (!line) { /* EOF */
			if (!input.pos) {           /* quit */
				break;
			} else {    /* abort previous input */
				input_reset();
				puts("");
				continue;
			}
		}

		if (line[0] == '\0') /* empty line, skip */
			continue;

		if (!input_append(line) || !input_append("\n")) {
			printf("Maximum length (%db) exceeded.\n", MAX_DATA);
			input_reset();
			continue;
		}

		if (is_lua_incomplete(input.buf, input.pos)) {
			prompt = "...> ";
			continue;
		}

		char type = 0;
		while (request(input.buf, &type)) {
			fputs(input.buf, stdout);
		}
		input_reset();
	}

	return 0;
}

int main(int argc, char **argv)
{
	sockfilename = get_socket_filename();
	FILE *remote = sock_connect();
	char buf[MAX_DATA + 1];
	char *data;

	if (argc == 1 && isatty(STDIN_FILENO)) {
		return repl_main();
	} else if (argc == 1 || (argc == 2 && strcmp(argv[1], "-R") == 0)) {
		data = buf;
		slurp(stdin, buf, MAX_DATA);
	} else if (argc == 3 && strcmp(argv[1], "-e") == 0) {
		data = argv[2];
		if (strlen(data) > MAX_DATA)
			die("Too much data\n");
	} else {
		die("Usage: %s [-R|-e code]\n", argv[0]);
	}

	barf(remote, data);

	size_t bytes = slurp(remote, buf, MAX_DATA+1);
	if (!bytes)
		die("fread failed\n");

	char type = buf[0];
	if (type != 'S' && type != 'E')
		die("Unknown response type '%c'(%x)\n", type, type);

	fputs(buf+1, type == 'S' ? stdout : stderr);
	do {
		bytes = slurp(remote, buf, MAX_DATA);
		fputs(buf, type == 'S' ? stdout : stderr);
	} while (bytes == MAX_DATA);

	return 0;
}
