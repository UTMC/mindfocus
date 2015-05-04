#ifndef __mfc_h__
#define __mfc_h__

#ifndef _XLIB_H_
#include <X11/Xlib.h>
#endif /* XLIB_H_ */

struct _mfc{
  Display* d;
  Window w;
  int width;
  int height;
  int cx;
  int cy;
  Pixmap image;
  Pixmap shape;
#ifdef ANIMATION
  int anime;
  int current;
  Pixmap* images;
  Pixmap* shapes;
#endif /* ANIMATION */
};
typedef struct _mfc MFC;


#define MFC_ERR_NO      0
#define MFC_ERR_OPEN    1
#define MFC_ERR_FORMAT  2
#define MFC_ERR_MEMORY  3
#define MFC_ERR_SRC     4
#define MFC_ERR_SUPPORT 5
#define MFC_ERR_SRCOPEN 6

MFC* mfc_open(Display* d, Window w, const char* filename);
void mfc_close(MFC* mfc);
int mfc_error();

#endif /* __mfc_h__ */
