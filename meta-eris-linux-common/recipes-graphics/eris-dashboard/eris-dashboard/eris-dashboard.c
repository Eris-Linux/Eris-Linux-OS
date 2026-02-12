#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>

#define RADIUS     15
#define DELTA      30

#define ERIS_SYSTEM_VERSION "0.0.0"


unsigned long alloc_pixel_from_rgb(Display *dpy, int screen, unsigned char r, unsigned char g, unsigned char b)
{
	XColor color;
	Colormap cmap = DefaultColormap(dpy, screen);

	color.red   = r * 256;
	color.green = g * 256;
	color.blue  = b * 256;

	color.flags = DoRed | DoGreen | DoBlue;
	if (!XAllocColor(dpy, cmap, &color))
		return BlackPixel(dpy, screen);
	return color.pixel;
}



int main(int argc, char *argv[])
{
	Window win;
	GC gc;
	int x = 50, y = 50;

	const char *display_name = getenv("DISPLAY");

	for (int i = 1; i < argc - 1; ++i) {
		if ((strcmp(argv[i], "-display") == 0 || strcmp(argv[i], "--display") == 0)) {
			display_name = argv[i + 1];
			break;
		}
	}

	Display *dpy = XOpenDisplay(display_name);

	if (!dpy) {
		fprintf(stderr, "%s: unable to open display %s\n", argv[0], display_name);
		exit(EXIT_FAILURE);
	}

	int screen = DefaultScreen(dpy);
	int width  = DisplayWidth(dpy, screen);
	int height = DisplayHeight(dpy, screen);

	win = RootWindow(dpy, screen);

	gc = XCreateGC(dpy, win, 0, NULL);

	unsigned long light_blue_pixel = alloc_pixel_from_rgb(dpy, screen, 0x49, 0xA3, 0xB6);
	unsigned long dark_blue_pixel  = alloc_pixel_from_rgb(dpy, screen, 0x2D, 0x63, 0x6F);
	unsigned long light_red_pixel  = alloc_pixel_from_rgb(dpy, screen, 0xB6, 0x5C, 0x49);
	unsigned long dark_red_pixel   = alloc_pixel_from_rgb(dpy, screen, 0x6F, 0x38, 0x2D);


	// Clear screen

	XSetForeground(dpy, gc, dark_blue_pixel);
	XFillRectangle(dpy, win, gc, 0, 0, width, height);


	XFontStruct *font = XLoadQueryFont(dpy, "-adobe-helvetica-medium-r-normal--34-240-100-100-p-176-iso8859-1");
	if (font) {
		XSetFont(dpy, gc, font->fid);
	}
	XSetForeground(dpy, gc, light_blue_pixel);

	char message[128];

	snprintf(message, 127, "Eris Linux  v.%s", ERIS_SYSTEM_VERSION);
	message[127] = '\0';
	XDrawString(dpy, win, gc, 50, height - 50, message, strlen(message));

	int step = 0;
	x = width / 2 - DELTA;
	y = height / 2 - DELTA;


	for (;;) {

		if (access("/tmp/boot-ended", F_OK) != 0) {

			// Still booting: animated dots.
			XSetForeground(dpy, gc, dark_blue_pixel);
			XFillArc(dpy, win, gc, x - RADIUS, y - RADIUS, 2 * RADIUS, 2 * RADIUS, 0, 360 * 64);

			XSetForeground(dpy, gc, WhitePixel(dpy, screen));
			x = width  / 2;
			y = height / 2;
			switch (step) {
				case 0: x = x - DELTA; y = y - DELTA; break;
				case 1: x = x + DELTA; y = y - DELTA; break;
				case 2: x = x + DELTA; y = y + DELTA; break;
				case 3: x = x - DELTA; y = y + DELTA; break;
			}
			XFillArc(dpy, win, gc, x - RADIUS, y - RADIUS, 2 * RADIUS, 2 * RADIUS, 0, 360 * 64);
			step++;
			step %= 4;

		} else {
			// Boot just ended: erase the dots.
			if (step != 4) {
				XSetForeground(dpy, gc, dark_blue_pixel);
				XFillRectangle(dpy, win, gc, x - DELTA - RADIUS - 1, y - DELTA - RADIUS - 1, 2 * (DELTA + 2 * RADIUS + 1), 2 * (DELTA + 2 * RADIUS + 1));
				step = 4;
			}
		}

		if (access("/tmp/reboot-is-needed", F_OK) == 0) {

			// Update ready: display the reboot symbol
			int xc = width - 80;
			int yc = 80;
			XSetForeground(dpy, gc, light_red_pixel);
			XSetLineAttributes(dpy, gc, 20, LineSolid, CapRound, JoinRound);
			XDrawArc(dpy, win, gc, xc - 40, yc - 40, 80, 80, 0*64, -270*64);

			XSetLineAttributes(dpy, gc, 1, LineSolid, CapRound, JoinRound);

			XPoint points[3];
			points[0].x = xc;
			points[0].y = yc - 10;
			points[1].x = xc;
			points[1].y = yc - 70;
			points[2].x = xc + 40;
			points[2].y = yc - 40;
			XFillPolygon(dpy, win, gc, points, 3, Convex, CoordModeOrigin);
		}


		struct timeval tv;
		gettimeofday(&tv, NULL);

		struct tm *t = localtime(&(tv.tv_sec));

		if ((t->tm_year < 125) || ((t->tm_year == 125) && (t->tm_mon < 7))) {
			strcpy(message, "----/--/-- --:--:--");
		} else {
			snprintf(message, 127, "%04d/%02d/%02d %02d:%02d:%02d",
				t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec);
		}
		message[127] = '\0';

		XSetForeground(dpy, gc, light_blue_pixel);
		XSetBackground(dpy, gc, dark_blue_pixel);
		XDrawImageString(dpy, win, gc, width - 50 - XTextWidth(font, message, strlen(message)), height - 50, message, strlen(message));

		XFlush(dpy);

		usleep(500000);

	}

	XFreeGC(dpy, gc);
	XCloseDisplay(dpy);

	return 0;
}
