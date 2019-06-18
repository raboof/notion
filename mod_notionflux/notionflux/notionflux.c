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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

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

/* read multiple lines */
void slurp(FILE *f, char *buf, size_t n)
{
	while (n > 0 && fgets(buf, n, f) != NULL) {
		size_t len = strlen(buf);
		n -= len;
		buf += len;
	}
}

/* fputs, but with trailing NUL character - expected by mod_notionflux */
void barf(FILE *f, char *str)
{
	fputs(str, f);
	fputc('\0', f);
	fflush(f);
}


int main(int argc, char **argv)
{
	sockfilename = get_socket_filename();
	FILE *remote = sock_connect();
        char buf[MAX_DATA + 1];
	char *data;

	if (argc == 1) {
		data = buf;
		slurp(stdin, buf, MAX_DATA);
	} else if (argc == 3 && strcmp(argv[1], "-e") == 0) {
		data = argv[2];
		if (strlen(data) > MAX_DATA)
			die("Too much data\n");
	} else {
		die("Usage: %s [-e code]\n", argv[0]);
	}

	barf(remote, data);
	slurp(remote, buf, MAX_DATA + 1);

	if (buf[0] != 'S' && buf[0] != 'E')
		die("Unknown response type %c\n", buf[0]);

	fputs(buf + 1, buf[0] == 'S' ? stdout : stderr);

	return 0;
}
