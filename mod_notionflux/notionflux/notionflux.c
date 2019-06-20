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

#define _POSIX_C_SOURCE 200809L /* strndup */
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

FILE *request(char const buf[MAX_DATA], char *type)
{
	FILE *sock = NULL;

	sock = sock_connect();

	fputs(buf, sock);
	fputc('\0', sock);
	fflush(sock);

	fread(type, 1, 1, sock);

	if (feof(sock) || ferror(sock))
		die("Short response from mod_notionflux.\n");

	if (*type != 'S' && *type != 'E')
		die("Unknown response type (%x)\n", (int)*type);

	return sock;
}

bool response(char buf[MAX_DATA], FILE **sock)
{
	if (!*sock)
		return false;

	size_t bytes = slurp(*sock, buf, MAX_DATA);
	if (bytes == MAX_DATA)
		return true;

	if (ferror(*sock))
		die("Error condition on mod_notionflux response.\n");

	// must be feof
	fclose(*sock);
	*sock = NULL;
	return true;
}

struct buf {
	size_t size;
	size_t pos;
	char buf[MAX_DATA];
};

bool buf_append(struct buf *buf, char const *str)
{
	size_t len = strlen(str);

	if (buf->pos + len >= buf->size)
		return false;

	memcpy(buf->buf + buf->pos, str, len + 1);
	buf->pos += len;
	return true;
}

void buf_grow(struct buf **buf, size_t n)
{
	*buf = realloc(*buf, (*buf)->size+n);
	if (!*buf)
		die("realloc failed\n");
}


char *completion_generator(const char *text, int state)
{
	static struct buf *buf = NULL;
	const char *req_str = "return table.concat("
		"mod_query.do_complete_lua(_G,'%s'),' ');";
	rl_completion_suppress_append = 1;

	if (state == 0) {
		if (buf) free(buf); /* huh */
		buf = malloc(sizeof(*buf));
		if (!buf) die("malloc failed\n");
		buf->pos = 0;
		buf->buf[0] = 0;
		buf->size = MAX_DATA;

		char temp[MAX_DATA];
		snprintf(temp, MAX_DATA, req_str, text);
		char type;
		FILE *remote = request(temp, &type);
		if (type == 'E') {
			fclose(remote);
			goto finish;
		}

		while (response(temp, &remote)) {
			if (!buf_append(buf, temp)) {
				buf_grow(&buf, MAX_DATA);
				buf_append(buf, temp);
			}
		}
		buf->pos = 0;
	}

	char const skip_chars[] = " \"\n\r";
	buf->pos += strspn(buf->buf+buf->pos, skip_chars);

	if (buf->buf[buf->pos] == '\0')
		goto finish;

	size_t next = strcspn(buf->buf+buf->pos, skip_chars);
	char *ret = strndup(buf->buf+buf->pos, next);
	buf->pos += next+1;
	return ret;
finish:
	free(buf);
	buf = NULL;
	return NULL;
}

int initialize_readline(void)
{
	rl_readline_name = "notionflux";
	rl_completion_entry_function = completion_generator;
	return 0;
}

struct buf input = {MAX_DATA, 0, {0}};

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

#define input_append(str) buf_append(&input, str)
#define input_reset() prompt="lua> ", input.pos=0, input.buf[0]=0

int repl_main(void)
{
	char *line = NULL;
	char const *prompt = "lua> ";
	rl_startup_hook = initialize_readline;

	while (1) {
		free(line);
		line = readline(prompt);

		if (!line) { /* EOF */
			puts("^D");
			if (!input.pos) {           /* quit */
				return 0;
			} else {    /* abort previous input */
				input_reset();
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
		FILE *remote = request(input.buf, &type);
		char out[MAX_DATA];
		while (response(out, &remote))
			fputs(out, stdout);
		input_reset();
	}
	/* unreachable */
}

int main(int argc, char **argv)
{
	sockfilename = get_socket_filename();
	char *data;
	char buf[MAX_DATA];

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

	FILE *remote;
	char type;
	remote = request(data, &type);

	FILE *dest = type == 'S' ? stdout : stderr;
	while (response(buf, &remote))
		fputs(buf, dest);

	return 0;
}
