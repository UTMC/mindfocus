#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "../configure.h"
#include "../lib/inifile/inifile.h"
#include "../lib/mindscript/mindscript.h"
#include "./mfc.h"
#include "./mindfocus.h"
#ifdef DEBUG
#include "../debug.h"
#endif /* DEBUG */

#define METHOD_PLUS  0
#define METHOD_MINUS 1
#define METHOD_PER   2

#define PIPE_DISPLAY 'D'
#define PIPE_MOVE    'M'

#define MF_MASK (FocusChangeMask|StructureNotifyMask|SubstructureNotifyMask)

static Display* d;
static Window w;
static MFC* mfc;
static POSITION winpos;

static XWindowChanges values;
static XSetWindowAttributes attr;
static Window focus;
static Window root;
static Window parent;
static Window child;
static Window* children;
static int nchildren;
static int depth;
static int border;
static int width;
static int height;
static int revert;
static int xfd;
static int anime_pipe[2];
static XEvent retry_event;
static int mf_xerr = 0;

#ifdef DEBUG
int _Xdebug = 1;
#endif /* DEBUG */

int
errorhandler(Display *display, XErrorEvent *err)
{
#ifdef DEBUG
  char errmsg[128];
  XGetErrorText(display, err->error_code, errmsg, sizeof(errmsg));
  printf("An Error occured.\n  %s\n", errmsg);
#endif
  mf_xerr = 1;
  return 0;
}

void
calcpos(int* x, int* y, POSITION* pos)
{
  if(pos->win_w == 0){
    *x = *y = 0;
    return;
  }

  switch(pos->x_method){
  case METHOD_PLUS:
    pos->from_left = pos->x_param;
    pos->from_right = pos->win_w - pos->from_left;
    pos->x_percent = pos->from_left * 100 / pos->win_w;
    break;
  case METHOD_MINUS:
    pos->from_right = pos->x_param;
    pos->from_left = pos->win_w - pos->from_right;
    pos->x_percent = pos->from_left * 100 / pos->win_w;
    break;
  case METHOD_PER:
    pos->x_percent = pos->x_param;
    pos->from_left = (int)((double)(pos->win_w * pos->x_param) / 100);
    pos->from_right = pos->win_w - pos->from_left;
    break;
  }
  switch(pos->y_method){
  case METHOD_PLUS:
    pos->from_top = pos->y_param;
    pos->from_bottom = pos->win_h - pos->from_top;
    pos->y_percent = pos->from_top * 100 / pos->win_h;
    break;
  case METHOD_MINUS:
    pos->from_bottom = pos->y_param;
    pos->from_top = pos->win_h - pos->from_bottom;
    pos->y_percent = pos->from_top * 100 / pos->win_h;
    break;
  case METHOD_PER:
    pos->y_percent = pos->y_param;
    pos->from_top = (int)((double)(pos->win_h * pos->y_param) / 100);
    pos->from_bottom = pos->win_h - pos->from_top;
    break;
  }
  *x = pos->win_x + pos->from_left;
  *y = pos->win_y + pos->from_top;
}

void
set_x_pos_by_per(int x_pos)
{
  static char command = PIPE_MOVE;
  int x, y;

  winpos.x_param = x_pos;
  winpos.x_method = METHOD_PER;
  calcpos(&x, &y, &winpos);
  write(anime_pipe[1], &command, 1);
}

int
get_x_pos_by_per()
{
  return winpos.x_percent;
}

void
set_x_pos_from_left(int x_pos)
{
  static char command = PIPE_MOVE;
  int x, y;

  winpos.x_param = x_pos;
  winpos.x_method = METHOD_PLUS;
  calcpos(&x, &y, &winpos);
  write(anime_pipe[1], &command, 1);
}

int
get_x_pos_from_left()
{
  return winpos.from_left;
}

void
set_x_pos_from_right(int x_pos)
{
  static char command = PIPE_MOVE;
  int x, y;

  winpos.x_param = x_pos;
  winpos.x_method = METHOD_MINUS;
  calcpos(&x, &y, &winpos);
  write(anime_pipe[1], &command, 1);
}

int
get_x_pos_from_right()
{
  return winpos.from_right;
}

void
set_y_pos_by_per(int y_pos)
{
  static char command = PIPE_MOVE;
  int x, y;

  winpos.y_param = y_pos;
  winpos.y_method = METHOD_PER;
  calcpos(&x, &y, &winpos);
  write(anime_pipe[1], &command, 1);
}

int
get_y_pos_by_per()
{
  return winpos.y_percent;
}

void
set_y_pos_from_top(int y_pos)
{
  static char command = PIPE_MOVE;
  int x, y;

  winpos.y_param = y_pos;
  winpos.y_method = METHOD_PLUS;
  calcpos(&x, &y, &winpos);
  write(anime_pipe[1], &command, 1);
}

int
get_y_pos_from_top()
{
  return winpos.from_top;
}

void
set_y_pos_from_bottom(int y_pos)
{
  static char command = PIPE_MOVE;
  int x, y;

  winpos.y_param = y_pos;
  winpos.y_method = METHOD_MINUS;
  calcpos(&x, &y, &winpos);
  write(anime_pipe[1], &command, 1);
}

int
get_y_pos_from_bottom()
{
  return winpos.from_bottom;
}

void
chg_grp(int num)
{
  static char command = PIPE_DISPLAY;

  mfc->current = num;
  write(anime_pipe[1], &command, 1);
}

void
disp_grp()
{
  attr.background_pixmap = mfc->images[mfc->current];
  XChangeWindowAttributes(d, w, CWBackPixmap, &attr);
  XShapeCombineMask(d, w, 0, 0, 0, mfc->shapes[mfc->current], 0);
  XClearWindow(d, w);
  XFlush(d);
}

void
version_info()
{
  printf("mindfocus version %1.2f\n", VERSION);
  puts("\t1998 (C)Takashi TOYOSHIMA");
  puts("\tUniversity of Tokyo Microcomputer Club\n");
  exit(0);
}

void
help(int exitcode)
{
  puts("Usage: mindfocus [options] [mfc filename1] [options1] ...");
  puts("Options:");
  puts("  -position <x>[,<y>] |");
  puts("  -p <x>[,<y>]\t: set display position. (([+-]?<n>)|(<n>%))");
  puts("  -version|");
  puts("  -v\t\t: show version information.");
  puts("  -help |");
  puts("  -? |");
  puts("  -h\t\t: show this help message.");
  exit(exitcode);
}

void
animefunc(int code)
{
  struct itimerval timer;
  int wait;

  wait = ms_run();
  timer.it_value.tv_sec = wait / 1000;
  timer.it_value.tv_usec = 1000 * (wait % 1000);
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &timer, NULL);
}

int
stackctl(Window win, int stack, int title)
{
  int tx;
  int ty;

  if(stack){
    /* stack control */
    if((PointerRoot == win) || (None == win)) {
      /* invalid window */
      return 0;
    }
    focus = win;
    if(title){
      XQueryTree(d, win, &root, &parent, &children, &nchildren);
      XFree(children);
      if(win == root)
	return 0;
      for(;;){
	if(parent == root)
	  break;
	focus = parent;
	XQueryTree(d, focus, &root, &parent, &children, &nchildren);
	XFree(children);
      }
      XGetGeometry(d, focus, &root, &tx, &ty, &width, &height, &border, &depth);
    }
    values.sibling = win;
    values.stack_mode = Above;
    XConfigureWindow(d, w, CWSibling|CWStackMode, &values);
  }else{
    /* always top */
    XRaiseWindow(d, w);
  }
  return 1;
}

void
getpos(int title, int* x, int* y, POSITION* pos, Window* win)
{
  int rx;
  int ry;
  int wx;
  int wy;
  int mask;

  /* check focus window */
#ifndef OLD_METHOD
  XGetInputFocus(d, &focus, &revert);
  if((PointerRoot == focus) || (None == focus)) {
    /* invalid window */
    pos->win_w = 0;
    *win = focus;
    XSendEvent(d, w, True, 0, &retry_event);
    calcpos(x, y, pos);
    return;
  }else if(focus == RootWindow(d, 0)){
    /* root window */
    XGetGeometry(d, *win, &root, &pos->win_x, &pos->win_y,
		 &pos->win_w, &pos->win_h, &border, &depth);
    XSendEvent(d, w, True, 0, &retry_event);
    calcpos(x, y, pos);
    return;
  }

#else /* OLD_METHOD */
  for(XGetInputFocus(d, &focus, &revert); PointerRoot == focus;)
    XGetInputFocus(d, &focus, &revert);
#endif /* OLD_METHOD */

  if(title){
    /* TITLE = <num> */
    XGetGeometry(d, focus, &root, x, y, &width, &height, &border, &depth);
    XQueryPointer(d, focus, &root, &child, &rx, &ry, &wx, &wy, &mask);

    /* Window position is "<global pointer> - <local pointer>"! */
    *x = rx - wx;
    *y = ry - wy - title;
  }else{
    /* TITLE = Auto */
#ifndef OLD_METHOD
    do{
      XQueryTree(d, focus, &root, &parent, &children, &nchildren);
      if(mf_xerr != 0)
	break; /* quit */
      XFree(children);
      for(;parent != root;){
	focus = parent;
	XQueryTree(d, focus, &root, &parent, &children, &nchildren);
	if(mf_xerr != 0)
	  break; /* quit */
	XFree(children);
      }
      if(mf_xerr != 0)
	break; /* quit */
      XGetGeometry(d, focus, &root, x, y, &width, &height, &border, &depth);
      XSelectInput(d, focus, MF_MASK);
    }while(0); /* dummy block */
    if(mf_xerr != 0){
      mf_xerr = 0;
      pos->win_w = 0;
      *win = focus;
      XSendEvent(d, w, True, 0, &retry_event);
      calcpos(x, y, pos);
      return;
    }
    
#else /* OLD_METHOD */
    XQueryTree(d, focus, &root, &parent, &children, &nchildren);
    XFree(children);
    if(focus == root){
      XQueryTree(d, *win, &root, &parent, &children, &nchildren);
      XFree(children);
      if(focus == root)
        focus = *win;
      XGetGeometry(d, focus, &root, x, y, &width, &height, &border, &depth);
      XSelectInput(d, root, MF_MASK);
      XSelectInput(d, focus, MF_MASK);
#ifdef DEBUG
      printf("root win:%x\n", root);
#endif /* DEBUG */
    }else{
      for(;;){
	if(parent == root)
	  break;
	focus = parent;
	XQueryTree(d, focus, &root, &parent, &children, &nchildren);
	XFree(children);
      }
      XGetGeometry(d, focus, &root, x, y, &width, &height, &border, &depth);
      XSelectInput(d, focus, MF_MASK);
#ifdef DEBUG
      printf("focus win:%x\n", focus);
#endif /* DEBUG */
    }

#endif /* OLD_METHOD */
  }

  *win = focus;
  pos->win_x = *x;
  pos->win_y = *y;
  pos->win_w = width;
  pos->win_h = height;
  calcpos(x, y, pos);
}

void
mainloop(INIFILE* ini, const char* position)
{
  int title;
  const char* ptr;
  int ox;
  int oy;
  int tx;
  int ty;
  int x;
  int y;
  char* p;
  int stack;
  XEvent event;
  Window win;
  fd_set in_set;
  char command[256];
  int num_of_com;
  int fd_max;
  struct itimerval timer;
  struct sigaction sig;
  int i;

  ptr = ini_getstr(ini, "TITLE");
  title = (NULL == ptr)? 0: atoi(ptr);
  ptr = ini_getstr(ini, "STACK");
  stack = 1;
  if(ptr)
    if(!strcmp(ptr, "OFF"))
      stack = 0;
  if(position)
    ptr = position;
  else
    ptr = ini_getstr(ini, "POS");
  if(ptr){
    p = (char *)ptr;
    if(*p == '-'){
      winpos.x_param = atoi(++p);
      for(; isdigit(*p); p++) /* skip num */;
      winpos.x_method = METHOD_MINUS;
    }else if(*p == '+'){
      winpos.x_param = atoi(++p);
      for(; isdigit(*p); p++) /* skip num */;
      winpos.x_method = METHOD_PLUS;
    }else{
      winpos.x_param = atoi(p);
      for(; isdigit(*p); p++) /* skip num */;
      if(*p == '%'){
	winpos.x_method = METHOD_PER;
	p++;
      }else{
	winpos.x_method = METHOD_PLUS;
      }
    }
    if(*p == ','){
      /* position-Y */
      p++;
      if(*p == '-'){
	winpos.y_param = atoi(++p);
	for(; isdigit(*p); p++) /* skip num */;
	winpos.y_method = METHOD_MINUS;
      }else if(*p == '+'){
	winpos.y_param = atoi(++p);
	for(; isdigit(*p); p++) /* skip num */;
	winpos.y_method = METHOD_PLUS;
      }else{
	winpos.y_param = atoi(p);
	for(; isdigit(*p); p++) /* skip num */;
	if(*p == '%'){
	  winpos.y_method = METHOD_PER;
	  p++;
	}else{
	  winpos.y_method = METHOD_PLUS;
	}
      }
    }else{
      winpos.y_param = 0;
      winpos.y_method = METHOD_PLUS;
    }
  }else{
    winpos.x_param = 80;
    winpos.x_method = METHOD_PER;
    winpos.y_param = 0;
    winpos.y_method = METHOD_PLUS;
  }
  winpos.win_x = 0;
  winpos.win_y = 0;
  winpos.win_w = 0;
  winpos.win_h = 0;

  xfd = ConnectionNumber(d);
  pipe(anime_pipe);
  fd_max = (anime_pipe[0] > xfd)? anime_pipe[0]+1: xfd+1;

  for(XGetInputFocus(d, &focus, &revert); PointerRoot == focus;)
    XGetInputFocus(d, &focus, &revert);
  getpos(title, &x, &y, &winpos, &win);

  if((x > 0) || (y > 0)){
    values.x = x - mfc->cx;
    values.y = y - mfc->cy;
    XConfigureWindow(d, w, CWX|CWY, &values);
    XMapWindow(d, w);
    XFlush(d);
  }

  srand(winpos.x_param+time(NULL));
  bzero(&sig, sizeof(struct sigaction));
  sig.sa_handler = animefunc;
  sigaction(SIGALRM, &sig, NULL);

  animefunc(0);

  /* main loop */
  for(;;){
    FD_ZERO(&in_set);
    FD_SET(xfd, &in_set);
    FD_SET(anime_pipe[0], &in_set);
    if(0 >= select(fd_max, &in_set, NULL, NULL, NULL))
      continue;
    /* mindfocus message is comming */
    if(FD_ISSET(anime_pipe[0], &in_set)){
      /* check command type */
      num_of_com = read(anime_pipe[0], command, 256);
      for(i = 0; i<num_of_com; i++){
	switch(command[i]){
	case PIPE_DISPLAY:
	  /* redraw */
	  disp_grp();
	  break;
	case PIPE_MOVE:
	  /* move */
	  getpos(title, &x, &y, &winpos, &win);
	  /* calcpos(&x, &y, &winpos);*/
	  values.x = x - mfc->cx;
	  values.y = y - mfc->cy;
	  XConfigureWindow(d, w, CWX|CWY, &values);
	  break;
	}
      }
    }

    /* xevent? */
    if(!FD_ISSET(xfd, &in_set)){
      continue;
    }

    /* check event queue */
    while(XEventsQueued(d, QueuedAfterFlush)){
      XNextEvent(d, &event);
      switch(event.type){
      case FocusOut:
      case FocusIn:
	break;
      case ConfigureNotify:
	continue;
      case ButtonPress:
/*
	if(event.xany.window != w)
	  continue;
*/
	break;
      default:
	continue;
      } /* switch(event.type) */

      ox = x;
      oy = y;
      getpos(title, &x, &y, &winpos, &win);

      if((ox == x) && (oy == y)){
	stackctl(win, stack, title);
	continue;
      }

      if((x > 0) || (y > 0)){
	values.x = x - mfc->cx;
	values.y = y - mfc->cy;
	XConfigureWindow(d, w, CWX|CWY, &values);
	if(stackctl(win, stack, title)){
	  XFlush(d);
	}
      }
    } /* while(XEventsQueued(d, QueuedAfterFlush)) */
  } /* for(;;) */

  bzero(&timer, sizeof(struct itimerval));
  setitimer(0, &timer, NULL);

  bzero(&sig, sizeof(struct sigaction));
  sig.sa_handler = SIG_DFL;
  sigaction(SIGALRM, &sig, NULL);
}

int
main(int argc, char** argv)
{
  char* home;
  char* dotfilename;
  char* dir;
  const char* mfcfilename = NULL;
  const char* position = NULL;
  const char* userpath = NULL;
  char* tmp;
  INIFILE* dotfile;
  int errcode;
  int i;

  /* options */
  for(i = 1; i < argc; i++){
    if(*argv[i] == '-'){
      /* option */
      if((!strcmp(&argv[i][1], "p") ||
	  !strcmp(&argv[i][1], "position")) && (i != argc-1)){
	position = argv[++i];
      }else if(!strcmp(&argv[i][1], "v") ||
	       !strcmp(&argv[i][1], "version")){
	version_info();
      }else{
	fprintf(stderr, "%s: invalid option.\n", argv[0]);
	help(1);
      }
    }else{
      /* mfc filename */

      if(NULL != mfcfilename){
	/* multi target */
	switch(fork()){
	case -1:
	  /* failed */
	  fprintf(stderr, "%s: can not forked.\n", argv[0]);
	  help(2);
	  break;
	case 0:
	  /* child process */
	  mfcfilename = argv[i];
	  position = NULL;
	  break;
	default:
	  /* parent process */
	  i = argc; /* break loop */
	  break;
	}
      }else{
	/* this is first */
	mfcfilename = argv[i];
      }
    }
  }

  /* load .mindfocus */
  home = getenv("HOME");
  dotfilename = alloca(strlen(home) + 1 + strlen(DOTFILENAME) + 1);
  strcpy(dotfilename, home);
  strcat(dotfilename, "/");
  strcat(dotfilename, DOTFILENAME);
  dotfile = ini_open(dotfilename);
  if(NULL == dotfile)
    dotfile = ini_open(SYSTEMRC);

  if(dotfile == NULL){
    fprintf(stderr, "%s: '%s' can not opened.\n", argv[0], SYSTEMRC);
    help(3);
  }

  /* connect */
  d = XOpenDisplay(NULL);
  if(NULL == d){
    fprintf(stderr, "%s: can not open display.\n", argv[0]);
    ini_close(dotfile);
    help(4);
  }
  w = XCreateSimpleWindow(d, RootWindow(d, 0), 0, 0, 1, 1, 0, 0, 0);
  XSelectInput(d, w, ButtonPressMask|MF_MASK);

  /* load mfc file */
  if(NULL == mfcfilename)
    mfcfilename = ini_getstr(dotfile, "MFC");

  if(NULL == mfcfilename){
    fprintf(stderr, "%s: no mfc file specified.\n", argv[0]);
    XDestroyWindow(d, w);
    XCloseDisplay(d);
    ini_close(dotfile);
    help(5);
  }

  /* open <filename> */
  mfc = mfc_open(d, w, mfcfilename);
  if(*mfcfilename != '/'){

    /* open USERPATH/<filename> */
    if((NULL == mfc) && (mfc_error() == MFC_ERR_OPEN)){
      userpath = ini_getstr(dotfile, "PATH");
      if(NULL != userpath){
	tmp = alloca(strlen(userpath) + strlen(mfcfilename) + 2);
	if(NULL != tmp){
	  strcpy(tmp, userpath);
	  strcat(tmp, "/");
	  strcat(tmp, mfcfilename);
	  mfc = mfc_open(d, w, tmp);
	  if(NULL != mfc)
	    mfcfilename = tmp;
	}
      }
    }

    /* open DEFAULTDIR/<filename> */
    if((NULL == mfc) && (mfc_error() == MFC_ERR_OPEN)){
      tmp = alloca(strlen(DEFAULTDIR) + strlen(mfcfilename) + 2);
      if(NULL != tmp){
	strcpy(tmp, DEFAULTDIR);
	strcat(tmp, "/");
	strcat(tmp, mfcfilename);
	mfc = mfc_open(d, w, tmp);
	if(NULL != mfc)
	  mfcfilename = tmp;
      }
    }
  }

  if(NULL == mfc){
    errcode = mfc_error();
    switch(errcode){
    case MFC_ERR_OPEN:
      fprintf(stderr, "%s: '%s' can not opened.\n", argv[0], mfcfilename);
      break;
    case MFC_ERR_FORMAT:
      fprintf(stderr, "%s: '%s' is invalid.\n", argv[0], mfcfilename);
      break;
    case MFC_ERR_MEMORY:
      fprintf(stderr, "%s: malloc error.\n", argv[0]);
      break;
    case MFC_ERR_SRC:
      fprintf(stderr, "%s: src file is invalid.\n", argv[0]);
      break;
    case MFC_ERR_SUPPORT:
      fprintf(stderr, "%s: unsupported format.\n", argv[0]);
      break;
    case MFC_ERR_SRCOPEN:
      fprintf(stderr, "%s: invalid reference in '%s'.\n", argv[0], mfcfilename);
      break;
    default:
      fprintf(stderr, "%s: mfc format error.\n", argv[0]);
    }
    XDestroyWindow(d, w);
    XCloseDisplay(d);
    ini_close(dotfile);
    help(6+errcode);
  }

  XStoreName(d, w, "mindfocus");
  attr.override_redirect = True;
  XChangeWindowAttributes(d, w, CWOverrideRedirect, &attr);

  attr.background_pixmap = mfc->image;
  XChangeWindowAttributes(d, w, CWBackPixmap, &attr);
  XShapeCombineMask(d, w, 0, 0, 0, mfc->shape, 0);
  values.width = mfc->width;
  values.height = mfc->height;
  XConfigureWindow(d, w, CWWidth|CWHeight, &values);
  XSetErrorHandler(errorhandler);

  retry_event.type = FocusOut;

  /* main loop */
  mainloop(dotfile, position);

  mfc_close(mfc);
  XDestroyWindow(d, w);
  XCloseDisplay(d);
  ini_close(dotfile);
  exit(0);
  return 0;
}
