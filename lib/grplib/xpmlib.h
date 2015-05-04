#include <X11/xpm.h>

int xpm_load(Display* d, Window w, const char* filename, Pixmap* image, Pixmap* shape)
{
  if(!XReadPixmapFile(d, w, (char*)filename, image, shape, 0))
    return 1;
  return 0;
}
