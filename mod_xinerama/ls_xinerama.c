#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <stdbool.h>
#include <stdio.h>

int main()
{
    Display *dpy = XOpenDisplay(NULL);
    int xinerama_event_base;
    int xinerama_error_base;
    bool xinerama_ready = XineramaQueryExtension(dpy,&xinerama_event_base, &xinerama_error_base);

    fprintf(stdout, "Basic Xinerama screen information - for all the details run 'xdpyinfo -ext XINERAMA'\n");

    if (!xinerama_ready)
        fprintf(stderr, "No Xinerama support detected, mod_xinerama won't do anything.");
    else {
        int nRects,i;
        XineramaScreenInfo* sInfo = XineramaQueryScreens(dpy, &nRects);
        for(i = 0 ; i < nRects ; ++i) {
            fprintf(stdout, "Screen %d: %dx%d+%d+%d\n", i, sInfo[i].width, sInfo[i].height, sInfo[i].x_org, sInfo[i].y_org);
        }
    }

    return 0;
}
