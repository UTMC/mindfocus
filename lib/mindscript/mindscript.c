#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../configure.h"
#include "../../mindfocus/mindfocus.h"
#include "mindscript.h"

#ifdef DEBUG
static char* types[] = {
  "<number>", "<symbol>", "begin", "cond", "setq", "rand", "equal",
    "chg-grp", "main", "define", "pair", "plus", "minus", "and", "or",
      "greater than", "less than", "greater or equal", "less or equal",
	"get-pos-by-per", "set-pos-by-per",
};
#endif /* DEBUG */

static MSWORKEXP* symbol;
static MSWORKEXP mainexp;
static char** symboltable;
static int symbolnum;
static int nest;
static int flag = 0;

extern void chg_grp(int num);

static struct ms_stack{
  int sp;
  int max;
  MSWORKEXP** buf;
} stack;

static int
ms_exec(MSWORKEXP* exp)
{
  int i;
  int c;

#ifdef DEBUG
  printf("execute: %s\n", types[exp->type]);
#endif /* DEBUG */

  switch(exp->type){
  case MSTYPENUMBER:
#ifdef DEBUG
    printf(" ... %d\n", ((MSWORKNUMBER*)exp->ptr)->number);
#endif /* DEBUG */
    return ((MSWORKNUMBER*)exp->ptr)->number;
  case MSTYPESYMBOL:
#ifdef DEBUG
    printf(" ... #<%d>\n", ((MSWORKSYMBOL*)exp->ptr)->num);
#endif /* DEBUG */
    return ms_exec(&symbol[((MSWORKSYMBOL*)exp->ptr)->num]);
  case MSTYPEBEGIN:
    c = ((MSWORKBEGIN*)exp->ptr)->num - 1;
    for(i = 0; i < c; i++){
      ms_exec(((MSWORKBEGIN*)exp->ptr)->exps[i]);
    }
    return ms_exec(((MSWORKBEGIN*)exp->ptr)->exps[i]);
  case MSTYPECOND:
    c = ((MSWORKCOND*)exp->ptr)->num;
    for(i = 0; i < c; i++){
      if(ms_exec(((MSWORKCOND*)exp->ptr)->exps[i]->exp1))
	return ms_exec(((MSWORKCOND*)exp->ptr)->exps[i]->exp2);
    }
    break;
  case MSTYPESETQ:
    ((MSWORKNUMBER*)(symbol[((MSWORKSETQ*)exp->ptr)->num].ptr))->number
      = ms_exec(((MSWORKSETQ*)exp->ptr)->exp);
    break;
  case MSTYPERAND:
    return (int)((double)ms_exec(((MSWORKRAND*)exp->ptr)->exp) * (double)rand() / (((double)RAND_MAX)+1));
  case MSTYPECHGGRP:
    c = ms_exec(((MSWORKCHGGRP*)exp->ptr)->exp);
#ifdef DEBUG
    printf("change graphic to %d\n", c);
#endif /* DEBUG */
    chg_grp(c);
    break;
  case MSTYPEGETPBP:
    return get_pos_by_per();
  case MSTYPESETPBP:
    c = ms_exec(((MSWORKSETPBP*)exp->ptr)->exp);
    set_pos_by_per(c);
    break;
  case MSTYPEEQUAL:
    return (ms_exec(((MSWORKEQUAL*)exp->ptr)->exp1) ==
	    (ms_exec(((MSWORKEQUAL*)exp->ptr)->exp2)));
  case MSTYPEPLUS:
    return (ms_exec(((MSWORKPLUS*)exp->ptr)->exp1) +
	    (ms_exec(((MSWORKPLUS*)exp->ptr)->exp2)));
  case MSTYPEMINUS:
    return (ms_exec(((MSWORKPLUS*)exp->ptr)->exp1) -
	    (ms_exec(((MSWORKPLUS*)exp->ptr)->exp2)));
  case MSTYPEAND:
    return (ms_exec(((MSWORKPLUS*)exp->ptr)->exp1) &
	    (ms_exec(((MSWORKPLUS*)exp->ptr)->exp2)));
  case MSTYPEOR:
    return (ms_exec(((MSWORKPLUS*)exp->ptr)->exp1) |
	    (ms_exec(((MSWORKPLUS*)exp->ptr)->exp2)));
  case MSTYPEGT:
    return (ms_exec(((MSWORKPLUS*)exp->ptr)->exp1) >
	    (ms_exec(((MSWORKPLUS*)exp->ptr)->exp2)));
  case MSTYPELT:
    return (ms_exec(((MSWORKPLUS*)exp->ptr)->exp1) <
	    (ms_exec(((MSWORKPLUS*)exp->ptr)->exp2)));
  case MSTYPEGE:
    return (ms_exec(((MSWORKPLUS*)exp->ptr)->exp1) >=
	    (ms_exec(((MSWORKPLUS*)exp->ptr)->exp2)));
  case MSTYPELE:
    return (ms_exec(((MSWORKPLUS*)exp->ptr)->exp1) <=
	    (ms_exec(((MSWORKPLUS*)exp->ptr)->exp2)));
  }
  return 0;
}

static void
ms_error(char* str)
{
  printf("mindscript error: %s.\n", (str)? str: "syntax error");
}

static int
ms_null(char code)
{
  switch(code){
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    return 1;
  }
  return 0;
}

static int
ms_ret(char code)
{
  switch(code){
  case '\r':
  case '\n':
  case 0:
    return 1;
  }
  return 0;
}

static int
ms_size(char* buf)
{
  int size;

  if(('(' == *buf) || (')' == *buf))
    return 1;

  for(size = 0; !ms_null(*buf); size++, buf++)
    if((';' == *buf) || (0 == *buf) || (')' == *buf))
      return size;
  return size;
}

static int
ms_next(char** buf)
{
  for(;;){
    while(ms_null(**buf))
      *buf += 1;
    if(**buf == ';'){
      while(!ms_ret(**buf))
	*buf += 1;
    }else{
      return ms_size(*buf);
    }
  }
}

static int
ms_check(char* buf, int size, char* exp)
{
  int expsize;

  expsize = strlen(exp);
  if(size != expsize)
    return 0;
  return !strncmp(buf, exp, size);
}

static MSWORKEXP*
ms_new()
{
  return malloc(sizeof(MSWORKEXP));
}

/* 構造体たどるのが面倒なので後回し */
static void
ms_free(MSWORKEXP* exp)
{
}

static void
ms_stack_push(MSWORKEXP* exp)
{
  if(0 == stack.max){
    /* 確保してないなら確保 */
    stack.buf = malloc(sizeof(MSWORKEXP*) * 10);
    stack.max = 10;
  }
  if(stack.sp >= stack.max){
    /* いっぱいになったら拡張 */
    stack.max += 10;
    stack.buf = realloc(stack.buf, sizeof(MSWORKEXP*) * stack.max);
  }
  stack.buf[stack.sp++] = exp;
}

static MSWORKEXP*
ms_stack_pop()
{
  if(stack.sp == 0)
    return NULL;

  stack.sp--;
  return stack.buf[stack.sp];
}

static void
ms_stack_free()
{
  if(stack.buf){
    free(stack.buf);
    stack.buf = NULL;
    stack.max = stack.sp = 0;
  }
}

/**/
static int
ms_makestruct(int stack)
{
  MSWORKEXP** buf;
  int i;

  buf = alloca(sizeof(MSWORKEXP*) * stack);
  for(i = 1; i <= stack; i++)
    buf[stack-i] = ms_stack_pop();

  switch(buf[0]->type){
  case MSTYPEBEGIN:
    buf[0]->ptr = malloc(sizeof(MSWORKBEGIN));
    ((MSWORKBEGIN*)buf[0]->ptr)->num = stack - 1;
    ((MSWORKBEGIN*)buf[0]->ptr)->exps = malloc(sizeof(MSWORKEXP*) * (stack - 1));
    for(i = 1; i < stack; i++)
      ((MSWORKBEGIN*)buf[0]->ptr)->exps[i-1] = buf[i];
    break;
  case MSTYPECOND:
    buf[0]->ptr = malloc(sizeof(MSWORKCOND));
    ((MSWORKCOND*)buf[0]->ptr)->num = stack - 1;
    ((MSWORKCOND*)buf[0]->ptr)->exps = malloc(sizeof(MSWORKPAIR*) * (stack - 1));
    for(i = 1; i < stack; i++)
      ((MSWORKCOND*)buf[0]->ptr)->exps[i-1] = (MSWORKPAIR*)buf[i]->ptr;
    break;
  case MSTYPESETQ:
    if(stack != 3){
      ms_error(NULL);
      return 0;
    }
    if(buf[1]->type != MSTYPESYMBOL){
      ms_error("'setq' needs <symbol>");
      return 0;
    }
    buf[0]->ptr = malloc(sizeof(MSWORKSETQ));
    ((MSWORKSETQ*)buf[0]->ptr)->num = ((MSWORKSYMBOL*)buf[1]->ptr)->num;
    ((MSWORKSETQ*)buf[0]->ptr)->exp = buf[2];
    break;
  case MSTYPERAND:
    if(stack != 2){
      ms_error(NULL);
      return 0;
    }
    buf[0]->ptr = malloc(sizeof(MSWORKRAND));
    ((MSWORKRAND*)buf[0]->ptr)->exp = buf[1];
    break;
  case MSTYPEMAIN:
    if(stack != 3){
      ms_error(NULL);
      return 0;
    }
    buf[0]->ptr = malloc(sizeof(MSWORKMAIN));
    ((MSWORKMAIN*)buf[0]->ptr)->exp1 = buf[1];
    ((MSWORKMAIN*)buf[0]->ptr)->exp2 = buf[2];
    break;
  case MSTYPEDEFINE:
    if(stack != 3){
      ms_error(NULL);
      return 0;
    }
    if(buf[1]->type != MSTYPESYMBOL){
      ms_error("'define' needs <symbol>");
      return 0;
    }
    /* define はコンパイル時にかたがつく */
    symbol[((MSWORKSYMBOL*)buf[1]->ptr)->num].type = buf[2]->type;
    symbol[((MSWORKSYMBOL*)buf[1]->ptr)->num].ptr = buf[2]->ptr;

    /* 評価する時のためにダミーの数値を置く */
    buf[0]->ptr = malloc(sizeof(MSWORKNUMBER));
    ((MSWORKNUMBER*)buf[0]->ptr)->number = 0;
    buf[0]->type = MSTYPENUMBER;
    break;
  case MSTYPECHGGRP:
    if(stack != 2){
      ms_error(NULL);
      return 0;
    }
    buf[0]->ptr = malloc(sizeof(MSWORKCHGGRP));
    ((MSWORKCHGGRP*)buf[0]->ptr)->exp = buf[1];
    break;
  case MSTYPESETPBP:
    if(stack != 2){
      ms_error(NULL);
      return 0;
    }
    buf[0]->ptr = malloc(sizeof(MSWORKSETPBP));
    ((MSWORKSETPBP*)buf[0]->ptr)->exp = buf[1];
    break;
  case MSTYPEGETPBP:
    if(stack != 1){
      ms_error(NULL);
      return 0;
    }
    buf[0]->ptr = NULL;
    break;
  case MSTYPEPAIR:
  case MSTYPEEQUAL:
  case MSTYPEPLUS:
  case MSTYPEMINUS:
  case MSTYPEAND:
  case MSTYPEOR:
  case MSTYPEGT:
  case MSTYPELT:
  case MSTYPEGE:
  case MSTYPELE:
    if(stack != 3){
      ms_error(NULL);
      return 0;
    }
    buf[0]->ptr = malloc(sizeof(MSWORKPAIR));
    ((MSWORKPAIR*)buf[0]->ptr)->exp1 = buf[1];
    ((MSWORKPAIR*)buf[0]->ptr)->exp2 = buf[2];
    break;
  case MSTYPENUMBER:
  case MSTYPESYMBOL:
    break;
  default:
    ms_error(NULL);
    return 0;
  }
  ms_stack_push(buf[0]);

#ifdef DEBUG
    {
      for(i = 0; i<stack; i++)
	printf("%s, ", types[buf[i]->type]);
      puts("");
    }
#endif /* DEBUG */

  return 1;
}

static int
ms_defined_symbol(char* str, int size)
{
  int i;

  for(i = 0; i < symbolnum; i++){
    if(strlen(symboltable[i]) == size)
      if(!strncmp(symboltable[i], str, size))
	return i + 1;
  }
  return 0;
}

static int
ms_define_symbol(char* str, int size)
{
  if(0 == symbolnum){
    symboltable = malloc(sizeof(char*));
    symbol = malloc(sizeof(MSWORKEXP));
  }else{
    symboltable = realloc(symboltable, sizeof(char*) * (symbolnum + 1));
    symbol = realloc(symbol, sizeof(MSWORKEXP) * (symbolnum + 1));
  }

  symboltable[symbolnum] = malloc(size + 1);
  strncpy(symboltable[symbolnum], str, size);
  symboltable[symbolnum][size] = 0;
  symbolnum++;
  return symbolnum;
}

static void
ms_free_symbol()
{
  int i;
  
  for(i = 0; i < symbolnum; i++){
    free(symboltable[i]);
  }
  free(symboltable);
  symboltable = NULL;
  symbolnum = 0;
}

static int
ms_load_sub(char** buf, MSWORKEXP* exp)
{
  int size;
  int stack;
  MSWORKEXP* newexp;
  int r;

  if(exp->type == MSTYPEPAIR){
    stack = 1;
    exp->ptr = NULL;
    ms_stack_push(exp);
  }else{
    stack = 0;
  }

  for(;;){
    /* ヌル文字を飛ばして次の字句の大きさを求める。
     ポインタは次の字句の１文字目を指している */
    size = ms_next(buf);

    if(0 == size){
      /* きちんと括弧が閉じられたか */
      if(0 != nest){
	ms_error("unexpected eof");
	return 0;
      }
      /* 変換終了 */
      return 1;
    }

    if(stack == 0){
      /* 最初の要素を調べている */
      switch(**buf){
      case 'm':
	if(ms_check(*buf, size, "main")){
	  /* (main exp1 exp2) */
	  exp->type = MSTYPEMAIN;
	  break;
	}
      case 'a':
	if(ms_check(*buf, size, "and")){
	  /* (and exp1 exp2) */
	  exp->type = MSTYPEAND;
	  break;
	}
      case 'b':
	if(ms_check(*buf, size, "begin")){
	  /* (begin a1 a2 ... an) */
	  exp->type = MSTYPEBEGIN;
	  break;
	}
      case 'c':
	if(ms_check(*buf, size, "cond")){
	  /* (cond (exp1 exp1') ...) */
	  exp->type = MSTYPECOND;
	  break;
	}else if(ms_check(*buf, size, "chg-grp")){
	  /* (chg-grp n) */
	  exp->type = MSTYPECHGGRP;
	  break;
	}
      case 'd':
	if(ms_check(*buf, size, "define")){
	  /* (define var n) */
	  exp->type = MSTYPEDEFINE;
	  break;
	}
      case 'g':
	if(ms_check(*buf, size, "get-pos-by-per")){
	  /* (get-pos-by-per) */
	  exp->type = MSTYPEGETPBP;
	  break;
	}
      case 'o':
	if(ms_check(*buf, size, "or")){
	  /* (or exp1 exp2) */
	  exp->type = MSTYPEOR;
	  break;
	}
      case 'r':
	if(ms_check(*buf, size, "rand")){
	  /* (rand n) */
	  exp->type = MSTYPERAND;
	  break;
	}
      case 's':
	if(ms_check(*buf, size, "setq")){
	  /* (setq var n) */
	  exp->type = MSTYPESETQ;
	  break;
	}else if(ms_check(*buf, size, "set-pos-by-per")){
	  /* (set-pos-by-per pos) */
	  exp->type = MSTYPESETPBP;
	  break;
	}
      case '=':
	if(ms_check(*buf, size, "=")){
	  /* (= exp1 exp2) */
	  exp->type = MSTYPEEQUAL;
	  break;
	}
      case '+':
	if(ms_check(*buf, size, "+")){
	  /* (+ exp1 exp2) */
	  exp->type = MSTYPEPLUS;
	  break;
	}
      case '-':
	if(ms_check(*buf, size, "-")){
	  /* (- exp1 exp2) */
	  exp->type = MSTYPEMINUS;
	  break;
	}
      case '>':
	if(ms_check(*buf, size, ">")){
	  /* (> exp1 exp2) */
	  exp->type = MSTYPEGT;
	  break;
	}else if(ms_check(*buf, size, ">=")){
	  /* (>= exp1 exp2) */
	  exp->type = MSTYPEGE;
	  break;
	}
      case '<':
	if(ms_check(*buf, size, "<")){
	  /* (< exp1 exp2) */
	  exp->type = MSTYPELT;
	  break;
	}else if(ms_check(*buf, size, "<=")){
	  /* (<= exp1 exp2) */
	  exp->type = MSTYPELE;
	  break;
	}
      default:
	ms_error(NULL);
	return 0;
      }
      exp->ptr = NULL;
      ms_stack_push(exp);
      stack++;
      *buf += size;

    }else{
      /* ２つ目以降の要素を調べている */
      switch(**buf){
      case '(':
	if(size != 1){
	  ms_error(NULL);
	  return 0;
	}
	newexp = ms_new();
	*buf += size;
	nest++;
        if(exp->type == MSTYPECOND)
	  newexp->type = MSTYPEPAIR;
	else
	  newexp->type = MSTYPENULL;
	if(ms_load_sub(buf, newexp)){
	  stack++;
	}else{
	  ms_free(newexp);
	  return 0;
	}
	break;
      case ')':
	if(stack == 0){
	  ms_error(NULL);
	  return 0;
	}
	if(size != 1){
	  ms_error(NULL);
	  return 0;
	}
	*buf += 1;
	nest--;
	return ms_makestruct(stack);
      default:
	if(isdigit(**buf)){
	  /* 数値 */
	  newexp = ms_new();
	  newexp->type = MSTYPENUMBER;
	  newexp->ptr = malloc(sizeof(MSWORKNUMBER));
	  ((MSWORKNUMBER*)newexp->ptr)->number = atoi(*buf);
	  ms_stack_push(newexp);
	  stack++;
	  /* チェックがあまい。10hoge などを 10 と認識する */
	  *buf += size;
	}else{
	  /* シンボル */
	  r = ms_defined_symbol(*buf, size);
	  if(!r)
	    /* 未定義のシンボル */
	    r = ms_define_symbol(*buf, size);
	  newexp = ms_new();
	  newexp->type = MSTYPESYMBOL;
	  newexp->ptr = malloc(sizeof(MSWORKSYMBOL));
	  ((MSWORKSYMBOL*)newexp->ptr)->num = r - 1;
	  ms_stack_push(newexp);
	  stack++;
	  *buf += size;
	}
      }
    }
  }
}

static int
ms_load(char* buf)
{
  int size;

  nest  = 1;
  symbolnum = 0;
  symboltable = NULL;
  symbol = NULL;
  stack.sp = 0;
  stack.max = 0;
  stack.buf = NULL;

  size = ms_next(&buf);
  if(size != 1){
    ms_error(NULL);
    return 0;
  }
  if(*buf != '('){
    ms_error(NULL);
    return 0;
  }

  buf += size;
  mainexp.type = MSTYPENULL;
  mainexp.ptr = NULL;
  ms_load_sub(&buf, &mainexp);
  ms_stack_free();
  ms_free_symbol();

#ifdef DEBUG
  printf("start from %s\n", types[mainexp.type]);
  printf("first exp is %s\n", types[((MSWORKMAIN*)mainexp.ptr)->exp1->type]);
  printf("second exp is %s\n", types[((MSWORKMAIN*)mainexp.ptr)->exp2->type]);
#endif /* DEBUG */

  if(mainexp.ptr)
     return 1;
  return 0;
}


int
ms_init(const char* src)
{
  FILE* fp;
  long size;
  char* buf;

  if(NULL == src){
    flag = 0;
    return 1;
  }

  fp = fopen(src, "rb");
  if(NULL == fp){
    printf("mindfocus: can not open script '%s'\n", src);
    return 0;
  }

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  rewind(fp);

  buf = alloca(size + 1);
  fread(buf, size, 1, fp);
  buf[size] = 0;
  fclose(fp);

  flag = ms_load(buf);
  return flag;
}

void
ms_trash()
{
  flag = 0;
  ms_free(&mainexp);
}

int
ms_run()
{
  switch(flag){
  case 0:
    return 0;
  case 1:
    flag = 2;
    return ms_exec(((MSWORKMAIN*)mainexp.ptr)->exp1);
  case 2:
    return ms_exec(((MSWORKMAIN*)mainexp.ptr)->exp2);
  }
  return 0;
}
