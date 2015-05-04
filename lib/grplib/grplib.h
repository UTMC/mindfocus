#ifndef __grplib_h__
#define __grplib_h__

#ifndef _XLIB_H_
#include <X11/Xlib.h>
#endif /* XLIB_H_ */

int grp_support(const char* format);
int grp_load(Display* d, Window w, const char* format, const char* filename, Pixmap* image, Pixmap* shape);
void grp_free(Display* d, Pixmap* image, Pixmap* shape);

#endif /* __grplib_h__ */

