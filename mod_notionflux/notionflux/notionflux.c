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
	char *cur;
	char buf[MAX_DATA];
};

size_t buf_pos(struct buf *buf)
{
	return buf->cur - buf->buf;
}

bool buf_append(struct buf *buf, char const *str)
{
	size_t len = strlen(str);

	if (buf_pos(buf) + len >= buf->size)
		return false;

	memcpy(buf->cur, str, len + 1);
	buf->cur += len;
	return true;
}

void buf_grow(struct buf **buf, size_t n)
{
	size_t bufsize = (*buf)->size + n;
	size_t structsize = bufsize + (sizeof(struct buf) - sizeof((*buf)->buf));
	size_t pos = buf_pos(*buf);
	*buf = realloc(*buf, structsize);
	if (!*buf)
		die("realloc failed\n");
	(*buf)->size = bufsize;
	(*buf)->cur = (*buf)->buf + pos;
}

void *alloc(size_t bytes)
{
	void *buf = malloc(bytes);
	if (!buf)
		die("malloc failed\n");
	return buf;
}

struct buf *buf_make(void)
{
	struct buf *buf = alloc(sizeof(*buf));
	buf->cur = buf->buf;
	buf->buf[0] = 0;
	buf->size = MAX_DATA;
	return buf;
}

char *completion_generator(const char *text, int state)
{
	static struct buf *buf = NULL;
	const char *req_str = "return table.concat("
		"mod_query.do_complete_lua(_G,'%s'),' ');";
	rl_completion_suppress_append = 1;

	if (state == 0) {
		if (buf) free(buf); /* huh */
		buf = buf_make();

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
		buf->cur = buf->buf;
	}

	char const skip_chars[] = " \"\n\r";
	buf->cur += strspn(buf->cur, skip_chars);

	if (*buf->cur == '\0')
		goto finish;

	size_t length = strcspn(buf->cur, skip_chars);
	buf->cur[length] = '\0';

	char *ret;
	char *lastdot = strrchr(text, '.');
	if (!lastdot) {
		ret = strdup(buf->cur);
	} else {
		/* mod_query trims tables in completions, prepend back */
		size_t dotpos = lastdot - text + 1;
		ret = alloc(dotpos + length + 1);
		ret[0] = '\0';
		strncat(ret, text, dotpos);
		strcat(ret, buf->cur);
	}

	buf->cur += length+1;
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
	rl_basic_word_break_characters = " \t\n#;|&{(["
		"<>~="
		"+-*/%^";
	rl_basic_quote_characters = "\'\"";
	return 0;
}

struct buf input = {MAX_DATA, 0, {0}};

#if 1 /* Ideas taken from lua/lua.c, because the REPL isn't part of liblua */

/* mark in error messages for incomplete statements */
#define EOFMARK		"<eof>"
#define marklen		(sizeof(EOFMARK)/sizeof(char) - 1)

bool is_lua_incomplete(lua_State *L, char const *lua, size_t len)
{
	lua_settop(L, 0);
	int status = luaL_loadbuffer(L, lua, len, "=stdin");

	if (status == LUA_ERRSYNTAX) {
		size_t lmsg;
		const char *msg = lua_tolstring(L, -1, &lmsg);
		if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMARK) == 0) {
			lua_pop(L, 1);
			return true;
		}
	}
	return false;
}

char const *make_lua_exp(char *lua, size_t size)
{
	static lua_State *L = NULL;
	if (!L) L=luaL_newstate();

	/* Inspired by Lua 5.3:
	 * Attempt to add a return keyword and see if the snippet can be loaded
	 * without an error (which would mean that it is an expression). If not,
	 * see if it is complete. If it isn't complete either, wait for the user
	 * to finish the statement. This way, the user doesn't have to type
	 * return in front of everything and still gets to see the return value. */

	const char *retline = lua_pushfstring(L, "return %s;", lua);
	int status = luaL_loadbuffer(L, retline, strlen(retline), "=stdin");
	lua_settop(L, 0);

	if (status == LUA_OK)
		return retline; /* presumably managed by lua's GC */
	else if (!is_lua_incomplete(L, lua, size))
		return lua;
	return NULL;
}
#endif

#define input_append(str) buf_append(&input, str)
#define input_reset() prompt="lua> ", input.cur=input.buf, input.buf[0]=0

int repl_main(void)
{
	char *line = NULL;
	char const *prompt;
	rl_startup_hook = initialize_readline;
	input_reset();

	while (1) {
		free(line);
		line = readline(prompt);

		if (!line) { /* EOF */
			puts("^D");
			if (buf_pos(&input) == 0) {      /* user quit */
				return 0;
			} else {                /* abort previous input */
				input_reset();
				continue;
			}
		}

		if (line[0] == '\0') /* empty line, skip */
			continue;

		add_history(line);

		if (!input_append(line) || !input_append("\n")) {
			printf("Maximum length (%db) exceeded.\n", MAX_DATA);
			input_reset();
			continue;
		}

		char const *lua_exp = make_lua_exp(input.buf, buf_pos(&input));
		if (!lua_exp) {
			prompt = "...> ";
			continue;
		}

		char type = 0;
		FILE *remote = request(lua_exp, &type);
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
