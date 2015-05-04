#ifndef __inifile_h__
#define __inifile_h__

struct _listparam{
  int keylen;
  char* key;
  int paramlen;
  char* param;
};
typedef struct _listparam LISTPARAM;

struct _list{
  int num;
  LISTPARAM* param;
};
typedef struct _list LIST;

struct _hash{
  LIST list[256];
};
typedef struct _hash HASH;

struct _inifile{
  FILE* fp;
  HASH hash;
};
typedef struct _inifile INIFILE;


INIFILE* ini_open(const char* filename);
void ini_close(INIFILE* ini);
const char* ini_getstr(INIFILE* ini, const char* key);

#endif /* __inifile_h__ */
