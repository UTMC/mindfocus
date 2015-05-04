#ifndef __mindscript_h__
#define __mindscript_h__


struct _mswork_exp{
  int type;
  void* ptr;
};
typedef struct _mswork_exp MSWORKEXP;

#define MSTYPENULL   -1
#define MSTYPENUMBER 0
#define MSTYPESYMBOL 1
#define MSTYPEBEGIN  2
#define MSTYPECOND   3
#define MSTYPESETQ   4
#define MSTYPERAND   5
#define MSTYPEEQUAL  6
#define MSTYPECHGGRP 7
#define MSTYPEMAIN   8
#define MSTYPEDEFINE 9
#define MSTYPEPAIR   10
#define MSTYPEPLUS   11
#define MSTYPEMINUS  12
#define MSTYPEAND    13
#define MSTYPEOR     14
#define MSTYPEGT     15
#define MSTYPELT     16
#define MSTYPEGE     17
#define MSTYPELE     18
#define MSTYPEGETPBP 19
#define MSTYPESETPBP 20

struct _mswork_pair{
  MSWORKEXP* exp1;
  MSWORKEXP* exp2;
};
typedef struct _mswork_pair MSWORKPAIR;
typedef struct _mswork_pair MSWORKEQUAL;
typedef struct _mswork_pair MSWORKMAIN;
typedef struct _mswork_pair MSWORKPLUS;
typedef struct _mswork_pair MSWORKMINUS;
typedef struct _mswork_pair MSWORKAND;
typedef struct _mswork_pair MSWORKOR;
typedef struct _mswork_pair MSWORKGT;
typedef struct _mswork_pair MSWORKLT;
typedef struct _mswork_pair MSWORKGE;
typedef struct _mswork_pair MSWORKLE;


struct _mswork_number{
  int number;
};
typedef struct _mswork_number MSWORKNUMBER;

struct _mswork_symbol{
  int num;
};
typedef struct _mswork_symbol MSWORKSYMBOL;

struct _mswork_begin{
  int num;
  MSWORKEXP** exps;
};
typedef struct _mswork_begin MSWORKBEGIN;

struct _mswork_cond{
  int num;
  MSWORKPAIR** exps;
};
typedef struct _mswork_cond MSWORKCOND;

struct _mswork_setq{
  int num;
  MSWORKEXP* exp;
};
typedef struct _mswork_setq MSWORKSETQ;

struct _mswork_rand{
  MSWORKEXP* exp;
};
typedef struct _mswork_rand MSWORKRAND;

struct _mswork_chggrp{
  MSWORKEXP* exp;
};
typedef struct _mswork_chggrp MSWORKCHGGRP;

struct _mswork_setpbp{
  MSWORKEXP* exp;
};
typedef struct _mswork_setpbp MSWORKSETPBP;


int ms_init(const char* src);
void ms_trash();
int ms_run();

#endif /* __mindscript_h__ */
