/* emit foo webcam button event */

/* gcc -o test-webcam-button test-webcam-button.c -lX11 -lXtst */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/XF86keysym.h>

int
main (void)
{
  Display *dpy;

  dpy = XOpenDisplay (NULL);

  printf ("Emitting fake Webcam button press in 2 seconds...\n");
  printf ("Focus Cheese window to make it receive the event\n");

  XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, XF86XK_WebCam), True, 2000);
  XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, XF86XK_WebCam), False, CurrentTime);

  XCloseDisplay (dpy);
  return 0;
}
