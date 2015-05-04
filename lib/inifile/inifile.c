#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "inifile.h"
#include "../../configure.h"

static void
list_add(LIST* list, const char* ptr1, int len1, const char* ptr2, int len2)
{
  LISTPARAM* param;

  if(0 == list->num){
    list->num = 1;
    list->param = malloc(sizeof(LISTPARAM));
  }else{
    list->num++;
    list->param = realloc(list->param, sizeof(LISTPARAM) * list->num);
  }

  if(list->param == NULL)
    return;

  param = &list->param[list->num - 1];

  param->keylen = len1;
  param->key = malloc(len1+1);
  if(NULL != param->key){
    strncpy(param->key, ptr1, len1);
    param->key[len1] = 0;
  }

  param->paramlen = len2;
  param->param = malloc(len2+1);
  if(NULL != param->param){
    strncpy(param->param, ptr2, len2);
    param->param[len2] = 0;
  }
}

static void
list_free(LIST* list)
{
  int i;
  LISTPARAM* param;

  if(list->param == NULL){
    list->num = 0;
    return;
  }

  for(i = 0; i < list->num; i++){
    param = &list->param[i];
    if(param->key)
      free(param->key);
    if(param->param)
      free(param->param);
  }
  free(list->param);
}

static const char*
list_search(LIST* list, const char* key, int len)
{
  int i;

  for(i = 0; i < list->num; i++){
    if(len != list->param[i].keylen)
      continue;
    if(!strncmp(list->param[i].key, key, len))
      return list->param[i].param;
  }
  return NULL;
}

static int
hash_key(const char* str, int len)
{
  int i;
  int key;

  for(key = 0, i = 0; i < len; i++)
    key += str[i];
  key &= 0xff;
  return key;
}

static void
hash_add(HASH* hash, const char* ptr1, int len1, const char* ptr2, int len2)
{
  int pos;

  pos = hash_key(ptr1, len1);
  list_add(&hash->list[pos], ptr1, len1, ptr2, len2);
}

static void
hash_free(HASH* hash)
{
  int i;

  for(i = 0; i<256; i++)
    list_free(&hash->list[i]);
}

static const char*
hash_search(HASH* hash, const char* key, int len)
{
  int pos;

  pos = hash_key(key, len);
  return list_search(&hash->list[pos], key, len);
}

INIFILE*
ini_open(const char* filename)
{
  FILE* fp;
#ifdef SUNOS
  long size;
#else /* not SUNOS */
  fpos_t size;
#endif /* SUNOS */
  char* buf;
  const char* ptr1;
  const char* ptr2;
  int len1;
  int len2;
  INIFILE* ini;

  fp = fopen(filename, "rb");
  if(NULL == fp)
    return NULL;

  fseek(fp, 0, SEEK_END);
#ifdef SUNOS
  size = ftell(fp);
#else /* not SUNOS */
  fgetpos(fp, &size);
#endif /* SUNOS */
  rewind(fp);

  buf = alloca(size + 1);
  if(NULL == buf){
    fclose(fp);
    return NULL;
  }
  fread(buf, size, 1, fp);
  buf[size] = 0;

  ini = malloc(sizeof(INIFILE));
  if(NULL == ini){
    fclose(fp);
    return NULL;
  }
  bzero(ini, sizeof(INIFILE));
  ini->fp = fp;

  /* パラメータ解析 */
  for(;;){
    while('#' == *buf){
      buf = strpbrk(buf, "\n");
      buf++;
    }

    if(0 == *buf)
      break;

    ptr1 = buf;
    buf = strpbrk(buf, " \t=\n");
    len1 = buf - ptr1;
    buf = strpbrk(buf, " =\n");
    if(*buf == '=')
      buf++;
    else if(*buf == 0)
      break;
    else{
      buf++;
      continue;
    }
    buf += strspn(buf, " \t");

    ptr2 = buf;
    buf = strpbrk(buf, "\n");
    len2 = buf - ptr2;

    if(len2 == 0){
      if(*buf == 0)
	break;
      buf++;
      continue;
    }
    buf++;

    /* キーとパラメータの組をハッシュテーブルに登録 */
    hash_add(&ini->hash, ptr1, len1, ptr2, len2);
  }

  return ini;
}

void
ini_close(INIFILE* ini)
{
  fclose(ini->fp);
  hash_free(&ini->hash);
  free(ini);
}

const char*
ini_getstr(INIFILE* ini, const char* key)
{
  return hash_search(&ini->hash, key, strlen(key));
}
