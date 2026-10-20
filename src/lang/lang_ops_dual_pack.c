/* CubalC lang — lang_ops_dual_pack.c
 * DUAL pack/unpack plane: DLO4/DPACK4…DUNPACK32 (G peel from dual).
 * Mesh vote G_split_dual_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_pack.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_dual_try_pack(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 dual-stack nibble plane: DLO4 · DHI4 · DPACK4 */
  if (kw(&L->cur,"DLO4")||kw(&L->cur,"2LO4")||kw(&L->cur,"S2LO4")||
      kw(&L->cur,"STACK2LO4")||kw(&L->cur,"PAIRLO4")||kw(&L->cur,"DLOWN")||
      kw(&L->cur,"2LOWN")||kw(&L->cur,"DNIBLO")||kw(&L->cur,"2NIBLO")||
      kw(&L->cur,"DHI4")||kw(&L->cur,"2HI4")||kw(&L->cur,"S2HI4")||
      kw(&L->cur,"STACK2HI4")||kw(&L->cur,"PAIRHI4")||kw(&L->cur,"DHIN")||
      kw(&L->cur,"2HIN")||kw(&L->cur,"DNIBHI")||kw(&L->cur,"2NIBHI")){
    /* a b → lo4/hi4 nibble of each (bits 0..3 / 4..7) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_lo = (strcmp(op,"DLO4")==0 || strcmp(op,"2LO4")==0 || strcmp(op,"S2LO4")==0 ||
                 strcmp(op,"STACK2LO4")==0 || strcmp(op,"PAIRLO4")==0 ||
                 strcmp(op,"DLOWN")==0 || strcmp(op,"2LOWN")==0 ||
                 strcmp(op,"DNIBLO")==0 || strcmp(op,"2NIBLO")==0);
    long x, y;
    if (is_lo){
      x = a & 0xFL;
      y = b & 0xFL;
    } else {
      x = (a >> 4) & 0xFL;
      y = (b >> 4) & 0xFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPACK4")||kw(&L->cur,"2PACK4")||kw(&L->cur,"S2PACK4")||
      kw(&L->cur,"STACK2PACK4")||kw(&L->cur,"PAIRPACK4")||kw(&L->cur,"DPACKN")||
      kw(&L->cur,"2PACKN")){
    /* a b c d → ((a&0xF)<<4)|(c&0xF)  ((b&0xF)<<4)|(d&0xF) — hi,lo nibbles → byte */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = ((a & 0xFL) << 4) | (c & 0xFL);
    long y = ((b & 0xFL) << 4) | (d & 0xFL);
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack nibble unpack: DUNPACK4 (inverse of DPACK4) */
  if (kw(&L->cur,"DUNPACK4")||kw(&L->cur,"2UNPACK4")||kw(&L->cur,"S2UNPACK4")||
      kw(&L->cur,"STACK2UNPACK4")||kw(&L->cur,"PAIRUNPACK4")||kw(&L->cur,"DUNPACKN")||
      kw(&L->cur,"2UNPACKN")||kw(&L->cur,"DNIBSPLIT")||kw(&L->cur,"2NIBSPLIT")){
    /* x y → (x>>4)&0xF  (y>>4)&0xF  x&0xF  y&0xF  — hi,hi,lo,lo so DPACK4 restores */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    long ha = (x >> 4) & 0xFL;
    long hb = (y >> 4) & 0xFL;
    long la = x & 0xFL;
    long lb = y & 0xFL;
    vm->stack[vm->sp++] = ha;
    vm->stack[vm->sp++] = hb;
    vm->stack[vm->sp++] = la;
    vm->stack[vm->sp++] = lb;
    var_set_num(vm,"LAST_N",lb); vm->last_n=lb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack control-word pack: DLO8 · DHI8 · DPACK8 */
  if (kw(&L->cur,"DLO8")||kw(&L->cur,"2LO8")||kw(&L->cur,"S2LO8")||
      kw(&L->cur,"STACK2LO8")||kw(&L->cur,"PAIRLO8")||kw(&L->cur,"DLOWB")||
      kw(&L->cur,"2LOWB")||
      kw(&L->cur,"DHI8")||kw(&L->cur,"2HI8")||kw(&L->cur,"S2HI8")||
      kw(&L->cur,"STACK2HI8")||kw(&L->cur,"PAIRHI8")||kw(&L->cur,"DHIB")||
      kw(&L->cur,"2HIB")){
    /* a b → lo8/hi8 of each */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_lo = (strcmp(op,"DLO8")==0 || strcmp(op,"2LO8")==0 || strcmp(op,"S2LO8")==0 ||
                 strcmp(op,"STACK2LO8")==0 || strcmp(op,"PAIRLO8")==0 ||
                 strcmp(op,"DLOWB")==0 || strcmp(op,"2LOWB")==0);
    long x, y;
    if (is_lo){
      x = a & 0xFFL;
      y = b & 0xFFL;
    } else {
      x = (a >> 8) & 0xFFL;
      y = (b >> 8) & 0xFFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPACK8")||kw(&L->cur,"2PACK8")||kw(&L->cur,"S2PACK8")||
      kw(&L->cur,"STACK2PACK8")||kw(&L->cur,"PAIRPACK8")||kw(&L->cur,"DPACKB")||
      kw(&L->cur,"2PACKB")){
    /* a b c d → ((a&0xFF)<<8)|(c&0xFF)  ((b&0xFF)<<8)|(d&0xFF)  — hi,lo bytes */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = ((a & 0xFFL) << 8) | (c & 0xFFL);
    long y = ((b & 0xFFL) << 8) | (d & 0xFFL);
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack control-word halfpack: DLO16 · DHI16 · DPACK16 */
  if (kw(&L->cur,"DLO16")||kw(&L->cur,"2LO16")||kw(&L->cur,"S2LO16")||
      kw(&L->cur,"STACK2LO16")||kw(&L->cur,"PAIRLO16")||kw(&L->cur,"DLOWW")||
      kw(&L->cur,"2LOWW")||
      kw(&L->cur,"DHI16")||kw(&L->cur,"2HI16")||kw(&L->cur,"S2HI16")||
      kw(&L->cur,"STACK2HI16")||kw(&L->cur,"PAIRHI16")||kw(&L->cur,"DHIW")||
      kw(&L->cur,"2HIW")){
    /* a b → lo16/hi16 of each (bits 0..15 / 16..31) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_lo = (strcmp(op,"DLO16")==0 || strcmp(op,"2LO16")==0 || strcmp(op,"S2LO16")==0 ||
                 strcmp(op,"STACK2LO16")==0 || strcmp(op,"PAIRLO16")==0 ||
                 strcmp(op,"DLOWW")==0 || strcmp(op,"2LOWW")==0);
    long x, y;
    if (is_lo){
      x = a & 0xFFFFL;
      y = b & 0xFFFFL;
    } else {
      x = (a >> 16) & 0xFFFFL;
      y = (b >> 16) & 0xFFFFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPACK16")||kw(&L->cur,"2PACK16")||kw(&L->cur,"S2PACK16")||
      kw(&L->cur,"STACK2PACK16")||kw(&L->cur,"PAIRPACK16")||kw(&L->cur,"DPACKW")||
      kw(&L->cur,"2PACKW")){
    /* a b c d → ((a&0xFFFF)<<16)|(c&0xFFFF)  ((b&0xFFFF)<<16)|(d&0xFFFF) — hi,lo halfwords */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = ((a & 0xFFFFL) << 16) | (c & 0xFFFFL);
    long y = ((b & 0xFFFFL) << 16) | (d & 0xFFFFL);
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack control-word unpack: DUNPACK8 · DUNPACK16 (inverse of DPACK8/16) */
  if (kw(&L->cur,"DUNPACK8")||kw(&L->cur,"2UNPACK8")||kw(&L->cur,"S2UNPACK8")||
      kw(&L->cur,"STACK2UNPACK8")||kw(&L->cur,"PAIRUNPACK8")||kw(&L->cur,"DUNPACKB")||
      kw(&L->cur,"2UNPACKB")||kw(&L->cur,"DBYTSPLIT")||kw(&L->cur,"2BYTSPLIT")){
    /* x y → (x>>8)&0xFF  (y>>8)&0xFF  x&0xFF  y&0xFF  — hi,hi,lo,lo so DPACK8 restores */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    long ha = (x >> 8) & 0xFFL;
    long hb = (y >> 8) & 0xFFL;
    long la = x & 0xFFL;
    long lb = y & 0xFFL;
    vm->stack[vm->sp++] = ha;
    vm->stack[vm->sp++] = hb;
    vm->stack[vm->sp++] = la;
    vm->stack[vm->sp++] = lb;
    var_set_num(vm,"LAST_N",lb); vm->last_n=lb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUNPACK16")||kw(&L->cur,"2UNPACK16")||kw(&L->cur,"S2UNPACK16")||
      kw(&L->cur,"STACK2UNPACK16")||kw(&L->cur,"PAIRUNPACK16")||kw(&L->cur,"DUNPACKW")||
      kw(&L->cur,"2UNPACKW")||kw(&L->cur,"DHALFSPLIT")||kw(&L->cur,"2HALFSPLIT")){
    /* x y → (x>>16)&0xFFFF  (y>>16)&0xFFFF  x&0xFFFF  y&0xFFFF — hi,hi,lo,lo so DPACK16 restores */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    long ha = (x >> 16) & 0xFFFFL;
    long hb = (y >> 16) & 0xFFFFL;
    long la = x & 0xFFFFL;
    long lb = y & 0xFFFFL;
    vm->stack[vm->sp++] = ha;
    vm->stack[vm->sp++] = hb;
    vm->stack[vm->sp++] = la;
    vm->stack[vm->sp++] = lb;
    var_set_num(vm,"LAST_N",lb); vm->last_n=lb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack word pack: DLO32 · DHI32 · DPACK32 */
  if (kw(&L->cur,"DLO32")||kw(&L->cur,"2LO32")||kw(&L->cur,"S2LO32")||
      kw(&L->cur,"STACK2LO32")||kw(&L->cur,"PAIRLO32")||kw(&L->cur,"DLOWD")||
      kw(&L->cur,"2LOWD")||
      kw(&L->cur,"DHI32")||kw(&L->cur,"2HI32")||kw(&L->cur,"S2HI32")||
      kw(&L->cur,"STACK2HI32")||kw(&L->cur,"PAIRHI32")||kw(&L->cur,"DHID")||
      kw(&L->cur,"2HID")){
    /* a b → lo32/hi32 of each (bits 0..31 / 32..63) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_lo = (strcmp(op,"DLO32")==0 || strcmp(op,"2LO32")==0 || strcmp(op,"S2LO32")==0 ||
                 strcmp(op,"STACK2LO32")==0 || strcmp(op,"PAIRLO32")==0 ||
                 strcmp(op,"DLOWD")==0 || strcmp(op,"2LOWD")==0);
    long x, y;
    if (is_lo){
      x = (long)(unsigned int)(unsigned long)a;
      y = (long)(unsigned int)(unsigned long)b;
    } else {
      x = (long)(unsigned int)((unsigned long long)(unsigned long)a >> 32);
      y = (long)(unsigned int)((unsigned long long)(unsigned long)b >> 32);
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPACK32")||kw(&L->cur,"2PACK32")||kw(&L->cur,"S2PACK32")||
      kw(&L->cur,"STACK2PACK32")||kw(&L->cur,"PAIRPACK32")||kw(&L->cur,"DPACKDW")||
      kw(&L->cur,"2PACKDW")){
    /* a b c d → ((a&0xFFFFFFFF)<<32)|(c&0xFFFFFFFF)  same for b,d — hi,lo words */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long long xa = ((unsigned long long)(unsigned int)a << 32) |
                            (unsigned long long)(unsigned int)c;
    unsigned long long ya = ((unsigned long long)(unsigned int)b << 32) |
                            (unsigned long long)(unsigned int)d;
    long x = (long)xa;
    long y = (long)ya;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack word unpack: DUNPACK32 (inverse of DPACK32; complete DLO32/DHI32 plane) */
  if (kw(&L->cur,"DUNPACK32")||kw(&L->cur,"2UNPACK32")||kw(&L->cur,"S2UNPACK32")||
      kw(&L->cur,"STACK2UNPACK32")||kw(&L->cur,"PAIRUNPACK32")||kw(&L->cur,"DUNPACKDW")||
      kw(&L->cur,"2UNPACKDW")||kw(&L->cur,"DWORDSPLIT")||kw(&L->cur,"2WORDSPLIT")){
    /* x y → hi32(x) hi32(y) lo32(x) lo32(y) — hi,hi,lo,lo so DPACK32 restores */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    unsigned long long ux = (unsigned long long)(unsigned long)x;
    unsigned long long uy = (unsigned long long)(unsigned long)y;
    long ha = (long)(unsigned int)(ux >> 32);
    long hb = (long)(unsigned int)(uy >> 32);
    long la = (long)(unsigned int)ux;
    long lb = (long)(unsigned int)uy;
    vm->stack[vm->sp++] = ha;
    vm->stack[vm->sp++] = hb;
    vm->stack[vm->sp++] = la;
    vm->stack[vm->sp++] = lb;
    var_set_num(vm,"LAST_N",lb); vm->last_n=lb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
