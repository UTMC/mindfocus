#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../lib/inifile/inifile.h"
#include "../lib/grplib/grplib.h"
#include "../lib/mindscript/mindscript.h"
#include "../configure.h"
#include "mfc.h"

static int error = 0;

static int
strcut(char* str)
{
  if(!*str)
    return 0;

  for(;; str++){
    switch(*str){
    case ' ':
    case '\t':
      *str = 0;
    case 0:
      return 1;
    }
  }
}

static char*
strnext(char* str)
{
  for(; *str != 0; str++);
  str++;
  for(; (*str == ' ') || (*str == '\t'); str++);
  return str;
}

static void
file2dir(char* name)
{
  char* p;

  for(p = name; *name != 0; name++)
    if(*name == '/')
      p = name;

  for(; *p != 0; *p++ = 0);
}

MFC*
mfc_open(Display* d, Window w, const char* filename)
{
  INIFILE* ini;
  MFC* mfc;
  const char* srcname;
  const char* format;
  const char* ptr;
  const char* script;
  char* dir;
  char* tmpname;
  int i;
  int size;

  ini = ini_open(filename);
  if(NULL == ini){
    error = MFC_ERR_OPEN;
    return NULL;
  }

  srcname = ini_getstr(ini, "SRC");
  format = ini_getstr(ini, "FORMAT");
  script = ini_getstr(ini, "SCRIPT");

  if((NULL == srcname) || (NULL == format)){
    error = MFC_ERR_FORMAT;
    ini_close(ini);
    return NULL;
  }

  if(!grp_support(format)){
    error = MFC_ERR_SUPPORT;
    ini_close(ini);
    return NULL;
  }

  mfc = malloc(sizeof(MFC));
  if(NULL == mfc){
    error = MFC_ERR_MEMORY;
    ini_close(ini);
    return NULL;
  }

  mfc->d = d;
  mfc->w = w;

  ptr = ini_getstr(ini, "WIDTH");
  mfc->width = (NULL == ptr)? 0: atoi(ptr);
  ptr = ini_getstr(ini, "HEIGHT");
  mfc->height = (NULL == ptr)? 0: atoi(ptr);
  ptr = ini_getstr(ini, "CX");
  mfc->cx = (NULL == ptr)? 0: atoi(ptr);
  ptr = ini_getstr(ini, "CY");
  mfc->cy = (NULL == ptr)? 0: atoi(ptr);
#ifdef ANIMATION
  ptr = ini_getstr(ini, "ANIME");
  mfc->anime = (NULL == ptr)? 1: atoi(ptr);
  mfc->current = 0;
  mfc->images = malloc(mfc->anime * sizeof(Pixmap));
  mfc->shapes = malloc(mfc->anime * sizeof(Pixmap));
  if((NULL == mfc->images) || (NULL == mfc->shapes)){
    if(mfc->images)
      free(mfc->images);
    if(mfc->shapes)
      free(mfc->shapes);
    error = MFC_ERR_MEMORY;
    ini_close(ini);
    free(mfc);
    return NULL;
  }
#endif /* ANIMATION */

  /* chdir to mfc file dir */
  dir = alloca(strlen(filename) + 1);
  if(NULL != dir){
    strcpy(dir, filename);
    file2dir(dir);
    if(dir[0] != 0)
      chdir(dir);
  }


#ifdef ANIMATION
  ms_init(script);
  size = strlen(srcname);
  tmpname = alloca(size + 2);
  if(NULL == tmpname){
    error = MFC_ERR_MEMORY;
    ini_close(ini);
    free(mfc->images);
    free(mfc->shapes);
    free(mfc);
    return NULL;
  }
  strcpy(tmpname, srcname);
  tmpname[size] = tmpname[size+1] = 0;

  for(i = 0; i<mfc->anime; i++){
    strcut(tmpname);
    /* file check */
    error = 0;
    if(access(tmpname, R_OK))
      error = MFC_ERR_SRCOPEN;
    else if(!strcut(tmpname) ||
       !grp_load(d, w, format, tmpname, &mfc->images[i], &mfc->shapes[i]))
	error = MFC_ERR_SRC;
    if(error){
      ini_close(ini);
      for(i--; i >= 0; i--)
	grp_free(mfc->d, &mfc->images[i], &mfc->shapes[i]);
      free(mfc);
      return NULL;
    }
    tmpname = strnext(tmpname);
  }
  mfc->image = mfc->images[0];
  mfc->shape = mfc->shapes[0];
#else /* ANIMATION */
  /* file check */
  error = 0;
  if(access(tmpname, R_OK))
    error = MFC_ERR_SRCOPEN;
  if(!grp_load(d, w, format, srcname, &mfc->image, &mfc->shape))
    error = MFC_ERR_SRC;
  if(error){
    ini_close(ini);
    free(mfc);
    return NULL;
  }
#endif /* ANIMATION */

  ini_close(ini);
  return mfc;
}

void
mfc_close(MFC* mfc)
{
  int i;
#ifdef ANIMATION
  ms_trash();
  for(i = 0; i<mfc->anime; i++)
    grp_free(mfc->d, &mfc->images[i], &mfc->shapes[i]);
  free(mfc->images);
  free(mfc->shapes);
#else /* ANIMATION */
  grp_free(mfc->d, &mfc->image, &mfc->shape);
#endif /* ANIMATION */
  free(mfc);
}

int
mfc_error()
{
  return error;
}
