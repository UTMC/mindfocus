#ifndef __debug_h__
#define __debug_h__

void* ___d_malloc(size_t, const char*, int);
void* ___d_realloc(void*, size_t, const char*, int);
void ___d_free(void*, const char*, int);
char* ___d_strdup(const char*, const char*, int);
void ___d_exit(int);

#ifdef __implement__

struct ___d_meminfo {
  unsigned long magic;
  const char* fname;
  int line;
  size_t size;
  struct ___d_meminfo* next;
  struct ___d_meminfo* before;
};

static struct ___d_meminfo* ___d_meminfo_f = NULL;
static struct ___d_meminfo* ___d_meminfo_l = NULL;

void*
___d_malloc(size_t size, const char* fname, int line)
{
  void* pv = malloc(size + sizeof(struct ___d_meminfo));
  struct ___d_meminfo* info;
  if (NULL == pv) {
    fprintf(stderr, "%s(%d):%s\n", fname, line, "*** malloc failed ***\n");
    fflush(stderr);
    return NULL;
  }
  info = (struct ___d_meminfo*)pv;
  info->magic = (unsigned long)'Samy';
  info->fname = fname;
  info->line = line;
  info->size = size;
  info->next = NULL;
  info->before = NULL;
  if (NULL == ___d_meminfo_f) {
    ___d_meminfo_f = ___d_meminfo_l = info;
  } else {
    ___d_meminfo_l->next = info;
    info->before = ___d_meminfo_l;
    ___d_meminfo_l = info;
  }
  info++;
  return (void*)info;
}

void*
___d_realloc(void* pv, size_t size, const char* fname, int line)
{
  void* result;
  struct ___d_meminfo* info = (struct ___d_meminfo*)pv;
  info--;
  if (info->magic != (unsigned long)'Samy') {
    fprintf(stderr, "%s(%d):%s\n", fname, line, "*** realloc called with invalid pv ***\n");
    fflush(stderr);
    return realloc(pv, size);
  }
  result = ___d_malloc(size, fname, line);
  if (NULL == result) {
    ___d_free(pv, fname, line);
    return NULL;
  }
  memcpy(result, pv, info->size);
  info->size = size;
  ___d_free(pv, fname, line);
  return result;
}

void
___d_free(void* pv, const char* fname, int line)
{
  struct ___d_meminfo* info;
  if (NULL == pv) return;
  info = (struct ___d_meminfo*)pv;
  info--;
  if (info->magic != (unsigned long)'Samy') {
    fprintf(stderr, "%s(%d):%s\n", fname, line, "*** free called with invalid pv ***\n");
    fflush(stderr);
    return free(pv);
    /*
  } else {
    fprintf(stderr, "%s(%d):free %s(%d)\n", fname, line, info->fname, info->line);
    fflush(stderr);
    */
  }
  info->magic = 0;
  if (NULL != info->next) info->next->before = info->before;
  if (NULL != info->before) info->before->next = info->next;
  if (___d_meminfo_f == info) ___d_meminfo_f = info->next;
  if (___d_meminfo_l == info) ___d_meminfo_l = info->before;
  free(info);
}

char*
___d_strdup(const char* c, const char* fname, int line)
{
  int size = strlen(c);
  char* result = (char*)___d_malloc(size + 1, fname, line);
  if (NULL != result) {
    memcpy(result, c, size);
    result[size] = 0;
  }
  return result;
}

void
___d_exit(int code)
{
  struct ___d_meminfo* p = ___d_meminfo_f;
  char* data;
  unsigned int ui;

  if (NULL != p) {
    fprintf(stderr, "*** memory leak detect ***\n");
    for (; NULL != p; p = p->next) {
      data = (char*)(&p[1]);
      fprintf(stderr, "%s(%d):0x%08x %dbyte(s) [", p->fname, p->line, &p[1], p->size);
      for (ui = 0; ui < p->size; ui++) {
	fprintf(stderr, "%c", data[ui]);
	if (ui == 7) {
	  fprintf(stderr, "...");
	  break;
	}
      }
      fprintf(stderr,"(");
      for (ui = 0; ui < p->size; ui++) {
	fprintf(stderr, "%02x", data[ui] & 0xff);
	if (ui == 7) {
	  fprintf(stderr, "...");
	  break;
	}
      }
      fprintf(stderr, ")]\n");
    }
  }
  exit(code);
}

#endif /* __implement__ */

#define malloc(s) ___d_malloc(s,__FILE__,__LINE__)
#define realloc(v,s) ___d_realloc(v,s,__FILE__,__LINE__)
#define free(v) ___d_free(v,__FILE__,__LINE__)
#define strdup(c) ___d_strdup(c,__FILE__,__LINE__)
#define exit(i) ___d_exit(i)

#endif /* __debug_h__ */
