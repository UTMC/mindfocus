#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "../../configure.h"
#ifdef XPM
#include "./xpmlib.h"
#endif /* XPM */


static char* support_list[] =
{
#ifdef XPM
  "XPM",
#endif /* XPM */
  NULL
};


int
grp_support(const char* format)
{
  int i;

  for(i = 0; support_list[i] != NULL; i++)
    if(!strcmp(support_list[i], format))
      return 1;
  return 0;
}

int
grp_load(Display* d, Window w, const char* format, const char* filename, Pixmap* image, Pixmap* shape)
{
  switch(*((unsigned char*)format)){
#ifdef XPM
  case 'X':
    return xpm_load(d, w, filename, image, shape);
#endif /* XPM */
  default:
    return 0;
  }
}

void
grp_free(Display* d, Pixmap* image, Pixmap* shape)
{
  XFreePixmap(d, *image);
  XFreePixmap(d, *shape);
}
