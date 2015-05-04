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

#define METHOD_PLUS  0
#define METHOD_MINUS 1
#define METHOD_PER   2

#define MF_MASK (FocusChangeMask|StructureNotifyMask|SubstructureNotifyMask)

#ifdef SUNOS
#ifndef RAND_MAX
#define RAND_MAX 65535
#endif /* RAND_MAX */
#endif /* SUNOS */

static Display* d;
static Window w;
static MFC* mfc;

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

/*int _Xdebug = 1;*/

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
chg_grp(int num)
{
  char dumy;

  mfc->current = num;
  write(anime_pipe[1], &dumy, 1);
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
  puts("  -position <pos> |");
  puts("  -p <pos>\t: set display position. (([+-]?<n>)|(<n>%))");
  puts("  -version|");
  puts("  -v\t\t: show version information.");
  puts("  -help |");
  puts("  -? |");
  puts("  -h\t\t: show this help message.");
  exit(exitcode);
}

#if 0
int
xerr(Display* d, XErrorEvent* e)
{
  char message[1024];

  return _XError(d, e);
/*
  if(e->resourceid == w){
    fprintf(stderr, "mindfocus: X protocol error\n\t%d, %d, %d, %d, %d, %d\n",
	    e->type, e->display, e->resourceid, e->serial,
	    e->error_code, e->request_code, e->minor_code);
  }
  return 0;
*/
}
#endif

void
animefunc(int code)
{
  struct itimerval timer;

  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 1000 * ms_run();
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
    focus = win;
    if(title){
      XQueryTree(d, win, &root, &parent, &children, &nchildren);
      if(win == root)
	return 0;
      for(;;){
	if(parent == root)
	  break;
	focus = parent;
	XQueryTree(d, focus, &root, &parent, &children, &nchildren);
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
getpos(int title, int* x, int* y, int pos, int method, Window* win)
{
  int rx;
  int ry;
  int wx;
  int wy;
  int mask;

  /* check focus window */
  for(XGetInputFocus(d, &focus, &revert); focus == 1; sleep(1))
    XGetInputFocus(d, &focus, &revert);

  if(title){
    /* TITLE = <num> */
    XGetGeometry(d, focus, &root, x, y, &width, &height, &border, &depth);
    XQueryPointer(d, focus, &root, &child, &rx, &ry, &wx, &wy, &mask);

    /* Window position is "<global pointer> - <local pointer>"! */
    *x = rx - wx;
    *y = ry - wy - title;
  }else{
    /* TITLE = Auto */
    XQueryTree(d, focus, &root, &parent, &children, &nchildren);
    if(focus == root){
      XQueryTree(d, *win, &root, &parent, &children, &nchildren);
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
      }
      XGetGeometry(d, focus, &root, x, y, &width, &height, &border, &depth);
      XSelectInput(d, focus, MF_MASK);
#ifdef DEBUG
      printf("focus win:%x\n", focus);
#endif /* DEBUG */
    }
  }
  switch(method){
  case METHOD_PLUS:
    *x += pos;
    break;
  case METHOD_MINUS:
    *x += width - pos;
    break;
  case METHOD_PER:
    *x += (int)((double)(width * pos) / 100);
    break;
  }
  *win = focus;
}

void
mainloop(INIFILE* ini, const char* position)
{
  int title;
  const char* ptr;
  int ox;
  int oy;
  int x;
  int y;
  int tx;
  int ty;
  int pos;
  int method;
  char* p;
  int stack;
  XEvent event;
  Window win;
  fd_set in_set;
  char dumy;
  int fd_max;
  struct itimerval timer;
  struct sigaction sig;

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
    if(*ptr == '-'){
      pos = atoi(&ptr[1]);
      method = METHOD_MINUS;
    }else if(*ptr == '+'){
      pos = atoi(&ptr[1]);
      method = METHOD_PLUS;
    }else{
      pos = atoi(ptr);
      for(p = (char *)ptr; ('0' <= *p) && (*p <= '9'); p++);
      if(*p == '%')
	method = METHOD_PER;
      else
	method = METHOD_PLUS;
    }
  }else{
    pos = 80;
    method = METHOD_PER;
  }

  getpos(title, &x, &y, pos, method, &win);

  if((x > 0) || (y > 0)){
    values.x = x - mfc->cx;
    values.y = y - mfc->cy;
    XConfigureWindow(d, w, CWX|CWY, &values);
    XMapWindow(d, w);
    XFlush(d);
  }

  xfd = ConnectionNumber(d);
  pipe(anime_pipe);
  fd_max = (anime_pipe[0] > xfd)? anime_pipe[0]+1: xfd+1;

  srand(pos+time(NULL));
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
    if(FD_ISSET(anime_pipe[0], &in_set)){
      read(anime_pipe[0], &dumy, 1);
      disp_grp();
    }
    if(!FD_ISSET(xfd, &in_set)){
      continue;
    }
    if(!XEventsQueued(d, QueuedAfterFlush))
      continue;
    XNextEvent(d, &event);
    switch(event.type){
    case FocusOut:
    case ButtonPress:
      break;
    default:
      continue;
    }

/*
    if((event.xany.window != w) && (event.type == FocusOut))
      XSelectInput(d, event.xany.window, NoEventMask);
*/

#ifdef DEBUG
      printf("win:%x, event:%d\n", event.xany.window, event.type);
#endif /* DEBUG */

    ox = x;
    oy = y;
    getpos(title, &x, &y, pos, method, &win);

    if((ox == x) && (oy == y)){
      stackctl(win, stack, title);
      continue;
    }

    XUnmapWindow(d, w);
    if((x > 0) || (y > 0)){
      values.x = x - mfc->cx;
      values.y = y - mfc->cy;
      XConfigureWindow(d, w, CWX|CWY, &values);
      if(stackctl(win, stack, title)){
	XMapWindow(d, w);
	XFlush(d);
      }
    }
  }

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
    if((*argv[i] == '-') && (argc != (i-1))){
      /* option */
      if(!strcmp(&argv[i][1], "p") ||
	 !strcmp(&argv[i][1], "position")){
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

  /* set X error handler */
#if 0
    XSetErrorHandler(xerr);
#endif

  /* connect */
  d = XOpenDisplay(NULL);
  if(NULL == d){
    fprintf(stderr, "%s: can not open display.\n", argv[0]);
    ini_close(dotfile);
    help(4);
  }
  w = XCreateSimpleWindow(d, RootWindow(d, 0), 0, 0, 1, 1, 0, 0, 0);
  XSelectInput(d, w, ButtonPressMask);

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

  /* main loop */
  mainloop(dotfile, position);

  mfc_close(mfc);
  XDestroyWindow(d, w);
  XCloseDisplay(d);
  ini_close(dotfile);
  exit(0);
}
