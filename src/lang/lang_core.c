/* CubalC lang — lang_core.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"


/* CubalC lang — place/plug/pulse/flow/look. Grammar = ops, not prose. */

/* types: include/lang/cubalc_lang_internal.h */

void cubalc_lang_fail(VM *vm, const char *msg) {
  if (vm->fatal) return;
  vm->fatal = 1;
  snprintf(vm->err, sizeof vm->err, "%s", msg);
  /* Sticky agent-readable error (survives later LAST overwrites). */
  var_set_str(vm, "ERR", msg ? msg : "fail");
  var_set_str(vm, "LAST_ERR", msg ? msg : "fail");
  if (vm->res) {
    vm->res->ok = 0;
    snprintf(vm->res->err, sizeof vm->res->err, "%s", msg);
  }
}
void cubalc_lang_fail_at(VM *vm, int line, const char *msg) {
  char ebuf[192];
  if (!msg) msg = "fail";
  if (line > 0 && !strstr(msg, "line "))
    snprintf(ebuf, sizeof ebuf, "%s line %d", msg, line);
  else
    snprintf(ebuf, sizeof ebuf, "%s", msg);
  cubalc_lang_fail(vm, ebuf);
}
void cubalc_lang_bump(VM *vm) { if (vm->res) vm->res->stmts++; }

/* Usability: wall-clock run budget (CUBALC_RUN_TIMEOUT / cubalc run -T).
 * Agents bound runaway loops without shell timeout(1). */
long cubalc_lang_mono_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    return 0;
  return (long)ts.tv_sec * 1000L + (long)(ts.tv_nsec / 1000000L);
}
long cubalc_lang_timeout_remain_ms(VM *vm) {
  long now, left;
  if (!vm || vm->run_deadline_ms <= 0)
    return -1; /* unlimited */
  now = cubalc_lang_mono_ms();
  left = vm->run_deadline_ms - now;
  return left < 0 ? 0 : left;
}
int cubalc_lang_check_timeout(VM *vm, int line) {
  char msg[192];
  long left;
  if (!vm || vm->fatal || vm->run_deadline_ms <= 0)
    return 0;
  left = cubalc_lang_timeout_remain_ms(vm);
  if (left > 0)
    return 0;
  if (vm->res) {
    vm->res->timed_out = 1;
    if (vm->run_timeout_ms > 0)
      vm->res->timeout_ms = (int)vm->run_timeout_ms;
  }
  if (line > 0)
    snprintf(msg, sizeof msg,
             "TIMEOUT line %d: exceeded CUBALC_RUN_TIMEOUT %ldms — "
             "raise -T / CUBALC_RUN_TIMEOUT or lean the loop",
             line, vm->run_timeout_ms > 0 ? vm->run_timeout_ms : 0L);
  else
    snprintf(msg, sizeof msg,
             "TIMEOUT: exceeded CUBALC_RUN_TIMEOUT %ldms — "
             "raise -T / CUBALC_RUN_TIMEOUT or lean the loop",
             vm->run_timeout_ms > 0 ? vm->run_timeout_ms : 0L);
  cubalc_lang_fail(vm, msg);
  var_set_num(vm, "TIMED_OUT", 1);
  var_set_num(vm, "TIMEOUT_MS", vm->run_timeout_ms);
  var_set_num(vm, "OK", 0);
  return 1;
}
int cubalc_lang_kw(const Tok *t, const char *k) {
  return t->kind == TK_IDENT && strcasecmp(t->text, k) == 0;
}

void cubalc_lang_lex_skip(Lex *L) {
  while (L->i < L->n) {
    char c = L->s[L->i];
    if (c==' '||c=='\t'||c=='\r') { L->i++; continue; }
    if (c=='#' || (c=='/' && L->i+1<L->n && L->s[L->i+1]=='/')) {
      while (L->i < L->n && L->s[L->i] != '\n') L->i++;
      continue;
    }
    break;
  }
}
static int is_id0(int c){ return isalpha(c)||c=='_'; }
static int is_id(int c){ return isalnum(c)||c=='_'||c=='-'||c=='.'; }

void cubalc_lang_lex_next(Lex *L) {
  lex_skip(L);
  L->tok_off = L->i; /* start of current token (for FN body capture) */
  L->cur.line = L->line;
  L->cur.text[0]=0; L->cur.num=0;
  if (L->i >= L->n) { L->cur.kind = TK_EOF; return; }
  char c = L->s[L->i];
  if (c=='\n'){ L->i++; L->line++; L->cur.kind=TK_NL; return; }
  if (c=='['){ L->i++; L->cur.kind=TK_LBRACK; return; }
  if (c==']'){ L->i++; L->cur.kind=TK_RBRACK; return; }
  if (c=='~'){ L->i++; L->cur.kind=TK_TILDE; return; }
  if (c=='?'){ L->i++; L->cur.kind=TK_QMARK; return; }
  if (c==':'){ L->i++; L->cur.kind=TK_COLON; return; }
  if (c=='|'){ L->i++; L->cur.kind=TK_PIPE; return; }
  if (c=='!'){
    L->i++;
    if (L->i<L->n && L->s[L->i]=='='){ L->i++; L->cur.kind=TK_NE; return; }
    L->cur.kind=TK_BANG; return;
  }
  if (c=='"'){
    L->i++; size_t k=0;
    while (L->i<L->n && L->s[L->i]!='"'){
      char ch = L->s[L->i];
      if (ch=='\\' && L->i+1<L->n){
        char e = L->s[L->i+1];
        L->i += 2;
        if (e=='n') ch='\n';
        else if (e=='t') ch='\t';
        else if (e=='r') ch='\r';
        else if (e=='"') ch='"';
        else if (e=='\\') ch='\\';
        else ch=e;
        if (k+1<sizeof L->cur.text) L->cur.text[k++]=ch;
        continue;
      }
      if (ch=='\n') L->line++;
      if (k+1<sizeof L->cur.text) L->cur.text[k++]=ch;
      L->i++;
    }
    L->cur.text[k]=0; if (L->i<L->n) L->i++;
    L->cur.kind=TK_STR; return;
  }
  if (isdigit((unsigned char)c)){
    char b[64]; size_t k=0;
    /* 0x… / 0X… hex integer literals (universal data-path) */
    if (c=='0' && L->i+1<L->n && (L->s[L->i+1]=='x' || L->s[L->i+1]=='X')){
      b[k++]=L->s[L->i++]; /* 0 */
      b[k++]=L->s[L->i++]; /* x */
      while (L->i<L->n && isxdigit((unsigned char)L->s[L->i])){
        if (k+1<sizeof b) b[k++]=L->s[L->i];
        L->i++;
      }
      b[k]=0;
      L->cur.num = strtoul(b, NULL, 16);
      L->cur.kind = TK_NUM;
      snprintf(L->cur.text, sizeof L->cur.text, "%s", b);
      return;
    }
    while (L->i<L->n && isdigit((unsigned char)L->s[L->i])){
      if (k+1<sizeof b) b[k++]=L->s[L->i]; L->i++;
    }
    /* 2DUP / 2DROP / 2SWAP / 2OVER / 2ROT / 2RROT / 2NIP / 2TUCK — Forth double ops (digit-8)
     * 3DUP / 3DROP / 3SWAP / 3OVER / 3ROT / 3NIP / 3TUCK — triple (digit-8)
     * 4DUP / 4DROP / 4SWAP / 4NIP / 4ROT / 4RROT / 4OVER / 4TUCK — quadruple
     * 5DUP / 5DROP / 5SWAP — quintuple depth (digit-8)
     * 6DUP / 6DROP — sextuple depth (digit-8)
     * 7DUP / 7DROP / 7SWAP / 7NIP / 7ROT / 7RROT / 7OVER / 7TUCK — septuple
     * 8DUP / 8DROP / 8SWAP / 8NIP / 8ROT / 8RROT / 8OVER / 8TUCK — octuple
     * 9DUP / 9DROP / 9SWAP / 9NIP / 9ROT / 9RROT / 9OVER / 9TUCK — nonuple */
    if (k==1 && (b[0]=='2' || b[0]=='3' || b[0]=='4' || b[0]=='5' || b[0]=='6' || b[0]=='7' || b[0]=='8' || b[0]=='9') && L->i<L->n && isalpha((unsigned char)L->s[L->i])){
      size_t j = L->i;
      char tail[16]; size_t t=0;
      /* alnum: allow 2ISPOW2 / 2POW2P style (digits inside tail) */
      while (j<L->n && (isalpha((unsigned char)L->s[j]) || isdigit((unsigned char)L->s[j])) && t+1<sizeof tail)
        tail[t++]=L->s[j++];
      tail[t]=0;
      int ok = 0;
      if (b[0]=='2'){
        if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
            strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"OVER")==0 ||
            strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"RROT")==0 ||
            strcasecmp(tail,"NIP")==0 ||
            strcasecmp(tail,"TUCK")==0 ||
            strcasecmp(tail,"ADD")==0 || strcasecmp(tail,"SUB")==0 ||
            strcasecmp(tail,"ADDN")==0 || strcasecmp(tail,"SUBN")==0 ||
            strcasecmp(tail,"MULN")==0 || strcasecmp(tail,"DIVN")==0 ||
            strcasecmp(tail,"MODN")==0 || strcasecmp(tail,"QUOTN")==0 ||
            strcasecmp(tail,"REMN")==0 ||
            strcasecmp(tail,"ADDIMM")==0 || strcasecmp(tail,"SUBIMM")==0 ||
            strcasecmp(tail,"MULIMM")==0 || strcasecmp(tail,"DIVIMM")==0 ||
            strcasecmp(tail,"MODIMM")==0 ||
            strcasecmp(tail,"ADDC")==0 || strcasecmp(tail,"ADC")==0 ||
            strcasecmp(tail,"SUBB")==0 || strcasecmp(tail,"SBB")==0 ||
            strcasecmp(tail,"ADDC2")==0 || strcasecmp(tail,"ADC2")==0 ||
            strcasecmp(tail,"SUBB2")==0 || strcasecmp(tail,"SBB2")==0 ||
            strcasecmp(tail,"ADDCST")==0 || strcasecmp(tail,"SUBBST")==0 ||
            strcasecmp(tail,"ADDOVF")==0 || strcasecmp(tail,"ADDOVER")==0 ||
            strcasecmp(tail,"SUBOVF")==0 || strcasecmp(tail,"SUBOVER")==0 ||
            strcasecmp(tail,"MULOVF")==0 || strcasecmp(tail,"MULOVER")==0 ||
            strcasecmp(tail,"UADDOVF")==0 || strcasecmp(tail,"UADDOVER")==0 ||
            strcasecmp(tail,"USUBOVF")==0 || strcasecmp(tail,"USUBOVER")==0 ||
            strcasecmp(tail,"UMULOVF")==0 || strcasecmp(tail,"UMULOVER")==0 ||
            strcasecmp(tail,"MUL")==0 || strcasecmp(tail,"DIV")==0 ||
            strcasecmp(tail,"MOD")==0 || strcasecmp(tail,"MIN")==0 ||
            strcasecmp(tail,"MAX")==0 ||
            strcasecmp(tail,"MINN")==0 || strcasecmp(tail,"MAXN")==0 ||
            strcasecmp(tail,"MINIMM")==0 || strcasecmp(tail,"MAXIMM")==0 ||
            strcasecmp(tail,"CLAMPN")==0 || strcasecmp(tail,"CLAMPIMM")==0 ||
            strcasecmp(tail,"EQN")==0 || strcasecmp(tail,"LTN")==0 ||
            strcasecmp(tail,"GTN")==0 || strcasecmp(tail,"EQIMM")==0 ||
            strcasecmp(tail,"LTIMM")==0 || strcasecmp(tail,"GTIMM")==0 ||
            strcasecmp(tail,"NEN")==0 || strcasecmp(tail,"NEIMM")==0 ||
            strcasecmp(tail,"LENN")==0 || strcasecmp(tail,"LEQN")==0 ||
            strcasecmp(tail,"LEIMM")==0 || strcasecmp(tail,"GENN")==0 ||
            strcasecmp(tail,"GEQN")==0 || strcasecmp(tail,"GEIMM")==0 ||
            strcasecmp(tail,"AND")==0 ||
            strcasecmp(tail,"ANDI")==0 || strcasecmp(tail,"ANDIMM")==0 ||
            strcasecmp(tail,"ORI")==0 || strcasecmp(tail,"ORIMM")==0 ||
            strcasecmp(tail,"XORI")==0 || strcasecmp(tail,"XORIMM")==0 ||
            strcasecmp(tail,"NANDI")==0 || strcasecmp(tail,"NANDIMM")==0 ||
            strcasecmp(tail,"NORI")==0 || strcasecmp(tail,"NORIMM")==0 ||
            strcasecmp(tail,"XNORI")==0 || strcasecmp(tail,"XNORIMM")==0 ||
            strcasecmp(tail,"EQUIVI")==0 ||
            strcasecmp(tail,"MADD")==0 || strcasecmp(tail,"FMA")==0 ||
            strcasecmp(tail,"MULADD")==0 ||
            strcasecmp(tail,"MULHI")==0 || strcasecmp(tail,"MULH")==0 ||
            strcasecmp(tail,"HMUL")==0 ||
            strcasecmp(tail,"UDIV")==0 || strcasecmp(tail,"UMOD")==0 ||
            strcasecmp(tail,"UREM")==0 || strcasecmp(tail,"UDIVIDE")==0 ||
            strcasecmp(tail,"UMIN")==0 || strcasecmp(tail,"UMAX")==0 ||
            strcasecmp(tail,"UMINN")==0 || strcasecmp(tail,"UMAXN")==0 ||
            strcasecmp(tail,"UMINIMM")==0 || strcasecmp(tail,"UMAXIMM")==0 ||
            strcasecmp(tail,"ULT")==0 || strcasecmp(tail,"ULE")==0 ||
            strcasecmp(tail,"UGT")==0 || strcasecmp(tail,"UGE")==0 ||
            strcasecmp(tail,"ULTN")==0 || strcasecmp(tail,"ULEN")==0 ||
            strcasecmp(tail,"UGTN")==0 || strcasecmp(tail,"UGEN")==0 ||
            strcasecmp(tail,"ULEQN")==0 || strcasecmp(tail,"UGEQN")==0 ||
            strcasecmp(tail,"ULTIMM")==0 || strcasecmp(tail,"ULEIMM")==0 ||
            strcasecmp(tail,"UGTIMM")==0 || strcasecmp(tail,"UGEIMM")==0 ||
            strcasecmp(tail,"INV")==0 || strcasecmp(tail,"RECIP")==0 ||
            strcasecmp(tail,"NORM100")==0 || strcasecmp(tail,"ENORM")==0 ||
            strcasecmp(tail,"NORME")==0 ||
            strcasecmp(tail,"OR")==0 || strcasecmp(tail,"XOR")==0 ||
            strcasecmp(tail,"NEG")==0 || strcasecmp(tail,"ABS")==0 ||
            strcasecmp(tail,"NEGC2")==0 || strcasecmp(tail,"NEGC")==0 ||
            strcasecmp(tail,"NEGCN")==0 || strcasecmp(tail,"NEGCC")==0 ||
            strcasecmp(tail,"NEGCIMM")==0 || strcasecmp(tail,"NEGCF")==0 ||
            strcasecmp(tail,"COMADC")==0 || strcasecmp(tail,"NEGADC")==0 ||
            strcasecmp(tail,"COMADCN")==0 || strcasecmp(tail,"COMADCC")==0 ||
            strcasecmp(tail,"EQ")==0 || strcasecmp(tail,"NE")==0 ||
            strcasecmp(tail,"LT")==0 || strcasecmp(tail,"LE")==0 ||
            strcasecmp(tail,"GT")==0 || strcasecmp(tail,"GE")==0 ||
            strcasecmp(tail,"CMP")==0 || strcasecmp(tail,"ICMP")==0 ||
            strcasecmp(tail,"CMP3")==0 || strcasecmp(tail,"UCMP")==0 ||
            strcasecmp(tail,"UCMP3")==0 ||
            strcasecmp(tail,"GCD")==0 || strcasecmp(tail,"LCM")==0 ||
            strcasecmp(tail,"GCDN")==0 || strcasecmp(tail,"LCMN")==0 ||
            strcasecmp(tail,"GCDIMM")==0 || strcasecmp(tail,"LCMIMM")==0 ||
            strcasecmp(tail,"COPRIMEN")==0 || strcasecmp(tail,"ISCOPRIMEN")==0 ||
            strcasecmp(tail,"COPRIMEIMM")==0 ||
            strcasecmp(tail,"POWN")==0 || strcasecmp(tail,"POWIMM")==0 ||
            strcasecmp(tail,"POWERN")==0 ||
            strcasecmp(tail,"POW")==0 ||
            strcasecmp(tail,"SHL")==0 || strcasecmp(tail,"SHR")==0 ||
            strcasecmp(tail,"SAR")==0 ||
            strcasecmp(tail,"SHLN")==0 || strcasecmp(tail,"SHRN")==0 ||
            strcasecmp(tail,"SARN")==0 || strcasecmp(tail,"ASHRN")==0 ||
            strcasecmp(tail,"SHL4")==0 || strcasecmp(tail,"SHR4")==0 ||
            strcasecmp(tail,"SAR4")==0 || strcasecmp(tail,"ASHR4")==0 ||
            strcasecmp(tail,"SHL8")==0 || strcasecmp(tail,"SHR8")==0 ||
            strcasecmp(tail,"SAR8")==0 || strcasecmp(tail,"ASHR8")==0 ||
            strcasecmp(tail,"SHL16")==0 || strcasecmp(tail,"SHR16")==0 ||
            strcasecmp(tail,"SAR16")==0 || strcasecmp(tail,"ASHR16")==0 ||
            strcasecmp(tail,"UNPACK4")==0 || strcasecmp(tail,"UNPACKN")==0 ||
            strcasecmp(tail,"NIBSPLIT")==0 ||
            strcasecmp(tail,"UNPACK8")==0 || strcasecmp(tail,"UNPACKB")==0 ||
            strcasecmp(tail,"BYTSPLIT")==0 ||
            strcasecmp(tail,"UNPACK16")==0 || strcasecmp(tail,"UNPACKW")==0 ||
            strcasecmp(tail,"UNPACK32")==0 || strcasecmp(tail,"UNPACKDW")==0 ||
            strcasecmp(tail,"HALFSPLIT")==0 ||
            strcasecmp(tail,"SHLC")==0 || strcasecmp(tail,"SHRC")==0 ||
            strcasecmp(tail,"SHLCY")==0 || strcasecmp(tail,"SHRCY")==0 ||
            strcasecmp(tail,"SQR")==0 || strcasecmp(tail,"ISQRT")==0 ||
            strcasecmp(tail,"SQRT")==0 || strcasecmp(tail,"COPRIME")==0 ||
            strcasecmp(tail,"DIVCEIL")==0 || strcasecmp(tail,"CEILDIV")==0 ||
            strcasecmp(tail,"DIVFLOOR")==0 || strcasecmp(tail,"FLOORDIV")==0 ||
            strcasecmp(tail,"ADDMOD")==0 || strcasecmp(tail,"SUBMOD")==0 ||
            strcasecmp(tail,"MULMOD")==0 || strcasecmp(tail,"POWMOD")==0 ||
            strcasecmp(tail,"MODINV")==0 || strcasecmp(tail,"INVMOD")==0 ||
            strcasecmp(tail,"MODDIV")==0 || strcasecmp(tail,"DIVMODM")==0 ||
            strcasecmp(tail,"MODINVN")==0 || strcasecmp(tail,"INVMODN")==0 ||
            strcasecmp(tail,"POWMODN")==0 || strcasecmp(tail,"POWMODIMM")==0 ||
            strcasecmp(tail,"MODDIVN")==0 || strcasecmp(tail,"DIVMODMN")==0 ||
            strcasecmp(tail,"MODINVIMM")==0 || strcasecmp(tail,"MODDIVIMM")==0 ||
            strcasecmp(tail,"SIGN")==0 || strcasecmp(tail,"CLAMP")==0 ||
            strcasecmp(tail,"SEL")==0 || strcasecmp(tail,"MUX")==0 ||
            strcasecmp(tail,"SEL2")==0 || strcasecmp(tail,"MUX2")==0 ||
            strcasecmp(tail,"LAND")==0 || strcasecmp(tail,"LOR")==0 ||
            strcasecmp(tail,"LXOR")==0 || strcasecmp(tail,"IMP")==0 ||
            strcasecmp(tail,"IMPLY")==0 || strcasecmp(tail,"IMPLIES")==0 ||
            strcasecmp(tail,"NIPIF")==0 || strcasecmp(tail,"KEEPIF")==0 ||
            strcasecmp(tail,"CNIP")==0 || strcasecmp(tail,"QKEEP")==0 ||
            strcasecmp(tail,"SWAPIF")==0 || strcasecmp(tail,"QSWAP")==0 ||
            strcasecmp(tail,"CSWAP")==0 ||
            strcasecmp(tail,"DROPIF")==0 || strcasecmp(tail,"QDROP")==0 ||
            strcasecmp(tail,"DROPWHEN")==0 ||
            strcasecmp(tail,"DUPIF")==0 || strcasecmp(tail,"2QDUP")==0 ||
            strcasecmp(tail,"DUPWHEN")==0 ||
            strcasecmp(tail,"OVERIF")==0 || strcasecmp(tail,"QOVER")==0 ||
            strcasecmp(tail,"TUCKIF")==0 || strcasecmp(tail,"QTUCK")==0 ||
            strcasecmp(tail,"ROTIF")==0 || strcasecmp(tail,"QROT")==0 ||
            strcasecmp(tail,"RROTIF")==0 || strcasecmp(tail,"QRROT")==0 ||
            strcasecmp(tail,"SHGATE")==0 || strcasecmp(tail,"GATES")==0 ||
            strcasecmp(tail,"ZEROUNLESS")==0 || strcasecmp(tail,"ZEROIF")==0 ||
            strcasecmp(tail,"ZAPIF")==0 || strcasecmp(tail,"QZERO")==0 ||
            strcasecmp(tail,"INC")==0 || strcasecmp(tail,"DEC")==0 ||
            strcasecmp(tail,"NOT")==0 || strcasecmp(tail,"EQZ")==0 ||
            strcasecmp(tail,"NEZ")==0 ||
            strcasecmp(tail,"ROL")==0 || strcasecmp(tail,"ROR")==0 ||
            strcasecmp(tail,"ROLN")==0 || strcasecmp(tail,"RORN")==0 ||
            strcasecmp(tail,"ROTLN")==0 || strcasecmp(tail,"ROTRN")==0 ||
            strcasecmp(tail,"ROL4")==0 || strcasecmp(tail,"ROR4")==0 ||
            strcasecmp(tail,"ROTL4")==0 || strcasecmp(tail,"ROTR4")==0 ||
            strcasecmp(tail,"ROL8")==0 || strcasecmp(tail,"ROR8")==0 ||
            strcasecmp(tail,"ROTL8")==0 || strcasecmp(tail,"ROTR8")==0 ||
            strcasecmp(tail,"ROL16")==0 || strcasecmp(tail,"ROR16")==0 ||
            strcasecmp(tail,"ROTL16")==0 || strcasecmp(tail,"ROTR16")==0 ||
            strcasecmp(tail,"NIBSWAP")==0 || strcasecmp(tail,"SWAPNIB")==0 ||
            strcasecmp(tail,"NIBXCHG")==0 ||
            strcasecmp(tail,"WITHIN")==0 || strcasecmp(tail,"BETWEEN")==0 ||
            strcasecmp(tail,"NAND")==0 || strcasecmp(tail,"NOR")==0 ||
            strcasecmp(tail,"XNOR")==0 || strcasecmp(tail,"ANDN")==0 ||
            strcasecmp(tail,"POPCNT")==0 || strcasecmp(tail,"PCNT")==0 ||
            strcasecmp(tail,"CLZ")==0 || strcasecmp(tail,"CTZ")==0 ||
            strcasecmp(tail,"ORN")==0 || strcasecmp(tail,"BREV")==0 ||
            strcasecmp(tail,"BITREV")==0 || strcasecmp(tail,"PARITY")==0 ||
            strcasecmp(tail,"BLS")==0 || strcasecmp(tail,"BLSI")==0 ||
            strcasecmp(tail,"BLC")==0 || strcasecmp(tail,"BLSR")==0 ||
            strcasecmp(tail,"MASK")==0 || strcasecmp(tail,"BITMASK")==0 ||
            strcasecmp(tail,"BTEST")==0 || strcasecmp(tail,"BITT")==0 ||
            strcasecmp(tail,"SETB")==0 || strcasecmp(tail,"SETBIT")==0 ||
            strcasecmp(tail,"CLRB")==0 || strcasecmp(tail,"CLRBIT")==0 ||
            strcasecmp(tail,"FLIPB")==0 || strcasecmp(tail,"FLIPBIT")==0 ||
            strcasecmp(tail,"SETBN")==0 || strcasecmp(tail,"SETBITN")==0 ||
            strcasecmp(tail,"CLRBN")==0 || strcasecmp(tail,"CLRBITN")==0 ||
            strcasecmp(tail,"FLIPBN")==0 || strcasecmp(tail,"FLIPBITN")==0 ||
            strcasecmp(tail,"MASKN")==0 || strcasecmp(tail,"ONESN")==0 ||
            strcasecmp(tail,"ANDMN")==0 || strcasecmp(tail,"KEEPLN")==0 ||
            strcasecmp(tail,"ORMN")==0 || strcasecmp(tail,"XORMN")==0 ||
            strcasecmp(tail,"NANDMN")==0 || strcasecmp(tail,"NORMN")==0 ||
            strcasecmp(tail,"XNORMN")==0 ||
            strcasecmp(tail,"NANDHN")==0 || strcasecmp(tail,"NORHN")==0 ||
            strcasecmp(tail,"XNORHN")==0 ||
            strcasecmp(tail,"POPMN")==0 || strcasecmp(tail,"ANYMN")==0 ||
            strcasecmp(tail,"ALLMN")==0 ||
            strcasecmp(tail,"ADDMODN")==0 || strcasecmp(tail,"SUBMODN")==0 ||
            strcasecmp(tail,"MULMODN")==0 ||
            strcasecmp(tail,"POPHN")==0 || strcasecmp(tail,"ANYHN")==0 ||
            strcasecmp(tail,"ALLHN")==0 ||
            strcasecmp(tail,"BREVN")==0 || strcasecmp(tail,"ROLBN")==0 ||
            strcasecmp(tail,"RORBN")==0 ||
            strcasecmp(tail,"BREVHN")==0 || strcasecmp(tail,"ROLHN")==0 ||
            strcasecmp(tail,"RORHN")==0 ||
            strcasecmp(tail,"BREVNS")==0 || strcasecmp(tail,"ROLBNS")==0 ||
            strcasecmp(tail,"RORBNS")==0 ||
            strcasecmp(tail,"BREVHNS")==0 || strcasecmp(tail,"ROLHNS")==0 ||
            strcasecmp(tail,"RORHNS")==0 ||
            strcasecmp(tail,"HMASKN")==0 || strcasecmp(tail,"ANDHN")==0 ||
            strcasecmp(tail,"KEEPHN")==0 || strcasecmp(tail,"CLRLN")==0 ||
            strcasecmp(tail,"ORHN")==0 || strcasecmp(tail,"XORHN")==0 ||
            strcasecmp(tail,"CLRHN")==0 || strcasecmp(tail,"SETHN")==0 ||
            strcasecmp(tail,"FLIPHN")==0 ||
            strcasecmp(tail,"BEXTN")==0 || strcasecmp(tail,"BITEXTN")==0 ||
            strcasecmp(tail,"BDEPN")==0 || strcasecmp(tail,"BITDEPN")==0 ||
            strcasecmp(tail,"BTESTN")==0 || strcasecmp(tail,"BITN")==0 ||
            strcasecmp(tail,"TESTBITN")==0 ||
            strcasecmp(tail,"BYTEN")==0 || strcasecmp(tail,"GETBYTEN")==0 ||
            strcasecmp(tail,"SETBYTEN")==0 || strcasecmp(tail,"PUTBYTEN")==0 ||
            strcasecmp(tail,"CLRBYTEN")==0 || strcasecmp(tail,"ZAPBYTEN")==0 ||
            strcasecmp(tail,"SETBYIMM")==0 || strcasecmp(tail,"CLRBYIMM")==0 ||
            strcasecmp(tail,"NIBN")==0 || strcasecmp(tail,"GETNIBN")==0 ||
            strcasecmp(tail,"SETNIBN")==0 || strcasecmp(tail,"PUTNIBN")==0 ||
            strcasecmp(tail,"CLRNIBN")==0 || strcasecmp(tail,"ZAPNIBN")==0 ||
            strcasecmp(tail,"SETNIBIMM")==0 || strcasecmp(tail,"CLRNIBIMM")==0 ||
            strcasecmp(tail,"NIBBLEN")==0 ||
            strcasecmp(tail,"BEXT")==0 || strcasecmp(tail,"BITEXT")==0 ||
            strcasecmp(tail,"BDEP")==0 || strcasecmp(tail,"BITDEP")==0 ||
            strcasecmp(tail,"PEXT")==0 || strcasecmp(tail,"PDEP")==0 ||
            strcasecmp(tail,"ZIP")==0 || strcasecmp(tail,"UNZIP")==0 ||
            strcasecmp(tail,"MORTON")==0 || strcasecmp(tail,"DEMORTON")==0 ||
            strcasecmp(tail,"PAR")==0 ||
            strcasecmp(tail,"FFS")==0 || strcasecmp(tail,"FINDLS")==0 ||
            strcasecmp(tail,"FLS")==0 || strcasecmp(tail,"MSB")==0 ||
            strcasecmp(tail,"BWIDTH")==0 || strcasecmp(tail,"BITWIDTH")==0 ||
            strcasecmp(tail,"BHSI")==0 || strcasecmp(tail,"HIBIT")==0 ||
            strcasecmp(tail,"CEILPOW2")==0 || strcasecmp(tail,"NEXTPOW2")==0 ||
            strcasecmp(tail,"CPOW2")==0 ||
            strcasecmp(tail,"CLO")==0 || strcasecmp(tail,"CTO")==0 ||
            strcasecmp(tail,"ISPOW2")==0 || strcasecmp(tail,"POW2P")==0 ||
            strcasecmp(tail,"AVG")==0 || strcasecmp(tail,"MEAN")==0 ||
            strcasecmp(tail,"DIST")==0 || strcasecmp(tail,"ABSDIFF")==0 ||
            strcasecmp(tail,"HAMM")==0 || strcasecmp(tail,"HAMMING")==0 ||
            strcasecmp(tail,"POPDIFF")==0 ||
            strcasecmp(tail,"GEOM")==0 || strcasecmp(tail,"GEOMEAN")==0 ||
            strcasecmp(tail,"HARM")==0 || strcasecmp(tail,"HARMMEAN")==0 ||
            strcasecmp(tail,"RMS")==0 || strcasecmp(tail,"ROOTMS")==0 ||
            strcasecmp(tail,"DBL")==0 || strcasecmp(tail,"DOUBLE")==0 ||
            strcasecmp(tail,"HALF")==0 || strcasecmp(tail,"HALVE")==0 ||
            strcasecmp(tail,"BSWAP")==0 || strcasecmp(tail,"BSWAP32")==0 ||
            strcasecmp(tail,"BSWAP16")==0 || strcasecmp(tail,"BSWAP64")==0 ||
            strcasecmp(tail,"UMULHI")==0 || strcasecmp(tail,"UMULH")==0 ||
            strcasecmp(tail,"LOG2")==0 || strcasecmp(tail,"ILOG2")==0 ||
            strcasecmp(tail,"PHI")==0 || strcasecmp(tail,"TOTIENT")==0 ||
            strcasecmp(tail,"FIB")==0 || strcasecmp(tail,"FIBONACCI")==0 ||
            strcasecmp(tail,"FACT")==0 || strcasecmp(tail,"FACTORIAL")==0 ||
            strcasecmp(tail,"LOG10")==0 || strcasecmp(tail,"ILOG10")==0 ||
            strcasecmp(tail,"POW10")==0 || strcasecmp(tail,"TENPOW")==0 ||
            strcasecmp(tail,"MOBIUS")==0 || strcasecmp(tail,"MU")==0 ||
            strcasecmp(tail,"RAD")==0 || strcasecmp(tail,"RADICAL")==0 ||
            strcasecmp(tail,"SQFREE")==0 || strcasecmp(tail,"ISSQFREE")==0 ||
            strcasecmp(tail,"ISSQUAREFREE")==0 ||
            strcasecmp(tail,"ISPRIME")==0 || strcasecmp(tail,"PRIMEP")==0 ||
            strcasecmp(tail,"ODD")==0 || strcasecmp(tail,"EVEN")==0 ||
            strcasecmp(tail,"LTZ")==0 || strcasecmp(tail,"GTZ")==0 ||
            strcasecmp(tail,"LEZ")==0 || strcasecmp(tail,"GEZ")==0 ||
            strcasecmp(tail,"RELU")==0 || strcasecmp(tail,"CLAMP0")==0 ||
            strcasecmp(tail,"RELU6")==0 || strcasecmp(tail,"CLAMP6")==0 ||
            strcasecmp(tail,"DEADZ")==0 || strcasecmp(tail,"DEADZONE")==0 ||
            strcasecmp(tail,"DEAD")==0 ||
            strcasecmp(tail,"THRESH")==0 || strcasecmp(tail,"GATE")==0 ||
            strcasecmp(tail,"ANDIF")==0 || strcasecmp(tail,"TH")==0 ||
            strcasecmp(tail,"COPYSIGN")==0 || strcasecmp(tail,"CSIGN")==0 ||
            strcasecmp(tail,"MEDIAN")==0 || strcasecmp(tail,"MID3")==0 ||
            strcasecmp(tail,"MED")==0 ||
            strcasecmp(tail,"MAXABS")==0 || strcasecmp(tail,"MINABS")==0 ||
            strcasecmp(tail,"ABSMAX")==0 || strcasecmp(tail,"ABSMIN")==0 ||
            strcasecmp(tail,"RAND")==0 || strcasecmp(tail,"RND")==0 ||
            strcasecmp(tail,"SATADD")==0 || strcasecmp(tail,"SATSUB")==0 ||
            strcasecmp(tail,"SATMUL")==0 || strcasecmp(tail,"SATDIV")==0 ||
            strcasecmp(tail,"WRAP")==0 || strcasecmp(tail,"WRAPMOD")==0 ||
            strcasecmp(tail,"WMOD")==0 ||
            strcasecmp(tail,"HYP")==0 || strcasecmp(tail,"HYPOT")==0 ||
            strcasecmp(tail,"PCT")==0 || strcasecmp(tail,"PERCENT")==0 ||
            strcasecmp(tail,"LERP")==0 || strcasecmp(tail,"MIX")==0 ||
            strcasecmp(tail,"SCALE")==0 || strcasecmp(tail,"SCL")==0 ||
            strcasecmp(tail,"CLIP100")==0 || strcasecmp(tail,"CLIPPCT")==0 ||
            strcasecmp(tail,"ENCLIP")==0 ||
            strcasecmp(tail,"CLIP01")==0 || strcasecmp(tail,"UNIT")==0 ||
            strcasecmp(tail,"CLAMP01")==0 ||
            strcasecmp(tail,"COMP100")==0 || strcasecmp(tail,"ENCOMP")==0 ||
            strcasecmp(tail,"INV100")==0 ||
            strcasecmp(tail,"SUMSQ")==0 || strcasecmp(tail,"SSQ")==0 ||
            strcasecmp(tail,"POW2SUM")==0 ||
            strcasecmp(tail,"DIFFSQ")==0 || strcasecmp(tail,"SQDIFF")==0 ||
            strcasecmp(tail,"ERRSQ")==0 ||
            strcasecmp(tail,"STEP")==0 || strcasecmp(tail,"HEAVI")==0 ||
            strcasecmp(tail,"HEAVISIDE")==0 || strcasecmp(tail,"UNITSTEP")==0 ||
            strcasecmp(tail,"LEAKY")==0 || strcasecmp(tail,"LEAKYRELU")==0 ||
            strcasecmp(tail,"SOFTSIGN")==0 || strcasecmp(tail,"SOFTSGN")==0 ||
            strcasecmp(tail,"RANDRANGE")==0 || strcasecmp(tail,"RANDIN")==0 ||
            strcasecmp(tail,"RANDBITS")==0 || strcasecmp(tail,"RBITS")==0 ||
            strcasecmp(tail,"CLIP4")==0 || strcasecmp(tail,"CLIPN")==0 ||
            strcasecmp(tail,"SEXT4")==0 || strcasecmp(tail,"SEXTN")==0 ||
            strcasecmp(tail,"ZEXT4")==0 || strcasecmp(tail,"ZEXTN")==0 ||
            strcasecmp(tail,"CLIP8")==0 || strcasecmp(tail,"CLIP16")==0 ||
            strcasecmp(tail,"CLIP32")==0 ||
            strcasecmp(tail,"CLIPS4")==0 || strcasecmp(tail,"CLIPSN")==0 ||
            strcasecmp(tail,"CLIPS8")==0 || strcasecmp(tail,"CLIPSB")==0 ||
            strcasecmp(tail,"CLIPS16")==0 || strcasecmp(tail,"CLIPSW")==0 ||
            strcasecmp(tail,"CLIPS32")==0 || strcasecmp(tail,"CLIPSL")==0 ||
            strcasecmp(tail,"CLIPSD")==0 ||
            strcasecmp(tail,"ROL32")==0 || strcasecmp(tail,"ROTL32")==0 ||
            strcasecmp(tail,"ROR32")==0 || strcasecmp(tail,"ROTR32")==0 ||
            strcasecmp(tail,"SHL32")==0 || strcasecmp(tail,"SHR32")==0 ||
            strcasecmp(tail,"SAR32")==0 || strcasecmp(tail,"ASHR32")==0 ||
            strcasecmp(tail,"SEXT8")==0 || strcasecmp(tail,"SEXT16")==0 ||
            strcasecmp(tail,"SEXTB")==0 || strcasecmp(tail,"SEXTW")==0 ||
            strcasecmp(tail,"SEXT32")==0 || strcasecmp(tail,"SEXTD")==0 ||
            strcasecmp(tail,"ZEXT8")==0 || strcasecmp(tail,"ZEXT16")==0 ||
            strcasecmp(tail,"ZEXTB")==0 || strcasecmp(tail,"ZEXTW")==0 ||
            strcasecmp(tail,"ZEXT32")==0 || strcasecmp(tail,"ZEXTD")==0 ||
            strcasecmp(tail,"LO4")==0 || strcasecmp(tail,"HI4")==0 ||
            strcasecmp(tail,"PACK4")==0 || strcasecmp(tail,"PACKN")==0 ||
            strcasecmp(tail,"LOWN")==0 || strcasecmp(tail,"HIN")==0 ||
            strcasecmp(tail,"NIBLO")==0 || strcasecmp(tail,"NIBHI")==0 ||
            strcasecmp(tail,"LO8")==0 || strcasecmp(tail,"HI8")==0 ||
            strcasecmp(tail,"PACK8")==0 || strcasecmp(tail,"PACKB")==0 ||
            strcasecmp(tail,"LOWB")==0 || strcasecmp(tail,"HIB")==0 ||
            strcasecmp(tail,"LO16")==0 || strcasecmp(tail,"HI16")==0 ||
            strcasecmp(tail,"PACK16")==0 || strcasecmp(tail,"PACKW")==0 ||
            strcasecmp(tail,"LOWW")==0 || strcasecmp(tail,"HIW")==0 ||
            strcasecmp(tail,"LO32")==0 || strcasecmp(tail,"HI32")==0 ||
            strcasecmp(tail,"PACK32")==0 || strcasecmp(tail,"PACKDW")==0 ||
            strcasecmp(tail,"LOWD")==0 || strcasecmp(tail,"HID")==0)
          ok = 1;
      } else if (b[0]=='3'){
        if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
            strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"OVER")==0 ||
            strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"NIP")==0 ||
            strcasecmp(tail,"TUCK")==0 || strcasecmp(tail,"RROT")==0)
          ok = 1;
      } else if (b[0]=='4'){
        if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
            strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"NIP")==0 ||
            strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"RROT")==0 ||
            strcasecmp(tail,"OVER")==0 || strcasecmp(tail,"TUCK")==0)
          ok = 1;
      } else if (b[0]=='5'){
        /* 5… depth plane */
        if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
            strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"NIP")==0 ||
            strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"RROT")==0 ||
            strcasecmp(tail,"OVER")==0 || strcasecmp(tail,"TUCK")==0)
          ok = 1;
      } else if (b[0]=='6'){
        /* 6… depth plane (digit-8 stack) */
        if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
            strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"NIP")==0 ||
            strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"RROT")==0 ||
            strcasecmp(tail,"OVER")==0 || strcasecmp(tail,"TUCK")==0)
          ok = 1;
      } else if (b[0]=='7'){
        /* 7… depth plane (digit-0/7/8 stack foundation) */
        if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
            strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"NIP")==0 ||
            strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"RROT")==0 ||
            strcasecmp(tail,"OVER")==0 || strcasecmp(tail,"TUCK")==0)
          ok = 1;
      } else if (b[0]=='8'){
        /* 8… depth plane (foundation + combinators complete) */
        if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
            strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"NIP")==0 ||
            strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"RROT")==0 ||
            strcasecmp(tail,"OVER")==0 || strcasecmp(tail,"TUCK")==0)
          ok = 1;
      } else if (b[0]=='9'){
        /* 9… depth plane (foundation + combinators complete) */
        if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
            strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"NIP")==0 ||
            strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"RROT")==0 ||
            strcasecmp(tail,"OVER")==0 || strcasecmp(tail,"TUCK")==0)
          ok = 1;
      }
      if (ok){
        L->i = j;
        snprintf(L->cur.text, sizeof L->cur.text, "%c%s", b[0], tail);
        for (char *p=L->cur.text; *p; p++)
          if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
        L->cur.kind = TK_IDENT;
        return;
      }
    }
    b[k]=0; L->cur.num=strtol(b,NULL,10); L->cur.kind=TK_NUM;
    snprintf(L->cur.text,sizeof L->cur.text,"%s",b); return;
  }
  if (c=='+'){ L->i++; L->cur.kind=TK_PLUS; return; }
  if (c=='-'){ L->i++; L->cur.kind=TK_MINUS; return; }
  if (c=='*'){ L->i++; L->cur.kind=TK_STAR; return; }
  if (c=='/'){ L->i++; L->cur.kind=TK_SLASH; return; }
  if (c=='%'){ L->i++; L->cur.kind=TK_PERCENT; return; }
  if (c=='('){ L->i++; L->cur.kind=TK_LPAREN; return; }
  if (c==')'){ L->i++; L->cur.kind=TK_RPAREN; return; }
  if (c==','){ L->i++; L->cur.kind=TK_COMMA; return; }
  if (c=='='){
    L->i++;
    if (L->i<L->n && L->s[L->i]=='='){ L->i++; L->cur.kind=TK_EQEQ; return; }
    L->cur.kind=TK_EQ; return;
  }
  if (c=='<'){
    L->i++;
    if (L->i<L->n && L->s[L->i]=='='){ L->i++; L->cur.kind=TK_LE; return; }
    L->cur.kind=TK_LT; return;
  }
  if (c=='>'){
    L->i++;
    if (L->i<L->n && L->s[L->i]=='='){ L->i++; L->cur.kind=TK_GE; return; }
    L->cur.kind=TK_GT; return;
  }
  if (is_id0((unsigned char)c)){
    size_t k=0;
    while (L->i<L->n && is_id((unsigned char)L->s[L->i])){
      if (k+1<sizeof L->cur.text) L->cur.text[k++]=L->s[L->i];
      L->i++;
    }
    L->cur.text[k]=0; L->cur.kind=TK_IDENT; return;
  }
  /* skip unknown */
  L->i++; L->cur.kind=TK_NL;
}

void cubalc_lang_lex_init(Lex *L, const char *s, size_t n){
  L->s=s; L->n=n; L->i=0; L->line=1; lex_next(L);
}
void cubalc_lang_skip_nl(Lex *L){ while (L->cur.kind==TK_NL) lex_next(L); }

int cubalc_lang_find_cube(VM *vm, const char *id){
  for (int i=0;i<vm->ch.n_cubes;i++)
    if (strcmp(vm->ch.cubes[i].id, id)==0) return i;
  return -1;
}
void cubalc_lang_ensure_world(VM *vm){
  if (vm->ch.n_cubes > 0 || vm->ch.initial.n > 0) return;
  cubalc_matrix gen;
  cubalc_coord_to_matrix(
    "NEXUS_COORD v1 | from=play | type=world | hold_flash=1 | visual=units |", &gen);
  cubalc_chain_from_initial(&vm->ch, &gen, 1);
  vm->ch.hold_flash = (uint8_t)vm->hold_flash;
  snprintf(vm->ch.creed, sizeof vm->ch.creed, "%s",
           vm->creed[0]?vm->creed:CUBALC_CREED);
}
void cubalc_lang_chunk_push(VM *vm, const char *id){
  if (vm->n_chunk >= 40) return;
  snprintf(vm->chunk[vm->n_chunk], sizeof vm->chunk[0], "%s", id);
  vm->n_chunk++;
}

Var *cubalc_lang_var_get(VM *vm, const char *name, int create) {
  for (int i=0;i<vm->n_vars;i++)
    if (strcmp(vm->vars[i].name, name)==0) return &vm->vars[i];
  if (!create || vm->n_vars >= CUBALC_MAX_VARS) {
    if (create && !vm->vars_full) {
      vm->vars_full = 1;
      snprintf(vm->err, sizeof vm->err,
               "var table full (%d) — specials/LET may drop; raise CUBALC_MAX_VARS or lean the board",
               CUBALC_MAX_VARS);
      if (vm->res)
        snprintf(vm->res->last_err, sizeof vm->res->last_err, "%s", vm->err);
    }
    return NULL;
  }
  Var *v = &vm->vars[vm->n_vars++];
  memset(v, 0, sizeof *v);
  snprintf(v->name, sizeof v->name, "%s", name);
  return v;
}
void cubalc_lang_var_set_num(VM *vm, const char *name, long val) {
  Var *v = var_get(vm, name, 1);
  if (v) { v->val = val; v->is_str = 0; v->sval[0]=0; }
}
void cubalc_lang_var_set_str(VM *vm, const char *name, const char *s) {
  Var *v = var_get(vm, name, 1);
  if (v) {
    v->is_str = 1;
    snprintf(v->sval, sizeof v->sval, "%s", s ? s : "");
    v->val = (long)strlen(v->sval);
  }
}

int cubalc_lang_place_cube(VM *vm, const char *id, const char *role, int proton){
  ensure_world(vm);
  if (find_cube(vm, id) >= 0) return find_cube(vm, id); /* already placed */
  float x = (float)(vm->ch.n_cubes % 5) * 0.28f;
  float z = (float)(vm->ch.n_cubes / 5) * 0.28f;
  if (cubalc_cube_spawn(&vm->ch, id, role && role[0]?role:id,
                        (uint8_t)(proton?1:0), x, 0.f, z) < 0) {
    fail(vm, "world full — budget");
    return -1;
  }
  chunk_push(vm, id);
  return find_cube(vm, id);
}
void cubalc_lang_do_plug(VM *vm, const char *a, const char *b){
  int ia=find_cube(vm,a), ib=find_cube(vm,b);
  if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
  if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
  if (ia<0||ib<0){ fail(vm,"plug missing unit"); return; }
  int rc = cubalc_cube_plug(&vm->ch, ia, ib);
  /* Virtual PLUG is never HOLD_FLASH-gated. Incompatible/missing ports soft-fail. */
  var_set_num(vm,"OK", rc == 0 ? 1 : 0);
  if (vm->trace && rc < 0)
    fprintf(vm->trace, "# PLUG soft-fail rc=%d (compat/ports)\n", rc);
}
/* Only CUBE is defined — I/O is pluggable; reverse flips IN/OUT on the wire. */
void cubalc_lang_do_reverse(VM *vm, const char *a, const char *b){
  int ia=find_cube(vm,a), ib=find_cube(vm,b);
  if (ia<0||ib<0){ fail(vm,"REVERSE needs two units"); return; }
  int rc = cubalc_cube_reverse(&vm->ch, ia, ib);
  if (rc < 0){ fail(vm,"REVERSE: no plug between units (pluggable I/O only)"); return; }
  var_set_num(vm, "REVERSED", rc);
  var_set_num(vm, "OK", 1);
}
void cubalc_lang_do_unplug(VM *vm, const char *a, const char *b){
  int ia=find_cube(vm,a), ib=find_cube(vm,b);
  if (ia<0||ib<0){ fail(vm,"UNPLUG needs two units"); return; }
  cubalc_cube_unplug(&vm->ch, ia, ib);
  var_set_num(vm, "OK", 1);
}
void cubalc_lang_do_io(VM *vm, const char *id, int face, int is_out){
  int ix=find_cube(vm,id);
  if (ix<0){ place_cube(vm,id,"io",1); ix=find_cube(vm,id); }
  if (ix<0){ fail(vm,"IO unit missing"); return; }
  int rc = cubalc_cube_io(&vm->ch, ix, face,
    is_out ? CUBALC_PORT_OUT : CUBALC_PORT_IN);
  if (rc < 0){ fail(vm,"IO port full"); return; }
  var_set_num(vm, "OK", 1);
  var_set_num(vm, "PORT", rc);
}
/* Nest child inside parent — units may nest. */
void cubalc_lang_do_nest(VM *vm, const char *parent, const char *child){
  int ip=find_cube(vm,parent), ic=find_cube(vm,child);
  if (ip<0){ place_cube(vm,parent,"shell",1); ip=find_cube(vm,parent); }
  if (ic<0){ place_cube(vm,child,"inner",1); ic=find_cube(vm,child); }
  if (ip<0||ic<0){ fail(vm,"NEST parent child — missing unit"); return; }
  int rc = cubalc_cube_nest(&vm->ch, ip, ic);
  if (rc == -2){ fail(vm,"NEST cycle or depth limit"); return; }
  if (rc < 0){ fail(vm,"NEST failed"); return; }
  var_set_num(vm, "OK", 1);
  var_set_num(vm, "NESTED", 1);
  var_set_num(vm, "PARENT", ip);
}
void cubalc_lang_do_unnest(VM *vm, const char *child){
  int ic=find_cube(vm,child);
  if (ic<0){ fail(vm,"UNNEST needs unit"); return; }
  cubalc_cube_unnest(&vm->ch, ic);
  var_set_num(vm, "OK", 1);
}
/* Law: each cube compiles into a matrix. No flow — no compiling. */
void cubalc_lang_do_compile_cube(VM *vm, const char *id){
  int ix=find_cube(vm,id);
  if (ix<0){ fail(vm,"COMPILE needs unit"); return; }
  int rc = cubalc_cube_compile(&vm->ch, ix);
  var_set_num(vm, "COMPILE_RC", rc);
  if (rc == -2){
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "COMPILED", 0);
    /* soft fail: law gate, not fatal — program may ASSERT no-compile */
    if (vm->trace) fprintf(vm->trace, "# COMPILE blocked: no flow on %s\n", id);
    return;
  }
  if (rc == -3){
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "COMPILED", 0);
    if (vm->trace) fprintf(vm->trace, "# COMPILE blocked: nested child not compiled for %s\n", id);
    return;
  }
  if (rc < 0){ fail(vm,"COMPILE failed"); return; }
  var_set_num(vm, "OK", 1);
  var_set_num(vm, "COMPILED", 1);
  var_set_num(vm, "SET", (long)vm->ch.cubes[ix].compiled_matrix.set);
}
void cubalc_lang_do_compile_all(VM *vm){
  ensure_world(vm);
  int failed = -1;
  int rc = cubalc_chain_compile(&vm->ch, &failed);
  var_set_num(vm, "COMPILE_RC", rc);
  var_set_num(vm, "FAILED", failed);
  if (rc == -2){
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "COMPILED", 0);
    if (vm->trace) fprintf(vm->trace, "# COMPILE ALL blocked: no flow (ix=%d)\n", failed);
    return;
  }
  if (rc < 0){
    var_set_num(vm, "OK", 0);
    if (vm->trace) fprintf(vm->trace, "# COMPILE ALL rc=%d ix=%d\n", rc, failed);
    return;
  }
  int ncomp = 0;
  for (int i = 0; i < vm->ch.n_cubes; i++)
    if (vm->ch.cubes[i].compiled) ncomp++;
  var_set_num(vm, "OK", 1);
  var_set_num(vm, "COMPILED", ncomp);
}
void cubalc_lang_do_ring(VM *vm){
  int n = vm->ch.n_cubes;
  if (n < 2) return;
  for (int i=0;i<n;i++) cubalc_cube_plug(&vm->ch, i, (i+1)%n);
}
void cubalc_lang_do_flow(VM *vm, int n){
  if (n < 1) n = 1;
  if (n > 1000) n = 1000;
  ensure_world(vm);
  /* async parallel energy flow — energy must flow (CPU workers; GPU-shaped path ready) */
  if (cubalc_async_chain_flow(&vm->ch, n) != 0) {
    for (int i=0;i<n;i++) cubalc_chain_flow(&vm->ch);
  }
}
void cubalc_lang_do_show(VM *vm, const char *id){
  FILE *o = vm->trace ? vm->trace : stdout;
  ensure_world(vm);
  if (id && id[0] && find_cube(vm,id)>=0)
    cubalc_cube_print_spin(&vm->ch, id, o);
  else
    cubalc_chain_print_cubes(&vm->ch, o);
}

/* forward */
int cubalc_lang_parse_form(VM *vm, Lex *L);
int cubalc_lang_parse_cube(VM *vm, Lex *L);
void cubalc_lang_do_deconstruct(VM *vm, const char *id);
void cubalc_lang_do_reconstruct(VM *vm, const char *id);
long cubalc_lang_do_decide(VM *vm, const char *id);
long *cubalc_lang_var_slot(VM *vm, const char *name, int create);
int cubalc_lang_exec_stmts_until(VM *vm, Lex *L, const char *stop1, const char *stop2);
long cubalc_lang_parse_expr(VM *vm, Lex *L);

/* Parse inside [ ... ] already consumed '[' */
int cubalc_lang_parse_cube_body(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind == TK_RBRACK){ lex_next(L); bump(vm); return 1; }

  /* nested chunk: [ [a] [b] [c] ]  → place children, ring them */
  if (L->cur.kind == TK_LBRACK){
    int save = vm->n_chunk;
    vm->n_chunk = 0;
    char local[40][48]; int nloc=0;
    while (L->cur.kind != TK_RBRACK && L->cur.kind != TK_EOF && !vm->fatal){
      skip_nl(L);
      if (L->cur.kind == TK_RBRACK) break;
      if (L->cur.kind == TK_LBRACK){
        int before = vm->ch.n_cubes;
        if (parse_cube(vm, L) < 0) return -1;
        /* collect ids placed in this nested form if single place */
        (void)before;
      } else if (L->cur.kind == TK_TILDE){
        lex_next(L);
        int n = 1;
        if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
        do_flow(vm, n); bump(vm);
      } else if (L->cur.kind == TK_QMARK){
        lex_next(L);
        char id[48]={0};
        if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
        do_show(vm, id[0]?id:NULL); bump(vm);
      } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_NUM){
        /* bare word inside chunk treated as place */
        char id[48]; snprintf(id,sizeof id,"%s", L->cur.kind==TK_NUM?L->cur.text:L->cur.text);
        lex_next(L);
        char role[48]; snprintf(role,sizeof role,"%s",id);
        int proton=1;
        if (L->cur.kind==TK_COLON){
          lex_next(L);
          if (L->cur.kind==TK_IDENT||L->cur.kind==TK_NUM){
            snprintf(role,sizeof role,"%s",L->cur.text); lex_next(L);
          }
        }
        if (L->cur.kind==TK_BANG){
          lex_next(L); proton=1;
          if (L->cur.kind==TK_NUM){ proton=L->cur.num?1:0; lex_next(L); }
          place_cube(vm,id,role,proton);
          cubalc_chain_impulse(&vm->ch, id, (uint8_t)proton);
        } else {
          place_cube(vm,id,role,proton);
        }
        if (nloc<40){ snprintf(local[nloc],sizeof local[0],"%s",id); nloc++; }
        bump(vm);
      } else {
        lex_next(L);
      }
    }
    if (L->cur.kind != TK_RBRACK){ fail(vm,"chunk missing ]"); return -1; }
    lex_next(L);
    /* ring cubes placed in this chunk session */
    if (vm->n_chunk >= 2){
      for (int i=0;i<vm->n_chunk;i++){
        int a=find_cube(vm, vm->chunk[i]);
        int b=find_cube(vm, vm->chunk[(i+1)%vm->n_chunk]);
        if (a>=0&&b>=0) cubalc_cube_plug(&vm->ch,a,b);
      }
    } else if (nloc>=2){
      for (int i=0;i<nloc;i++){
        int a=find_cube(vm, local[i]);
        int b=find_cube(vm, local[(i+1)%nloc]);
        if (a>=0&&b>=0) cubalc_cube_plug(&vm->ch,a,b);
      }
    }
    vm->n_chunk = save;
    bump(vm);
    return 1;
  }

  /* [~n] flow */
  if (L->cur.kind == TK_TILDE){
    lex_next(L);
    int n=1;
    if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[~n] needs ]"); return -1; }
    lex_next(L);
    do_flow(vm,n); bump(vm); return 1;
  }

  /* [?] or [?name] */
  if (L->cur.kind == TK_QMARK){
    lex_next(L);
    char id[48]={0};
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[?] needs ]"); return -1; }
    lex_next(L);
    do_show(vm, id[0]?id:NULL); bump(vm); return 1;
  }

  /* keywords inside cube: hold, ring, os, genesis, share */
  if (kw(&L->cur,"hold") || kw(&L->cur,"HOLD_FLASH")){
    lex_next(L);
    int v=1;
    if (L->cur.kind==TK_NUM){ v=L->cur.num?1:0; lex_next(L); }
    vm->hold_flash=v; vm->ch.hold_flash=(uint8_t)v;
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[hold] needs ]"); return -1; }
    lex_next(L); bump(vm); return 1;
  }
  if (kw(&L->cur,"ring")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[ring]"); return -1; }
    lex_next(L); do_ring(vm); bump(vm); return 1;
  }
  if (kw(&L->cur,"os") || kw(&L->cur,"OS_ASPECTS")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[os]"); return -1; }
    lex_next(L); ensure_world(vm); cubalc_chain_os_aspects(&vm->ch); bump(vm); return 1;
  }
  /* [sync] — hive lattice as abstract roles (no product/device names) */
  if (kw(&L->cur,"sync") || kw(&L->cur,"HIVE_SYNC")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[sync]"); return -1; }
    lex_next(L);
    ensure_world(vm);
    place_cube(vm, "construct", "construct", 1);
    place_cube(vm, "architect", "architect", 1);
    place_cube(vm, "peer_body", "body", 1);
    place_cube(vm, "peer_host", "host", 0); /* optional offline peer still a cube */
    place_cube(vm, "side", "SIDE_organ", 1);
    place_cube(vm, "hive", "atom", 1);
    do_plug(vm, "side", "construct");
    do_plug(vm, "construct", "architect");
    do_plug(vm, "architect", "peer_body");
    do_plug(vm, "peer_body", "hive");
    do_plug(vm, "hive", "side");
    do_plug(vm, "side", "peer_host");
    cubalc_chain_impulse(&vm->ch, "hive", 1);
    cubalc_chain_impulse(&vm->ch, "construct", 1);
    do_flow(vm, 4);
    if (vm->trace) fprintf(vm->trace, "# sync: construct architect peer_body peer_host side hive\n");
    bump(vm); return 1;
  }
  /* [export "path"] — dump board JSON for host (Grokium way) */
  if (kw(&L->cur,"export") || kw(&L->cur,"dump")){
    lex_next(L);
    char path[256];
    snprintf(path,sizeof path,"state/cubalc_export.json");
    if (L->cur.kind==TK_STR){ snprintf(path,sizeof path,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[export]"); return -1; }
    lex_next(L);
    ensure_world(vm);
    {
      FILE *f = fopen(path, "w");
      if (!f){ fail(vm,"export open failed"); return -1; }
      fprintf(f, "{\"schema\":\"cubalc.export.v1\",\"lang\":\"CubalC\",\"version\":\"%s\","
                 "\"hold_flash\":%d,\"n_cubes\":%d,\"unity\":%.4f,\"seq\":%u,"
                 "\"creed\":\"%s\",\"talk\":\"binary_CBLC\",\"store\":\"cubechain\","
                 "\"share\":\"%s\",\"cubes\":[",
              CUBALC_LANG_VERSION, (int)vm->ch.hold_flash, vm->ch.n_cubes,
              vm->ch.unity, (unsigned)vm->ch.seq, vm->ch.creed, CUBALC_SHARE);
      for (int i=0;i<vm->ch.n_cubes;i++){
        cubalc_cube *c=&vm->ch.cubes[i];
        if (i) fputc(',',f);
        fprintf(f, "{\"id\":\"%s\",\"role\":\"%s\",\"proton\":%u,\"energy\":%.3f,"
                   "\"set\":%u,\"digit\":%u,\"plugged\":%u}",
                c->id, c->role, (unsigned)c->atom.proton, c->atom.energy,
                (unsigned)c->atom.matrix.set, (unsigned)c->atom.digit,
                (unsigned)c->plugged);
      }
      fprintf(f, "]}\n");
      fclose(f);
    }
    if (vm->trace) fprintf(vm->trace, "# export %s\n", path);
    bump(vm); return 1;
  }
  /* [fleet] — Grokium nanobot roles as units */
  if (kw(&L->cur,"fleet") || kw(&L->cur,"nanobots")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[fleet]"); return -1; }
    lex_next(L);
    ensure_world(vm);
    place_cube(vm,"nb-integrity","integrity_no_leak",1);
    place_cube(vm,"nb-matrix-eval","evaluate_sot_smx",1);
    place_cube(vm,"nb-construct","construct_edge",1);
    place_cube(vm,"nb-observer","observe_unity",1);
    place_cube(vm,"nb-host","station_liaison",1);
    place_cube(vm,"hive","nanobot",1);
    do_plug(vm,"nb-integrity","nb-matrix-eval");
    do_plug(vm,"nb-matrix-eval","nb-construct");
    do_plug(vm,"nb-construct","nb-observer");
    do_plug(vm,"nb-observer","nb-host");
    do_plug(vm,"nb-host","hive");
    do_plug(vm,"hive","nb-integrity");
    cubalc_chain_impulse(&vm->ch,"hive",1);
    do_flow(vm,3);
    if (vm->trace) fprintf(vm->trace, "# fleet units placed\n");
    bump(vm); return 1;
  }
  /* [status] — short board line for hosts */
  if (kw(&L->cur,"status")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[status]"); return -1; }
    lex_next(L);
    ensure_world(vm);
    if (vm->trace)
      fprintf(vm->trace,
        "{\"schema\":\"cubalc.status.v1\",\"ok\":true,\"n_cubes\":%d,\"unity\":%.3f,"
        "\"hold_flash\":%d,\"seq\":%u,\"version\":\"%s\"}\n",
        vm->ch.n_cubes, vm->ch.unity, (int)vm->ch.hold_flash,
        (unsigned)vm->ch.seq, CUBALC_LANG_VERSION);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"deconstruct")||kw(&L->cur,"destroy")){
    lex_next(L);
    char id[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[deconstruct]"); return -1; }
    lex_next(L); do_deconstruct(vm,id); bump(vm); return 1;
  }
  if (kw(&L->cur,"reconstruct")||kw(&L->cur,"construct")){
    lex_next(L);
    char id[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[reconstruct]"); return -1; }
    lex_next(L); do_reconstruct(vm,id); bump(vm); return 1;
  }
  if (kw(&L->cur,"decide")||kw(&L->cur,"algocube")){
    lex_next(L);
    char id[48]={0};
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[decide]"); return -1; }
    lex_next(L); do_decide(vm,id[0]?id:NULL); bump(vm); return 1;
  }
  if (kw(&L->cur,"share")){
    lex_next(L);
    while (L->cur.kind!=TK_RBRACK && L->cur.kind!=TK_EOF) lex_next(L);
    if (L->cur.kind==TK_RBRACK) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"genesis") || kw(&L->cur,"world")){
    lex_next(L);
    char plate[512];
    if (L->cur.kind==TK_STR){
      snprintf(plate,sizeof plate,"%s",L->cur.text); lex_next(L);
    } else {
      snprintf(plate,sizeof plate,
        "NEXUS_COORD v1 | from=play | type=world | hold_flash=%d |", vm->hold_flash);
    }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[genesis] needs ]"); return -1; }
    lex_next(L);
    cubalc_matrix gen; cubalc_coord_to_matrix(plate,&gen);
    cubalc_chain_from_initial(&vm->ch,&gen,1);
    vm->ch.hold_flash=(uint8_t)vm->hold_flash;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"creed")){
    lex_next(L);
    if (L->cur.kind==TK_STR){
      snprintf(vm->creed,sizeof vm->creed,"%s",L->cur.text);
      snprintf(vm->ch.creed,sizeof vm->ch.creed,"%s",L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[creed]"); return -1; }
    lex_next(L); bump(vm); return 1;
  }

  /* place / plug / pulse:
   * [name]
   * [name:role]
   * [name!]
   * [name!0]
   * [a~b]
   * [a~b~c]
   * [a|b]
   */
  if (L->cur.kind != TK_IDENT && L->cur.kind != TK_NUM){
    fail(vm, "empty or unknown cube body");
    return -1;
  }
  char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);

  /* plug chain a~b~c or a|b */
  if (L->cur.kind==TK_TILDE || L->cur.kind==TK_PIPE){
    char prev[48]; snprintf(prev,sizeof prev,"%s",a);
    place_cube(vm, prev, prev, 1);
    while (L->cur.kind==TK_TILDE || L->cur.kind==TK_PIPE){
      lex_next(L);
      skip_nl(L);
      if (L->cur.kind!=TK_IDENT && L->cur.kind!=TK_NUM){ fail(vm,"plug needs name"); return -1; }
      char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
      place_cube(vm, b, b, 1);
      do_plug(vm, prev, b);
      snprintf(prev,sizeof prev,"%s",b);
    }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"plug chain needs ]"); return -1; }
    lex_next(L); bump(vm); return 1;
  }

  char role[48]; snprintf(role,sizeof role,"%s",a);
  int proton=1;
  int do_impulse=0;
  if (L->cur.kind==TK_COLON){
    lex_next(L);
    if (L->cur.kind==TK_IDENT||L->cur.kind==TK_NUM){
      snprintf(role,sizeof role,"%s",L->cur.text); lex_next(L);
    }
  }
  if (L->cur.kind==TK_BANG){
    do_impulse=1; lex_next(L);
    if (L->cur.kind==TK_NUM){ proton=L->cur.num?1:0; lex_next(L); }
  }
  if (L->cur.kind!=TK_RBRACK){ fail(vm,"cube needs ]"); return -1; }
  lex_next(L);
  place_cube(vm, a, role, proton);
  if (do_impulse) cubalc_chain_impulse(&vm->ch, a, (uint8_t)proton);
  bump(vm);
  return 1;
}

int cubalc_lang_parse_cube(VM *vm, Lex *L){
  if (L->cur.kind != TK_LBRACK){ fail(vm,"expected ["); return -1; }
  lex_next(L);
  return parse_cube_body(vm, L);
}


long *cubalc_lang_var_slot(VM *vm, const char *name, int create){
  Var *v = cubalc_lang_var_get(vm, name, create);
  return v ? &v->val : NULL;
}

/* Flow law: if stuck, deconstruct then reconstruct the way out. */
void cubalc_lang_do_deconstruct(VM *vm, const char *id){
  ensure_world(vm);
  int ix=find_cube(vm,id);
  if (ix<0){ place_cube(vm,id,"deconstruct",0); ix=find_cube(vm,id); }
  if (ix<0) return;
  cubalc_chain_impulse(&vm->ch, id, 0);
  /* clear half plugs by draining energy on peers */
  cubalc_cube *c=&vm->ch.cubes[ix];
  for (int p=0;p<c->n_ports;p++){
    int peer=c->ports[p].peer;
    if (peer>=0 && peer<vm->ch.n_cubes)
      cubalc_chain_impulse(&vm->ch, vm->ch.cubes[peer].id, 0);
  }
  if (vm->trace) fprintf(vm->trace,"# DECONSTRUCT %s\n",id);
}

void cubalc_lang_do_reconstruct(VM *vm, const char *id){
  ensure_world(vm);
  int ix=find_cube(vm,id);
  if (ix<0){ place_cube(vm,id,"construct",1); ix=find_cube(vm,id); }
  if (ix<0) return;
  cubalc_chain_impulse(&vm->ch, id, 1);
  /* re-open ports + ring-adjacent plugs for flow */
  for (int i=0;i<vm->ch.n_cubes;i++){
    if (i==ix) continue;
    float c=cubalc_matrix_compat(&vm->ch.cubes[ix].atom.matrix,&vm->ch.cubes[i].atom.matrix);
    if (c>=0.35f) cubalc_cube_plug(&vm->ch, ix, i);
  }
  do_flow(vm, 2);
  if (vm->trace) fprintf(vm->trace,"# RECONSTRUCT %s\n",id);
}

/* Resolve path/string arg: "lit" | LAST | string-var */
int cubalc_lang_resolve_str_arg(VM *vm, Lex *L, char *out, size_t outn){
  if (L->cur.kind==TK_STR){
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 0;
  }
  if (L->cur.kind==TK_IDENT){
    if (strcmp(L->cur.text,"LAST")==0){
      snprintf(out, outn, "%s", vm->last_str);
      lex_next(L);
      return 0;
    }
    Var *v = var_get(vm, L->cur.text, 0);
    if (v && v->is_str){
      /* prefer defined string var (REQUIRE LIB b after EACH LINE / PICKLIB) */
      snprintf(out, outn, "%s", v->sval);
      lex_next(L);
      return 0;
    }
    if (v && !v->is_str){
      /* numeric var → decimal (SYS CAT "x=" hit must not print name "hit") */
      snprintf(out, outn, "%ld", v->val);
      lex_next(L);
      return 0;
    }
    /* undefined bare IDENT → literal stem/name (REQUIRE LIB hold_seed · REQUIRE BIN sh).
     * Restores pre-var-resolve dual: var when set, else unquoted name. */
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 0;
  }
  return -1;
}

/* Peer digit inject: SETDIGIT cube n — CubeBrain algocube 0–9 into matrix SoT */
void cubalc_lang_do_setdigit(VM *vm, const char *id, long d){
  ensure_world(vm);
  if (d < 0) d = 0;
  if (d > 9) d = d % 10;
  int ix = find_cube(vm, id);
  if (ix < 0){ place_cube(vm, id, "peer", 1); ix = find_cube(vm, id); }
  if (ix < 0) return;
  cubalc_cube *c = &vm->ch.cubes[ix];
  c->atom.digit = (uint8_t)d;
  c->atom.digit_lock = 1; /* sticky through tick/impulse — peer digit inject */
  c->atom.alive = 1;
  c->atom.energy = fminf(1.f, c->atom.energy + 0.20f);
  /* encode digit pulse into State Matrix — matrix remains SoT */
  for (int i = 0; i < 8; i++)
    cubalc_matrix_set(&c->atom.matrix, (int)((d * 3 + i) % CUBALC_ATOM_BITS), 1);
  cubalc_matrix_set(&c->atom.matrix, (int)d, 1);
  cubalc_matrix_set(&c->atom.matrix, (int)d + 10, 1);
  long *slot = var_slot(vm, "DIGIT", 1);
  if (slot) *slot = d;
  if (vm->trace) fprintf(vm->trace, "# SETDIGIT %s → %ld (%s)\n",
    c->id, d, CUBALC_DIGIT_TAG[d % 10]);
}

/* FOLDBITS cube bits — fold 0/1 stream (newlines ok) into cube matrix + recompute digit */
void cubalc_lang_do_foldbits(VM *vm, const char *id, const char *bits){
  ensure_world(vm);
  int ix = find_cube(vm, id);
  if (ix < 0){ place_cube(vm, id, "io", 1); ix = find_cube(vm, id); }
  if (ix < 0 || !bits) return;
  char compact[CUBALC_ATOM_BITS + 1];
  int n = 0;
  for (const char *p = bits; *p && n < CUBALC_ATOM_BITS; p++){
    if (*p == '0' || *p == '1') compact[n++] = *p;
  }
  compact[n] = 0;
  if (n <= 0) return;
  cubalc_matrix_from_ascii(&vm->ch.cubes[ix].atom.matrix, compact, n);
  /* keep full atom width for plugs */
  if (vm->ch.cubes[ix].atom.matrix.n < CUBALC_ATOM_BITS)
    vm->ch.cubes[ix].atom.matrix.n = CUBALC_ATOM_BITS;
  /* FOLDBITS owns matrix → unlock peer digit so algocube can recompute */
  vm->ch.cubes[ix].atom.digit_lock = 0;
  vm->ch.cubes[ix].atom.digit =
    (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
  vm->ch.cubes[ix].atom.alive = 1;
  if (vm->trace) fprintf(vm->trace, "# FOLDBITS %s n=%d digit=%u\n",
    id, n, (unsigned)vm->ch.cubes[ix].atom.digit);
}

/* EEG → State Matrix: fold packed brain window into cube atom SoT */
void cubalc_lang_do_eeg_fold_matrix(VM *vm, const char *id, const cubalc_matrix *m){
  if (!vm || !m) return;
  char bits[CUBALC_ATOM_BITS + 1];
  cubalc_eeg_matrix_bits(m, bits, sizeof bits);
  do_foldbits(vm, id && id[0] ? id : "eeg", bits);
  var_set_str(vm, "EEG_BITS", bits);
  var_set_num(vm, "EEG_SET", (long)m->set);
  int ix = find_cube(vm, id && id[0] ? id : "eeg");
  long dig = 0;
  if (ix >= 0) dig = (long)vm->ch.cubes[ix].atom.digit;
  else dig = (long)cubalc_algocube_digit(m);
  var_set_num(vm, "EEG_DIGIT", dig);
  var_set_num(vm, "DIGIT", dig);
  var_set_num(vm, "OK", 1);
  if (vm->trace) fprintf(vm->trace, "# EEGFOLD %s set=%u digit=%ld\n",
    id && id[0] ? id : "eeg", (unsigned)m->set, dig);
}

void cubalc_lang_do_eegdemo(VM *vm, const char *id){
  ensure_world(vm);
  Var *_vg_nch = var_get(vm, "EEG_CH", 0);
  long nch = (_vg_nch && !_vg_nch->is_str) ? _vg_nch->val : 0;
  if (nch < 1) nch = CUBALC_EEG_DEF_CH;
  if (nch > CUBALC_EEG_MAX_CH) nch = CUBALC_EEG_MAX_CH;
  Var *_vg_seed = var_get(vm, "EEG_SEED", 0);
  long seed = (_vg_seed && !_vg_seed->is_str) ? _vg_seed->val : 0;
  if (seed == 0) seed = 0xC0BEA160L;
  cubalc_eeg_frame fr;
  cubalc_eeg_demo_frame(&fr, (int)nch, seed);
  Var *_vg_sc = var_get(vm, "EEG_SCALE", 0);
  long sc = (_vg_sc && !_vg_sc->is_str) ? _vg_sc->val : 0;
  if (sc > 0) fr.scale_uv = (float)sc;
  cubalc_matrix m;
  if (cubalc_eeg_pack_matrix(&fr, &m) != 0) {
    var_set_num(vm, "OK", 0);
    return;
  }
  var_set_num(vm, "EEG_CH", (long)fr.n_ch);
  var_set_num(vm, "EEG_WIN", (long)fr.n_samp);
  {
    Var *_vg_seq = var_get(vm, "EEG_SEQ", 0);
    long _seq = (_vg_seq && !_vg_seq->is_str) ? _vg_seq->val : 0;
    var_set_num(vm, "EEG_SEQ", _seq + 1);
  }
  do_eeg_fold_matrix(vm, id && id[0] ? id : "eeg", &m);
}

void cubalc_lang_do_eegpack_csv(VM *vm, const char *id, const char *csv){
  ensure_world(vm);
  Var *_vg_nch = var_get(vm, "EEG_CH", 0);
  long nch = (_vg_nch && !_vg_nch->is_str) ? _vg_nch->val : 0;
  if (nch < 1) nch = CUBALC_EEG_DEF_CH;
  Var *_vg_sc = var_get(vm, "EEG_SCALE", 0);
  long sc = (_vg_sc && !_vg_sc->is_str) ? _vg_sc->val : 0;
  float scale = sc > 0 ? (float)sc : CUBALC_EEG_DEF_SCALE;
  cubalc_matrix m;
  if (cubalc_eeg_pack_csv_line(csv, (int)nch, scale, &m) != 0) {
    var_set_num(vm, "OK", 0);
    var_set_str(vm, "LAST_ERR", "EEGPACK need CSV sample line");
    return;
  }
  {
    Var *_vg_seq = var_get(vm, "EEG_SEQ", 0);
    long _seq = (_vg_seq && !_vg_seq->is_str) ? _vg_seq->val : 0;
    var_set_num(vm, "EEG_SEQ", _seq + 1);
  }
  do_eeg_fold_matrix(vm, id && id[0] ? id : "eeg", &m);
}

void cubalc_lang_do_eegread(VM *vm, const char *id){
  ensure_world(vm);
  const char *path = NULL;
  /* EEG_PATH var preferred */
  for (int i = 0; i < vm->n_vars; i++) {
    if (vm->vars[i].is_str &&
        (strcmp(vm->vars[i].name, "EEG_PATH") == 0 ||
         strcmp(vm->vars[i].name, "eeg_path") == 0)) {
      path = vm->vars[i].sval;
      break;
    }
  }
  if (!path || !path[0]) {
    const char *env = getenv("CUBALC_EEG_PATH");
    if (env && env[0]) path = env;
  }
  if (!path || !path[0]) {
    var_set_num(vm, "OK", 0);
    var_set_str(vm, "LAST_ERR", "EEGREAD need EEG_PATH or CUBALC_EEG_PATH");
    return;
  }
  Var *_vg_nch = var_get(vm, "EEG_CH", 0);
  long nch = (_vg_nch && !_vg_nch->is_str) ? _vg_nch->val : 0;
  if (nch < 1) nch = CUBALC_EEG_DEF_CH;
  Var *_vg_sc = var_get(vm, "EEG_SCALE", 0);
  long sc = (_vg_sc && !_vg_sc->is_str) ? _vg_sc->val : 0;
  float scale = sc > 0 ? (float)sc : CUBALC_EEG_DEF_SCALE;
  Var *_vg_win = var_get(vm, "EEG_WIN", 0);
  long win = (_vg_win && !_vg_win->is_str) ? _vg_win->val : 0;
  if (win < 1) win = 16;
  if (win > CUBALC_EEG_MAX_WIN) win = CUBALC_EEG_MAX_WIN;

  FILE *fp = fopen(path, "rb");
  if (!fp) {
    var_set_num(vm, "OK", 0);
    var_set_str(vm, "LAST_ERR", "EEGREAD open failed");
    return;
  }
  cubalc_eeg_frame fr;
  cubalc_eeg_frame_init(&fr, (int)nch, scale);
  float ch[CUBALC_EEG_MAX_CH];
  cubalc_matrix m;
  int packed = 0;
  for (int i = 0; i < (int)win * 4; i++) {
    int rc = cubalc_eeg_read_csv_sample(fp, ch, (int)nch);
    if (rc < 0) break;
    if (rc > 0) continue;
    int pr = cubalc_eeg_window_push(&fr, ch, (int)nch, (int)win, 0, &m);
    if (pr == 1) { packed = 1; break; }
  }
  if (!packed && fr.n_samp > 0) {
    if (cubalc_eeg_window_push(&fr, ch, (int)nch, (int)win, 1, &m) == 1)
      packed = 1;
  }
  fclose(fp);
  if (!packed) {
    var_set_num(vm, "OK", 0);
    var_set_str(vm, "LAST_ERR", "EEGREAD no samples");
    return;
  }
  var_set_num(vm, "EEG_CH", (long)fr.n_ch);
  var_set_num(vm, "EEG_WIN", (long)fr.n_samp);
  {
    Var *_vg_seq = var_get(vm, "EEG_SEQ", 0);
    long _seq = (_vg_seq && !_vg_seq->is_str) ? _vg_seq->val : 0;
    var_set_num(vm, "EEG_SEQ", _seq + 1);
  }
  do_eeg_fold_matrix(vm, id && id[0] ? id : "eeg", &m);
}

/* Braincube decide: State Matrix → algocube digit 0..9 into var DECIDE + cube digit */
long cubalc_lang_do_decide(VM *vm, const char *id){
  ensure_world(vm);
  int ix = id && id[0] ? find_cube(vm,id) : -1;
  if (ix<0){
    /* prefer brain / braincube / algocube / hive */
    const char *cands[]={"brain","braincube","algo","algocube","hive","sot",NULL};
    for (int i=0;cands[i];i++){ ix=find_cube(vm,cands[i]); if(ix>=0) break; }
    if (ix<0 && vm->ch.n_cubes>0) ix=0;
  }
  if (ix<0) return 4; /* hail nexus default digit */
  cubalc_cube *c=&vm->ch.cubes[ix];
  /* matrix is SoT — recompute digit; lock result as braincube decision */
  c->atom.digit = (uint8_t)cubalc_algocube_digit(&c->atom.matrix);
  c->atom.digit_lock = 1;
  long d = c->atom.digit;
  long *slot = var_slot(vm, "DECIDE", 1);
  if (slot) *slot = d;
  long *slot2 = var_slot(vm, "decide", 1);
  if (slot2) *slot2 = d;
  if (vm->trace) fprintf(vm->trace,"# DECIDE %s → %ld (%s)\n",
    c->id, d, CUBALC_DIGIT_TAG[d%10]);
  long *slot3 = var_slot(vm, "DIGIT", 1);
  if (slot3) *slot3 = d;
  return d;
}

/* COMPARE a b — Hamming / unity / XOR digit (free-flow law). */
long cubalc_lang_do_compare(VM *vm, const char *ida, const char *idb){
  ensure_world(vm);
  int ia = find_cube(vm, ida), ib = find_cube(vm, idb);
  if (ia < 0 || ib < 0) return 0;
  cubalc_algo_cmp cmp;
  if (cubalc_algocube_compare(&vm->ch.cubes[ia].atom.matrix,
                              &vm->ch.cubes[ib].atom.matrix, &cmp) != 0)
    return 0;
  vm->ch.cubes[ia].atom.unity = cmp.unity;
  vm->ch.cubes[ib].atom.unity = cmp.unity;
  vm->ch.unity = cmp.unity;
  long u = (long)lround(cmp.unity * 100.0);
  long *su = var_slot(vm, "UNITY", 1); if (su) *su = u;
  long *sh = var_slot(vm, "HAMMING", 1); if (sh) *sh = cmp.hamming;
  long *sa = var_slot(vm, "AGREE", 1); if (sa) *sa = cmp.agree;
  long *sc = var_slot(vm, "COMPAT", 1); if (sc) *sc = u;
  long *sd = var_slot(vm, "DIGIT", 1); if (sd) *sd = cmp.digit;
  long *sx = var_slot(vm, "COMPARE", 1); if (sx) *sx = u;
  if (vm->trace) fprintf(vm->trace,
    "# COMPARE %s~%s hamming=%d unity=%.4f digit=%d\n",
    ida, idb, cmp.hamming, cmp.unity, cmp.digit);
  return u;
}

/* HARMONY [target] — majority-vote hive consensus; inject into target cube. */
long cubalc_lang_do_harmony(VM *vm, const char *target){
  ensure_world(vm);
  cubalc_algo_harm h;
  if (cubalc_algocube_chain_harmony(&vm->ch, &h) != 0 || !h.ok) {
    long *so = var_slot(vm, "HARMONY", 1); if (so) *so = 0;
    return 0;
  }
  long u = (long)lround(h.unity * 100.0);
  long *su = var_slot(vm, "UNITY", 1); if (su) *su = u;
  long *sh = var_slot(vm, "HARMONY", 1); if (sh) *sh = u;
  long *sd = var_slot(vm, "DIGIT", 1); if (sd) *sd = h.digit;
  long *sc = var_slot(vm, "CONSENSUS", 1); if (sc) *sc = h.consensus.set;
  long *sn = var_slot(vm, "HIVE_N", 1); if (sn) *sn = h.n;
  const char *tid = (target && target[0]) ? target : "hive";
  int ix = find_cube(vm, tid);
  if (ix < 0) { place_cube(vm, tid, "algocube_harmony", 1); ix = find_cube(vm, tid); }
  if (ix >= 0)
    cubalc_algocube_inject(&vm->ch.cubes[ix], &h.consensus, h.digit);
  const char *st = getenv("CUBALC_STATE");
  if (st && st[0]) {
    char path[512];
    snprintf(path, sizeof path, "%s/harmony.json", st);
    FILE *f = fopen(path, "w");
    if (f) { cubalc_algocube_harmony_json(&h, f); fclose(f); }
  }
  if (vm->trace) fprintf(vm->trace,
    "# HARMONY n=%d unity=%.4f digit=%d → %s\n", h.n, h.unity, h.digit, tid);
  if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print,
                        "harmony n=%d unity=%ld digit=%d", h.n, u, h.digit);
  return u;
}

/* RESOLVE [target] — The Cube demands: harmony + decide + energy pulse */
long cubalc_lang_do_resolve(VM *vm, const char *target){
  ensure_world(vm);
  long u = do_harmony(vm, target);
  long d = do_decide(vm, target && target[0] ? target : "brain");
  /* energy must flow: pulse create-protons after resolve */
  for (int i = 0; i < vm->ch.n_cubes; i++) {
    if (vm->ch.cubes[i].atom.proton && vm->ch.cubes[i].atom.alive) {
      vm->ch.cubes[i].atom.energy = fminf(1.f, vm->ch.cubes[i].atom.energy + 0.12f);
    }
  }
  do_flow(vm, 2);
  long e = 0;
  for (int i = 0; i < vm->ch.n_cubes; i++)
    e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
  long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
  long *sr = var_slot(vm, "RESOLVE", 1); if (sr) *sr = d;
  if (vm->trace) fprintf(vm->trace, "# RESOLVE unity=%ld decide=%ld energy=%ld\n", u, d, e);
  if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print,
                        "resolve d=%ld u=%ld e=%ld", d, u, e);
  return d;
}

int cubalc_lang_exec_stmts_until(VM *vm, Lex *L, const char *stop1, const char *stop2);

/* Scan one token toward block end. Parks on matching END (depth→0) or UNTIL (if allow_until).
 * Skips BREAK IF / CONTINUE IF / JUMP/JZ family so guarded IF does not nest the block. */
int cubalc_lang_block_scan_step(Lex *L, int *depth, int allow_until){
  if (L->cur.kind==TK_EOF) return 1;
  if (kw(&L->cur,"BREAK")||kw(&L->cur,"BREAKIF")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"CONTINUEIF")||
      kw(&L->cur,"CONTIF")||kw(&L->cur,"NEXT")||kw(&L->cur,"NEXTIF")||kw(&L->cur,"SKIP")||kw(&L->cur,"SKIPIF")||
      kw(&L->cur,"JUMP")||kw(&L->cur,"JMP")||kw(&L->cur,"JZ")||kw(&L->cur,"JZERO")||
      kw(&L->cur,"JNZ")||kw(&L->cur,"JNEZ")||kw(&L->cur,"JTRUE")||
      kw(&L->cur,"CJZ")||kw(&L->cur,"CJZERO")||kw(&L->cur,"CJNZ")||kw(&L->cur,"CJNEZ")||
      kw(&L->cur,"BEQ")||kw(&L->cur,"BNE")||kw(&L->cur,"BLT")||kw(&L->cur,"BLE")||
      kw(&L->cur,"BGT")||kw(&L->cur,"BGE")||
      kw(&L->cur,"CBEQ")||kw(&L->cur,"CBNE")||kw(&L->cur,"CBLT")||kw(&L->cur,"CBLE")||
      kw(&L->cur,"CBGT")||kw(&L->cur,"CBGE")||
      kw(&L->cur,"SJUMP")||kw(&L->cur,"SBREAK")||kw(&L->cur,"SCONTINUE")||
      kw(&L->cur,"SJZ")||kw(&L->cur,"SJZERO")||kw(&L->cur,"SJNZ")||kw(&L->cur,"SJNEZ")||
      kw(&L->cur,"SCJZ")||kw(&L->cur,"SCJNZ")||
      kw(&L->cur,"SBEQ")||kw(&L->cur,"SBNE")||kw(&L->cur,"SBLT")||kw(&L->cur,"SBLE")||
      kw(&L->cur,"SBGT")||kw(&L->cur,"SBGE")||
      kw(&L->cur,"SCBEQ")||kw(&L->cur,"SCBNE")||kw(&L->cur,"SCBLT")||kw(&L->cur,"SCBLE")||
      kw(&L->cur,"SCBGT")||kw(&L->cur,"SCBGE")){
    lex_next(L);
    if (kw(&L->cur,"IF")) lex_next(L);
    return 0;
  }
  if (allow_until && *depth==1 && kw(&L->cur,"UNTIL")) return 1;
  /* Note: METHOD is NOT a body block opener here — CLASS/METHOD bodies are
   * scanned with their own depth at CLASS parse. Treating METHOD as a block
   * would break EACH METHOD … END inside METHOD bodies (double depth). */
  if (kw(&L->cur,"FN")||kw(&L->cur,"FUNC")||kw(&L->cur,"FUNCTION")||kw(&L->cur,"DEF")||
      kw(&L->cur,"CLASS")||kw(&L->cur,"TYPE")||
      kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||
      kw(&L->cur,"IFERR")||kw(&L->cur,"IFOK")||kw(&L->cur,"IFEMPTY")||kw(&L->cur,"IFNONEMPTY")||kw(&L->cur,"IFDEFINED")||kw(&L->cur,"IFUNDEF")||kw(&L->cur,"IFSTARTS")||kw(&L->cur,"IFENDS")||kw(&L->cur,"IFCONTAINS")||kw(&L->cur,"IFHAS")||kw(&L->cur,"IFEQS")||kw(&L->cur,"IFSTARTSI")||kw(&L->cur,"IFENDSI")||kw(&L->cur,"IFCONTAINSI")||kw(&L->cur,"IFHASI")||kw(&L->cur,"IFEQSI")||kw(&L->cur,"IFGT")||kw(&L->cur,"IFLT")||kw(&L->cur,"IFGE")||kw(&L->cur,"IFLE")||kw(&L->cur,"IFEQN")||kw(&L->cur,"IFNEQN")||kw(&L->cur,"IFHASLINE")||kw(&L->cur,"IFINBAG")||kw(&L->cur,"WHENHASLINE")||kw(&L->cur,"WHENINBAG")||kw(&L->cur,"IFBETWEEN")||kw(&L->cur,"IFINRANGE")||kw(&L->cur,"IFWITHIN")||kw(&L->cur,"WHENBETWEEN")||kw(&L->cur,"IFFN")||kw(&L->cur,"IFCLASS")||kw(&L->cur,"IFHASFN")||kw(&L->cur,"WHENFN")||kw(&L->cur,"WHENCLASS")||kw(&L->cur,"IFOBJ")||kw(&L->cur,"IFHASOBJ")||kw(&L->cur,"IFMETHOD")||kw(&L->cur,"IFHASMETHOD")||kw(&L->cur,"WHENOBJ")||kw(&L->cur,"WHENMETHOD")||kw(&L->cur,"IFFILE")||kw(&L->cur,"IFDIR")||kw(&L->cur,"IFEXIST")||kw(&L->cur,"IFEXISTS")||kw(&L->cur,"WHENFILE")||kw(&L->cur,"WHENDIR")||kw(&L->cur,"WHENEXIST")||kw(&L->cur,"IFENV")||kw(&L->cur,"WHENENV")||kw(&L->cur,"IFHASENV")||kw(&L->cur,"WHENHASENV")||kw(&L->cur,"IFFIELD")||kw(&L->cur,"IFHASFIELD")||kw(&L->cur,"WHENFIELD")||kw(&L->cur,"WHENHASFIELD")||kw(&L->cur,"IFFRESH")||kw(&L->cur,"IFSTALE")||kw(&L->cur,"WHENFRESH")||kw(&L->cur,"WHENSTALE")||kw(&L->cur,"IFBIG")||kw(&L->cur,"IFSMALL")||kw(&L->cur,"IFSIZE")||kw(&L->cur,"WHENBIG")||kw(&L->cur,"WHENSMALL")||kw(&L->cur,"WHENSIZE")||kw(&L->cur,"IFNEWER")||kw(&L->cur,"IFOLDER")||kw(&L->cur,"IFOUTOFDATE")||kw(&L->cur,"WHENNEWER")||kw(&L->cur,"WHENOLDER")||kw(&L->cur,"WHENOUTOFDATE")||kw(&L->cur,"IFEQFILE")||kw(&L->cur,"IFDIFFILE")||kw(&L->cur,"IFSAMEFILE")||kw(&L->cur,"WHENEQFILE")||kw(&L->cur,"WHENDIFFILE")||kw(&L->cur,"WHENSAMEFILE")||kw(&L->cur,"IFJSONHAS")||kw(&L->cur,"IFJSONMISS")||kw(&L->cur,"IFHASJSON")||kw(&L->cur,"WHENJSONHAS")||kw(&L->cur,"WHENJSONMISS")||kw(&L->cur,"WHENHASJSON")||kw(&L->cur,"IFJSONHASALL")||kw(&L->cur,"IFJSONHASANY")||kw(&L->cur,"IFJSONNEED")||kw(&L->cur,"IFHASJSONALL")||kw(&L->cur,"IFHASJSONANY")||kw(&L->cur,"WHENJSONHASALL")||kw(&L->cur,"WHENJSONHASANY")||kw(&L->cur,"WHENJSONNEED")||kw(&L->cur,"WHENHASJSONALL")||kw(&L->cur,"WHENHASJSONANY")||kw(&L->cur,"IFONLYP")||kw(&L->cur,"IFNOEXTRAP")||kw(&L->cur,"IFJSONONLYP")||kw(&L->cur,"IFSTRICTP")||kw(&L->cur,"WHENONLYP")||kw(&L->cur,"WHENNOEXTRAP")||kw(&L->cur,"IFEXACTP")||kw(&L->cur,"IFSCHEMAP")||kw(&L->cur,"IFJSONEXACTP")||kw(&L->cur,"WHENEXACTP")||kw(&L->cur,"WHENSCHEMAP")||kw(&L->cur,"IFMISSP")||kw(&L->cur,"IFABSENTP")||kw(&L->cur,"IFMISSINGKEYSP")||kw(&L->cur,"WHENMISSP")||kw(&L->cur,"WHENABSENTP")||kw(&L->cur,"WHENMISSINGKEYSP")||kw(&L->cur,"IFEXTRAP")||kw(&L->cur,"IFUNKNOWNKEYSP")||kw(&L->cur,"IFUNKNOWNKEYS")||kw(&L->cur,"WHENEXTRAP")||kw(&L->cur,"WHENUNKNOWNKEYSP")||kw(&L->cur,"WHENUNKNOWNKEYS")||kw(&L->cur,"IFEMPTYP")||kw(&L->cur,"IFISEMPTYP")||kw(&L->cur,"IFEMPTYPLATE")||kw(&L->cur,"IFJSONEMPTY")||kw(&L->cur,"WHENEMPTYP")||kw(&L->cur,"WHENISEMPTYP")||kw(&L->cur,"WHENEMPTYPLATE")||kw(&L->cur,"WHENJSONEMPTY")||kw(&L->cur,"IFNONEMPTYP")||kw(&L->cur,"IFHASKEYSP")||kw(&L->cur,"IFNONEMPTYPLATE")||kw(&L->cur,"IFJSONNONEMPTY")||kw(&L->cur,"WHENNONEMPTYP")||kw(&L->cur,"WHENHASKEYSP")||kw(&L->cur,"WHENNONEMPTYPLATE")||kw(&L->cur,"WHENJSONNONEMPTY")||kw(&L->cur,"IFONEP")||kw(&L->cur,"IFSINGLEP")||kw(&L->cur,"IFONEKEYP")||kw(&L->cur,"WHENONEP")||kw(&L->cur,"WHENSINGLEP")||kw(&L->cur,"WHENONEKEYP")||kw(&L->cur,"IFMULTP")||kw(&L->cur,"IFMANYKEYSP")||kw(&L->cur,"IFMULTIKEYP")||kw(&L->cur,"WHENMULTP")||kw(&L->cur,"WHENMANYKEYSP")||kw(&L->cur,"WHENMULTIKEYP")||kw(&L->cur,"IFPAIRP")||kw(&L->cur,"IFTWOP")||kw(&L->cur,"IFTWOKEYP")||kw(&L->cur,"WHENPAIRP")||kw(&L->cur,"WHENTWOP")||kw(&L->cur,"WHENTWOKEYP")||kw(&L->cur,"IFTRIPLEP")||kw(&L->cur,"IFTHREEP")||kw(&L->cur,"IFTHREEKEYP")||kw(&L->cur,"WHENTRIPLEP")||kw(&L->cur,"WHENTHREEP")||kw(&L->cur,"WHENTHREEKEYP")||kw(&L->cur,"IFQUADP")||kw(&L->cur,"IFFOURP")||kw(&L->cur,"IFFOURKEYP")||kw(&L->cur,"WHENQUADP")||kw(&L->cur,"WHENFOURP")||kw(&L->cur,"WHENFOURKEYP")||kw(&L->cur,"IFQUINTP")||kw(&L->cur,"IFFIVEP")||kw(&L->cur,"IFFIVEKEYP")||kw(&L->cur,"WHENQUINTP")||kw(&L->cur,"WHENFIVEP")||kw(&L->cur,"WHENFIVEKEYP")||kw(&L->cur,"IFNUM")||kw(&L->cur,"IFSTR")||kw(&L->cur,"IFTYPE")||kw(&L->cur,"IFVARTYPE")||kw(&L->cur,"IFKIND")||kw(&L->cur,"WHENNUM")||kw(&L->cur,"WHENSTR")||kw(&L->cur,"WHENTYPE")||kw(&L->cur,"WHENVARTYPE")||kw(&L->cur,"IFZERO")||kw(&L->cur,"IFNZ")||kw(&L->cur,"IFPOS")||kw(&L->cur,"IFNEG")||kw(&L->cur,"WHENZERO")||kw(&L->cur,"WHENNZ")||kw(&L->cur,"WHENPOS")||kw(&L->cur,"WHENNEG")||kw(&L->cur,"IFCANREAD")||kw(&L->cur,"IFCANWRITE")||kw(&L->cur,"IFCANEXEC")||kw(&L->cur,"WHENCANREAD")||kw(&L->cur,"WHENCANWRITE")||kw(&L->cur,"WHENCANEXEC")||kw(&L->cur,"IFJSONEQ")||kw(&L->cur,"IFJSONNEQ")||kw(&L->cur,"IFSAMEJSON")||kw(&L->cur,"WHENJSONEQ")||kw(&L->cur,"WHENJSONNEQ")||kw(&L->cur,"WHENSAMEJSON")||kw(&L->cur,"IFBLANK")||kw(&L->cur,"IFNOTBLANK")||kw(&L->cur,"WHENBLANK")||kw(&L->cur,"WHENNOTBLANK")||kw(&L->cur,"IFLEN")||kw(&L->cur,"IFLONG")||kw(&L->cur,"IFSHORT")||kw(&L->cur,"WHENLEN")||kw(&L->cur,"WHENLONG")||kw(&L->cur,"WHENSHORT")||kw(&L->cur,"IFLINES")||kw(&L->cur,"IFMANY")||kw(&L->cur,"IFFEW")||kw(&L->cur,"WHENLINES")||kw(&L->cur,"WHENMANY")||kw(&L->cur,"WHENFEW")||kw(&L->cur,"IFHASFLAG")||kw(&L->cur,"IFNOFLAG")||kw(&L->cur,"IFHASARG")||kw(&L->cur,"IFNOARG")||kw(&L->cur,"WHENHASFLAG")||kw(&L->cur,"WHENNOFLAG")||kw(&L->cur,"WHENHASARG")||kw(&L->cur,"WHENNOARG")||kw(&L->cur,"WHENERR")||kw(&L->cur,"WHENOK")||
      kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FOREACH")||
      kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||kw(&L->cur,"FOREACHCELL")||
      kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FOREACHBIT")||
      kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||
      kw(&L->cur,"TIMEIT")||kw(&L->cur,"BENCH")||kw(&L->cur,"ELAPSED")||
      kw(&L->cur,"TIMING")||kw(&L->cur,"MEASURE")||
      kw(&L->cur,"RETRY")||kw(&L->cur,"ATTEMPT")||kw(&L->cur,"TRIES")||
      kw(&L->cur,"TRY")||kw(&L->cur,"GUARD")||kw(&L->cur,"WITHCLEANUP")||
      kw(&L->cur,"ENSURE")){
    (*depth)++;
    lex_next(L);
    return 0;
  }
  if (kw(&L->cur,"END")){
    (*depth)--;
    if (*depth==0) return 1; /* leave END for caller */
    lex_next(L);
    return 0;
  }
  lex_next(L);
  return 0;
}

/* legacy verbose still works (CREED, CUBE, …) so old plates run */
long cubalc_lang_parse_expr(VM *vm, Lex *L); /* minimal for ASSERT */
long cubalc_lang_parse_prim(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind==TK_NUM){ long v=L->cur.num; lex_next(L); return v; }
  if (L->cur.kind==TK_MINUS){ lex_next(L); return -parse_prim(vm,L); }
  if (L->cur.kind==TK_LPAREN){
    lex_next(L); long v=parse_expr(vm,L);
    if (L->cur.kind==TK_RPAREN) lex_next(L);
    return v;
  }
  if (L->cur.kind==TK_IDENT){
    char name[96]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    if (strcmp(name,"CUBES")==0) return vm->ch.n_cubes;
    if (strcmp(name,"WORKERS")==0) return cubalc_async_workers();
    if (strcmp(name,"GPU")==0) return cubalc_async_gpu_ok();
    if (strcmp(name,"HTTP_CODE")==0) return vm->last_code;
    if (strcmp(name,"SMX_OK")==0){
      Var *vv=var_get(vm,"SMX_OK",0); return vv?vv->val:(long)vm->smx_ok;
    }
    if (strcmp(name,"SMX_TALKS")==0){
      Var *vv=var_get(vm,"SMX_TALKS",0); return vv?vv->val:(long)vm->smx_talks;
    }
    if (strcmp(name,"SMX_N")==0){
      Var *vv=var_get(vm,"SMX_N",0); return vv?vv->val:0;
    }
    if (strcmp(name,"OK")==0){ Var *vv=var_get(vm,"OK",0); return vv?vv->val:0; }
    if (strcmp(name,"EXIT")==0){ Var *vv=var_get(vm,"EXIT",0); return vv?vv->val:0; }
    if (strcmp(name,"LAST_N")==0){ Var *vn=var_get(vm,"LAST_N",0); if(vn) return vn->val; return vm->last_n; }
    if (strcmp(name,"SP")==0 || strcmp(name,"STACKLEN")==0) return (long)vm->sp;
    if (strcmp(name,"CELLS")==0) return (long)CUBALC_CELL_N;
    if (strcmp(name,"STRLEN")==0){
      if (L->cur.kind==TK_LPAREN){ lex_next(L);
        long ln=0;
        if (L->cur.kind==TK_STR) ln=(long)strlen(L->cur.text);
        else if (L->cur.kind==TK_IDENT){ Var *sv=var_get(vm,L->cur.text,0); ln=sv?(sv->is_str?(long)strlen(sv->sval):sv->val):0; }
        if (L->cur.kind==TK_STR||L->cur.kind==TK_IDENT) lex_next(L);
        if (L->cur.kind==TK_RPAREN) lex_next(L);
        return ln;
      }
      return vm->last_n;
    }

    if (strcmp(name,"UNITY")==0) return (long)lround(vm->ch.unity*100);
    if (strcmp(name,"SEQ")==0) return (long)vm->ch.seq;
    /* Pure-science public-domain constants (integer scales) */
    if (strcmp(name,"PI100")==0 || strcmp(name,"PI")==0) return CUBALC_SCI_PI100;
    if (strcmp(name,"E100")==0 || strcmp(name,"EULER")==0) return CUBALC_SCI_E100;
    if (strcmp(name,"G_EARTH")==0 || strcmp(name,"GEARTH")==0) return CUBALC_SCI_G_EARTH10;
    if (strcmp(name,"C_LIGHT")==0 || strcmp(name,"CLIGHT")==0) return CUBALC_SCI_C_LIGHT;
    if (strcmp(name,"ATM_KPA")==0) return CUBALC_SCI_ATM_KPA;
    if (strcmp(name,"WATER_K")==0 || strcmp(name,"ZERO_C_K")==0) return CUBALC_SCI_WATER_K;
    if (strcmp(name,"H2O_BP")==0) return CUBALC_SCI_H2O_BP_C;
    if (strcmp(name,"R_GAS")==0) return CUBALC_SCI_R_J;
    if (strcmp(name,"NA_ORDER")==0) return CUBALC_SCI_AVOGADRO_E23;
    if (strcmp(name,"EARTH_R")==0 || strcmp(name,"EARTH_R_KM")==0) return CUBALC_SCI_EARTH_R_KM;
    if (strcmp(name,"AU_KM")==0 || strcmp(name,"AU")==0) return CUBALC_SCI_AU_KM;
    if (strcmp(name,"YEAR_D")==0) return CUBALC_SCI_YEAR_D;
    if (strcmp(name,"MOON_D")==0) return CUBALC_SCI_MOON_D;
    if (strcmp(name,"SOLAR_C")==0) return CUBALC_SCI_SOLAR_C;
    if (strcmp(name,"ATM_O2")==0) return CUBALC_SCI_ATM_O2_PCT;
    if (strcmp(name,"ATM_N2")==0) return CUBALC_SCI_ATM_N2_PCT;
    /* Math / science pure functions — pure-science language plane */
    if (strcmp(name,"ABS")==0 || strcmp(name,"SIGN")==0 ||
        strcmp(name,"SQRT")==0 || strcmp(name,"ISQRT")==0 ||
        strcmp(name,"MIN")==0 || strcmp(name,"MAX")==0 ||
        strcmp(name,"POW")==0 || strcmp(name,"GCD")==0 ||
        strcmp(name,"LCM")==0 || strcmp(name,"FACT")==0 ||
        strcmp(name,"FORCE")==0 || strcmp(name,"WORK")==0 ||
        strcmp(name,"KE")==0 || strcmp(name,"PE")==0 ||
        strcmp(name,"DENSITY")==0 || strcmp(name,"MOLAR")==0 ||
        strcmp(name,"CELSIUS_K")==0 || strcmp(name,"KELVIN_C")==0 ||
        strcmp(name,"PH_H")==0 || strcmp(name,"CLAMP")==0 ||
        strcmp(name,"BOUND")==0 ||
        /* digit-1 clamp/bound: saturating arith + modular wrap */
        strcmp(name,"SATADD")==0 || strcmp(name,"SATSUB")==0 ||
        strcmp(name,"SATMUL")==0 || strcmp(name,"SATDIV")==0 ||
        strcmp(name,"WRAPMOD")==0 || strcmp(name,"WMOD")==0 ||
        strcmp(name,"CLIP8")==0 || strcmp(name,"CLIP16")==0 ||
        /* digit-1 overflow predicates + single-bit ops */
        strcmp(name,"ADDOVF")==0 || strcmp(name,"ADDOVER")==0 ||
        strcmp(name,"SUBOVF")==0 || strcmp(name,"SUBOVER")==0 ||
        strcmp(name,"MULOVF")==0 || strcmp(name,"MULOVER")==0 ||
        /* digit-2 multiword arith: ADDC SUBB DIVMOD (carry/borrow/rem) */
        strcmp(name,"ADDC")==0 || strcmp(name,"ADC")==0 ||
        strcmp(name,"SUBB")==0 || strcmp(name,"SBB")==0 ||
        strcmp(name,"DIVMOD")==0 || strcmp(name,"DIVREM")==0 ||
        strcmp(name,"BTEST")==0 || strcmp(name,"BITTEST")==0 ||
        strcmp(name,"BSET")==0 || strcmp(name,"BITSET")==0 ||
        strcmp(name,"BCLR")==0 || strcmp(name,"BITCLR")==0 ||
        strcmp(name,"BFLIP")==0 || strcmp(name,"BITFLIP")==0 ||
        strcmp(name,"BTGL")==0 || strcmp(name,"BITTGL")==0 ||
        /* digit-1 word parity */
        strcmp(name,"PARITY")==0 || strcmp(name,"PAR")==0 ||
        strcmp(name,"ODDPAR")==0 || strcmp(name,"EVENPAR")==0 ||
        strcmp(name,"AVG")==0 || strcmp(name,"PCT")==0 ||
        strcmp(name,"CIRC")==0 || strcmp(name,"AREA_CIRCLE")==0 ||
        strcmp(name,"HYP")==0 || strcmp(name,"WAVE_V")==0 ||
        strcmp(name,"LIGHT_T")==0 || strcmp(name,"BOYLE_P2")==0 ||
        strcmp(name,"ORBIT_PERIOD")==0 ||
        /* universal bit ops (word forms — | used by play dialect) */
        strcmp(name,"BAND")==0 || strcmp(name,"BOR")==0 ||
        strcmp(name,"BXOR")==0 || strcmp(name,"BNOT")==0 ||
        /* digit-1 bitwise logic + bit-select blend */
        strcmp(name,"BNAND")==0 || strcmp(name,"BNOR")==0 ||
        strcmp(name,"BXNOR")==0 || strcmp(name,"XNOR")==0 ||
        strcmp(name,"ANDN")==0 || strcmp(name,"ANDNOT")==0 ||
        strcmp(name,"BIC")==0 ||
        strcmp(name,"ORN")==0 || strcmp(name,"ORNOT")==0 ||
        strcmp(name,"BSEL")==0 || strcmp(name,"BITSEL")==0 ||
        strcmp(name,"BLEND")==0 ||
        strcmp(name,"SHL")==0 || strcmp(name,"SHR")==0 ||
        strcmp(name,"SAR")==0 || strcmp(name,"ASHR")==0 ||
        strcmp(name,"BITCOUNT")==0 || strcmp(name,"POPCNT")==0 ||
        strcmp(name,"POPCOUNT")==0 || strcmp(name,"HAMMING32")==0 ||
        /* digit-1 bit metrics: FFS FLS CLO CTO BWIDTH */
        strcmp(name,"FFS")==0 || strcmp(name,"FINDLS")==0 ||
        strcmp(name,"FLS")==0 || strcmp(name,"MSB")==0 || strcmp(name,"FINDMSB")==0 ||
        strcmp(name,"CLO")==0 || strcmp(name,"CTO")==0 ||
        strcmp(name,"BWIDTH")==0 || strcmp(name,"BITWIDTH")==0 ||
        /* digit-9 universal data-path: rotate · pack · select */
        strcmp(name,"ROTL")==0 || strcmp(name,"ROTR")==0 ||
        strcmp(name,"ROL")==0 || strcmp(name,"ROR")==0 ||
        /* digit-1 rotate/shift extend: 8/16 rot + 32-bit shift/sext/zext */
        strcmp(name,"ROTL8")==0 || strcmp(name,"ROL8")==0 ||
        strcmp(name,"ROTR8")==0 || strcmp(name,"ROR8")==0 ||
        strcmp(name,"ROTL16")==0 || strcmp(name,"ROL16")==0 ||
        strcmp(name,"ROTR16")==0 || strcmp(name,"ROR16")==0 ||
        strcmp(name,"SHL32")==0 || strcmp(name,"SHR32")==0 ||
        strcmp(name,"SEXT32")==0 || strcmp(name,"SEXTL")==0 ||
        strcmp(name,"ZEXT32")==0 || strcmp(name,"ZEXTL")==0 ||
        /* digit-1 shift/rotate/cmp: 64-bit rotate + unsigned/3-way cmp */
        strcmp(name,"ROTL64")==0 || strcmp(name,"ROL64")==0 ||
        strcmp(name,"ROTR64")==0 || strcmp(name,"ROR64")==0 ||
        strcmp(name,"ICMP")==0 || strcmp(name,"CMP3")==0 ||
        strcmp(name,"UCMP")==0 || strcmp(name,"UCMP3")==0 ||
        strcmp(name,"ULT")==0 || strcmp(name,"UGT")==0 ||
        strcmp(name,"ULE")==0 || strcmp(name,"UGE")==0 ||
        /* digit-1 boolean logic + signed compare predicates */
        strcmp(name,"LAND")==0 || strcmp(name,"LOR")==0 ||
        strcmp(name,"LXOR")==0 || strcmp(name,"LNOT")==0 ||
        strcmp(name,"IMPLY")==0 || strcmp(name,"IMPLIES")==0 ||
        strcmp(name,"EQZ")==0 || strcmp(name,"ISZERO")==0 ||
        strcmp(name,"NEZ")==0 || strcmp(name,"ISNZ")==0 || strcmp(name,"NONZERO")==0 ||
        strcmp(name,"EQ")==0 || strcmp(name,"NE")==0 ||
        strcmp(name,"LT")==0 || strcmp(name,"LE")==0 ||
        strcmp(name,"GT")==0 || strcmp(name,"GE")==0 ||
        strcmp(name,"PACK16")==0 || strcmp(name,"PACK")==0 ||
        strcmp(name,"HI16")==0 || strcmp(name,"LO16")==0 ||
        strcmp(name,"HIWORD")==0 || strcmp(name,"LOWORD")==0 ||
        strcmp(name,"ISEL")==0 || strcmp(name,"SELECT")==0 ||
        /* digit-1 select/clamp family: range predicates + median */
        strcmp(name,"BETWEEN")==0 || strcmp(name,"INRANGE")==0 ||
        strcmp(name,"WITHIN")==0 ||
        strcmp(name,"MEDIAN")==0 || strcmp(name,"MID3")==0 ||
        strcmp(name,"NEG")==0 ||
        strcmp(name,"CELL")==0 || strcmp(name,"SLOT")==0 ||
        /* digit-1 mem control: set/inc/dec/xchg as expressions */
        strcmp(name,"SETCELL")==0 || strcmp(name,"PUTCELL")==0 ||
        strcmp(name,"STORE")==0 || strcmp(name,"POKE")==0 ||
        strcmp(name,"INCCELL")==0 || strcmp(name,"INCELL")==0 ||
        strcmp(name,"DECCELL")==0 || strcmp(name,"DECELL")==0 ||
        strcmp(name,"XCHGCELL")==0 || strcmp(name,"EXCHCELL")==0 ||
        strcmp(name,"SWAPC")==0 ||
        strcmp(name,"PEEK")==0 || strcmp(name,"STACKLEN")==0 ||
        strcmp(name,"SP")==0 ||
        strcmp(name,"SUMCELL")==0 || strcmp(name,"MINCELL")==0 ||
        strcmp(name,"MAXCELL")==0 ||
        strcmp(name,"FINDCELL")==0 || strcmp(name,"COUNTCELL")==0 ||
        strcmp(name,"MINIDX")==0 || strcmp(name,"ARGMIN")==0 ||
        strcmp(name,"MAXIDX")==0 || strcmp(name,"ARGMAX")==0 ||
        strcmp(name,"RAND")==0 || strcmp(name,"RND")==0 ||
        strcmp(name,"IRAND")==0 ||
        strcmp(name,"SEED")==0 || strcmp(name,"SETSEED")==0 ||
        strcmp(name,"RNG")==0 || strcmp(name,"GETSEED")==0 ||
        /* digit-2 math: modular + number theory + integer bit/log */
        strcmp(name,"ADDMOD")==0 || strcmp(name,"SUBMOD")==0 ||
        strcmp(name,"MULMOD")==0 || strcmp(name,"POWMOD")==0 ||
        strcmp(name,"FIB")==0 || strcmp(name,"FIBONACCI")==0 ||
        strcmp(name,"ISPRIME")==0 || strcmp(name,"PRIMEP")==0 ||
        /* digit-2 roots / primes / perfect powers */
        strcmp(name,"IROOT")==0 || strcmp(name,"NTHROOT")==0 ||
        strcmp(name,"ISSQUARE")==0 || strcmp(name,"ISQUARE")==0 ||
        strcmp(name,"ISSQR")==0 ||
        strcmp(name,"ISCUBE")==0 || strcmp(name,"ISCUB")==0 ||
        strcmp(name,"NEXTPRIME")==0 || strcmp(name,"NXTPRIME")==0 ||
        strcmp(name,"PREVPRIME")==0 || strcmp(name,"PRVPRIME")==0 ||
        /* digit-2 divisor arithmetic: NDIVS SIGMA PHI */
        strcmp(name,"NDIVS")==0 || strcmp(name,"NUMDIV")==0 ||
        strcmp(name,"DIVCOUNT")==0 || strcmp(name,"TAUD")==0 ||
        strcmp(name,"SIGMA")==0 || strcmp(name,"DIVSUM")==0 ||
        strcmp(name,"SIGMA1")==0 ||
        /* digit-2 abundance class: ALIQUOT ISPERFECT ISABUNDANT ISDEFICIENT */
        strcmp(name,"ALIQUOT")==0 || strcmp(name,"PROPERSIGMA")==0 ||
        strcmp(name,"S0")==0 || strcmp(name,"SIGMA0STAR")==0 ||
        strcmp(name,"ISPERFECT")==0 || strcmp(name,"PERFECTP")==0 ||
        strcmp(name,"ISABUNDANT")==0 || strcmp(name,"ABUNDANTP")==0 ||
        strcmp(name,"ISDEFICIENT")==0 || strcmp(name,"DEFICIENTP")==0 ||
        strcmp(name,"PHI")==0 || strcmp(name,"TOTIENT")==0 ||
        strcmp(name,"EULERPHI")==0 ||
        /* digit-2 number theory: MOBIUS RADICAL SQUAREFREE COPRIME CEILPOW2 */
        strcmp(name,"MOBIUS")==0 || strcmp(name,"MU")==0 ||
        strcmp(name,"RADICAL")==0 || strcmp(name,"RAD")==0 ||
        strcmp(name,"ISSQUAREFREE")==0 || strcmp(name,"SQFREE")==0 ||
        strcmp(name,"SQUAREFREE")==0 ||
        strcmp(name,"COPRIME")==0 || strcmp(name,"ISCOPRIME")==0 ||
        strcmp(name,"CEILPOW2")==0 || strcmp(name,"NEXTPOW2")==0 ||
        strcmp(name,"CPOW2")==0 ||
        /* digit-2 modular duals: JACOBI LEGENDRE MODDIV SPF */
        strcmp(name,"JACOBI")==0 || strcmp(name,"LEGENDRE")==0 ||
        strcmp(name,"MODDIV")==0 || strcmp(name,"DIVMODM")==0 ||
        strcmp(name,"SPF")==0 || strcmp(name,"SMALLPF")==0 ||
        strcmp(name,"MINPF")==0 ||
        /* digit-2 factor metrics: VALUATION OMEGA OMEGA0 */
        strcmp(name,"VALUATION")==0 || strcmp(name,"PVAL")==0 ||
        strcmp(name,"VP")==0 ||
        strcmp(name,"OMEGA")==0 || strcmp(name,"BIGOMEGA")==0 ||
        strcmp(name,"OMEGA0")==0 || strcmp(name,"LITTLEOMEGA")==0 ||
        strcmp(name,"NUOMEGA")==0 ||
        /* digit-2 modular order: SOPF CARMICHAEL LAMBDA ORDER */
        strcmp(name,"SOPF")==0 || strcmp(name,"SOPFR")==0 ||
        strcmp(name,"CARMICHAEL")==0 || strcmp(name,"LAMBDA")==0 ||
        strcmp(name,"CARM")==0 ||
        strcmp(name,"ORDER")==0 || strcmp(name,"MULTORDER")==0 ||
        /* digit-2 modular ext: SQPART ISPRIMITIVE */
        strcmp(name,"SQPART")==0 || strcmp(name,"LARGESQ")==0 ||
        strcmp(name,"MAXSQ")==0 ||
        strcmp(name,"ISPRIMITIVE")==0 || strcmp(name,"ISPRROOT")==0 ||
        strcmp(name,"PRIMROOTP")==0 ||
        /* digit-2 primes/powers: ISPOWER ISPRIMEPOWER NTHPRIME */
        strcmp(name,"ISPOWER")==0 || strcmp(name,"PERFPOW")==0 ||
        strcmp(name,"ISPOW")==0 ||
        strcmp(name,"ISPRIMEPOWER")==0 || strcmp(name,"IPP")==0 ||
        strcmp(name,"PRIMEPOWERP")==0 ||
        strcmp(name,"NTHPRIME")==0 || strcmp(name,"PRIMEN")==0 ||
        strcmp(name,"PRIMEK")==0 ||
        /* digit-2 prime metrics: PRIMECOUNT PRIMEGAP ISCOMPOSITE */
        strcmp(name,"PRIMECOUNT")==0 || strcmp(name,"PRIMEPI")==0 ||
        strcmp(name,"PIN")==0 || strcmp(name,"COUNTPRIMES")==0 ||
        strcmp(name,"PRIMEGAP")==0 || strcmp(name,"PGAP")==0 ||
        strcmp(name,"NEXTGAP")==0 ||
        strcmp(name,"ISCOMPOSITE")==0 || strcmp(name,"COMPOSITEP")==0 ||
        strcmp(name,"COMPP")==0 ||
        strcmp(name,"IDIV")==0 || strcmp(name,"IMOD")==0 ||
        /* digit-0/2 muldiv: unsigned div + high multiply */
        strcmp(name,"UDIV")==0 || strcmp(name,"UDIVIDE")==0 ||
        strcmp(name,"UMOD")==0 || strcmp(name,"UREM")==0 ||
        strcmp(name,"MULHI")==0 || strcmp(name,"MULH")==0 ||
        strcmp(name,"UMULHI")==0 || strcmp(name,"UMULH")==0 ||
        strcmp(name,"ILOG2")==0 || strcmp(name,"LOG2")==0 ||
        strcmp(name,"ILOG10")==0 || strcmp(name,"LOG10")==0 ||
        strcmp(name,"ODD")==0 || strcmp(name,"EVEN")==0 ||
        strcmp(name,"CTZ")==0 || strcmp(name,"CLZ")==0 ||
        strcmp(name,"NTZ")==0 || strcmp(name,"NLZ")==0 ||
        strcmp(name,"ISPOW2")==0 || strcmp(name,"POW2")==0 ||
        strcmp(name,"POW10")==0 || strcmp(name,"TENPOW")==0 ||
        strcmp(name,"NDIGITS")==0 || strcmp(name,"DIGSUM")==0 ||
        strcmp(name,"MODINV")==0 || strcmp(name,"INVMOD")==0 ||
        /* digit-0 foundation: bitfield extract/deposit + ceil div + mask */
        strcmp(name,"BEXT")==0 || strcmp(name,"BITEXT")==0 ||
        strcmp(name,"BDEP")==0 || strcmp(name,"BITDEP")==0 ||
        strcmp(name,"BYTE")==0 || strcmp(name,"HIBYTE")==0 ||
        strcmp(name,"LOBYTE")==0 ||
        strcmp(name,"MASK")==0 || strcmp(name,"BITMASK")==0 ||
        strcmp(name,"ISDIV")==0 || strcmp(name,"DIVISIBLE")==0 ||
        strcmp(name,"DIVCEIL")==0 || strcmp(name,"CEILDIV")==0 ||
        /* digit-1 data path: word reverse / parity / nibble */
        strcmp(name,"BSWAP")==0 || strcmp(name,"BSWAP32")==0 ||
        strcmp(name,"BSWAP16")==0 || strcmp(name,"BSWAP64")==0 ||
        strcmp(name,"BITREV")==0 || strcmp(name,"REVBITS")==0 ||
        strcmp(name,"PARITY")==0 ||
        strcmp(name,"NIBBLE")==0 || strcmp(name,"NIB")==0 ||
        strcmp(name,"DIST")==0 || strcmp(name,"ABSDIFF")==0 ||
        /* digit-8 sign/zero extend data path */
        strcmp(name,"SEXT")==0 || strcmp(name,"SIGNEXT")==0 ||
        strcmp(name,"ZEXT")==0 || strcmp(name,"ZEROEXT")==0 ||
        strcmp(name,"SEXT8")==0 || strcmp(name,"SEXTB")==0 ||
        strcmp(name,"SEXT16")==0 || strcmp(name,"SEXTW")==0 ||
        /* digit-8 pack byte/nibble + set nibble */
        strcmp(name,"PACK8")==0 || strcmp(name,"PACKB")==0 ||
        strcmp(name,"PACKNIB")==0 || strcmp(name,"PACK4")==0 ||
        strcmp(name,"SETNIB")==0 || strcmp(name,"SETNIBBLE")==0 ||
        strcmp(name,"SETBYTE")==0 || strcmp(name,"SETB")==0 ||
        /* digit-5 align / round-to-multiple */
        strcmp(name,"ALIGN")==0 || strcmp(name,"ROUNDUP")==0 ||
        strcmp(name,"ALIGNDN")==0 || strcmp(name,"ROUNDDN")==0 ||
        /* digit-2 math ext: combinatorics + square + floor div */
        strcmp(name,"SQR")==0 || strcmp(name,"SQUARE")==0 ||
        strcmp(name,"BINOM")==0 || strcmp(name,"CHOOSE")==0 ||
        strcmp(name,"PERM")==0 || strcmp(name,"PNR")==0 ||
        strcmp(name,"DIVFLOOR")==0 || strcmp(name,"FLOORDIV")==0){
      if (L->cur.kind==TK_LPAREN){
        lex_next(L);
        long a = parse_expr(vm,L);
        long b = 0, c = 0;
        if (L->cur.kind==TK_COMMA){ lex_next(L); b = parse_expr(vm,L); }
        if (L->cur.kind==TK_COMMA){ lex_next(L); c = parse_expr(vm,L); }
        if (L->cur.kind==TK_RPAREN) lex_next(L);
        if (strcmp(name,"ABS")==0) return a < 0 ? -a : a;
        if (strcmp(name,"SIGN")==0) return a > 0 ? 1 : (a < 0 ? -1 : 0);
        if (strcmp(name,"SQRT")==0 || strcmp(name,"ISQRT")==0){
          if (a < 0) return 0;
          long r = 0;
          while ((r+1)*(r+1) <= a) r++;
          return r;
        }
        if (strcmp(name,"MIN")==0) return a < b ? a : b;
        if (strcmp(name,"MAX")==0) return a > b ? a : b;
        if (strcmp(name,"POW")==0){
          long e = b; if (e < 0) return 0;
          long r = 1;
          while (e-- > 0) r *= a;
          return r;
        }
        if (strcmp(name,"GCD")==0){
          long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
          while (y){ long t = x % y; x = y; y = t; }
          return x;
        }
        if (strcmp(name,"LCM")==0){
          long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
          if (!x || !y) return 0;
          long g = x, h = y;
          while (h){ long t = g % h; g = h; h = t; }
          return (x / g) * y;
        }
        if (strcmp(name,"FACT")==0){
          if (a < 0) return 0;
          if (a > 20) a = 20; /* stay in long */
          long r = 1;
          for (long i = 2; i <= a; i++) r *= i;
          return r;
        }
        /* Physics (integer): F=ma, W=Fd, KE=mv²/2, PE=mgh (g scaled ×10 → divide 10) */
        if (strcmp(name,"FORCE")==0) return a * b;           /* m * a */
        if (strcmp(name,"WORK")==0) return a * b;            /* F * d */
        if (strcmp(name,"KE")==0) return (a * b * b) / 2;    /* m v v / 2 */
        if (strcmp(name,"PE")==0){
          /* PE(m,h) → m·g·h with g×10; PE(m,g10,h) if third arg present (c!=0 or b used as g) */
          long g10 = CUBALC_SCI_G_EARTH10;
          long h = b;
          /* 3-arg form: PE(m, g10, h) when user passes three numbers; detect via c!=0 OR
             convention: if only two args, b is height. Third arg always sets c from parse. */
          if (c) { g10 = b; h = c; }
          return (a * g10 * h) / 10;
        }
        if (strcmp(name,"DENSITY")==0) return b ? (a / b) : 0; /* m/V */
        if (strcmp(name,"MOLAR")==0) return b ? (a / b) : 0;   /* n = N/NA_order or mass/M */
        if (strcmp(name,"CELSIUS_K")==0) return a + CUBALC_SCI_WATER_K;
        if (strcmp(name,"KELVIN_C")==0) return a - CUBALC_SCI_WATER_K;
        /* pH proxy: pH = -log10[H+]; integer H_scaled e.g. 1e-3 → use H_pow = 3 → pH 3 */
        if (strcmp(name,"PH_H")==0) return a; /* pass-through: user supplies exponent as pH law */
        if (strcmp(name,"CLAMP")==0 || strcmp(name,"BOUND")==0){
          /* CLAMP/BOUND(x, lo, hi) */
          long lo = b, hi = c;
          if (hi < lo) { long t = lo; lo = hi; hi = t; }
          if (a < lo) return lo;
          if (a > hi) return hi;
          return a;
        }
        if (strcmp(name,"SATADD")==0){
          /* SATADD(a,b) — add with signed long saturation */
          if (b > 0 && a > LONG_MAX - b) return LONG_MAX;
          if (b < 0 && a < LONG_MIN - b) return LONG_MIN;
          return a + b;
        }
        if (strcmp(name,"SATSUB")==0){
          /* SATSUB(a,b) — subtract with signed long saturation */
          if (b > 0 && a < LONG_MIN + b) return LONG_MIN;
          if (b < 0 && a > LONG_MAX + b) return LONG_MAX;
          return a - b;
        }
        if (strcmp(name,"SATMUL")==0){
          /* SATMUL(a,b) — multiply with signed long saturation */
          if (a == 0 || b == 0) return 0;
          {
            __int128 p = (__int128)a * (__int128)b;
            if (p > (__int128)LONG_MAX) return LONG_MAX;
            if (p < (__int128)LONG_MIN) return LONG_MIN;
            return (long)p;
          }
        }
        if (strcmp(name,"SATDIV")==0){
          /* SATDIV(a,b) — trunc-toward-zero; /0 → 0; LONG_MIN/-1 → LONG_MAX */
          if (b == 0) return 0;
          if (a == LONG_MIN && b == -1) return LONG_MAX;
          return a / b;
        }
        if (strcmp(name,"WRAPMOD")==0 || strcmp(name,"WMOD")==0){
          /* WRAPMOD(n, m) — n mod m in [0, m); m<=0 → 0 */
          if (b <= 0) return 0;
          long r = a % b;
          if (r < 0) r += b;
          return r;
        }
        if (strcmp(name,"CLIP8")==0){
          /* CLIP8(n) — clamp to unsigned 8-bit range [0,255] */
          if (a < 0) return 0;
          if (a > 255) return 255;
          return a;
        }
        if (strcmp(name,"CLIP16")==0){
          /* CLIP16(n) — clamp to unsigned 16-bit range [0,65535] */
          if (a < 0) return 0;
          if (a > 65535) return 65535;
          return a;
        }
        if (strcmp(name,"ADDOVF")==0 || strcmp(name,"ADDOVER")==0){
          /* ADDOVF(a,b) → 1 if signed a+b overflows long */
          if (b > 0 && a > LONG_MAX - b) return 1;
          if (b < 0 && a < LONG_MIN - b) return 1;
          return 0;
        }
        if (strcmp(name,"SUBOVF")==0 || strcmp(name,"SUBOVER")==0){
          /* SUBOVF(a,b) → 1 if signed a-b overflows long */
          if (b > 0 && a < LONG_MIN + b) return 1;
          if (b < 0 && a > LONG_MAX + b) return 1;
          return 0;
        }
        if (strcmp(name,"MULOVF")==0 || strcmp(name,"MULOVER")==0){
          /* MULOVF(a,b) → 1 if signed a*b overflows long */
          if (a == 0 || b == 0) return 0;
          {
            __int128 p = (__int128)a * (__int128)b;
            if (p > (__int128)LONG_MAX || p < (__int128)LONG_MIN) return 1;
            return 0;
          }
        }
        if (strcmp(name,"ADDC")==0 || strcmp(name,"ADC")==0){
          /* ADDC(a,b[,cin]) — unsigned wrap add; sets CARRY 0/1; returns sum */
          unsigned long ua = (unsigned long)a;
          unsigned long ub = (unsigned long)b;
          unsigned long uc = (unsigned long)(c != 0 ? 1 : 0);
          unsigned long s = ua + ub;
          int c1 = (s < ua) ? 1 : 0;
          unsigned long r = s + uc;
          int c2 = (r < s) ? 1 : 0;
          int carry = c1 | c2;
          var_set_num(vm, "CARRY", carry);
          var_set_num(vm, "CY", carry);
          return (long)r;
        }
        if (strcmp(name,"SUBB")==0 || strcmp(name,"SBB")==0){
          /* SUBB(a,b[,bin]) — unsigned wrap sub; sets BORROW 0/1; returns diff */
          unsigned long ua = (unsigned long)a;
          unsigned long ub = (unsigned long)b;
          unsigned long uin = (unsigned long)(c != 0 ? 1 : 0);
          int b1 = (ua < ub) ? 1 : 0;
          unsigned long d = ua - ub;
          int b2 = (d < uin) ? 1 : 0;
          unsigned long r = d - uin;
          int borrow = b1 | b2;
          var_set_num(vm, "BORROW", borrow);
          var_set_num(vm, "BW", borrow);
          var_set_num(vm, "CARRY", borrow); /* alias: borrow as carry-in chain */
          return (long)r;
        }
        if (strcmp(name,"DIVMOD")==0 || strcmp(name,"DIVREM")==0){
          /* DIVMOD(a,b) — return quot; set QUOT REM; b==0 → 0/0 OK soft */
          if (b == 0){
            var_set_num(vm, "QUOT", 0);
            var_set_num(vm, "REM", 0);
            var_set_num(vm, "OK", 0);
            return 0;
          }
          long q = a / b;
          long r = a % b;
          var_set_num(vm, "QUOT", q);
          var_set_num(vm, "REM", r);
          var_set_num(vm, "OK", 1);
          return q;
        }
        if (strcmp(name,"BTEST")==0 || strcmp(name,"BITTEST")==0){
          /* BTEST(val, k) → 1 if bit k set (k clamped 0..63) */
          long k = b;
          if (k < 0) k = 0;
          if (k > 63) k = 63;
          return (((unsigned long)a >> (unsigned)k) & 1ul) ? 1 : 0;
        }
        if (strcmp(name,"PARITY")==0 || strcmp(name,"PAR")==0 ||
            strcmp(name,"ODDPAR")==0){
          /* PARITY(a) → popcount(a) mod 2 (odd parity of bits) */
          unsigned long u = (unsigned long)a;
          int p = 0;
          while (u){ p ^= (int)(u & 1ul); u >>= 1; }
          return p;
        }
        if (strcmp(name,"EVENPAR")==0){
          unsigned long u = (unsigned long)a;
          int p = 0;
          while (u){ p ^= (int)(u & 1ul); u >>= 1; }
          return p ? 0 : 1;
        }
        if (strcmp(name,"BSET")==0 || strcmp(name,"BITSET")==0){
          /* BSET(val, k) → val with bit k set */
          long k = b;
          if (k < 0) k = 0;
          if (k > 63) k = 63;
          return (long)((unsigned long)a | (1ul << (unsigned)k));
        }
        if (strcmp(name,"BCLR")==0 || strcmp(name,"BITCLR")==0){
          /* BCLR(val, k) → val with bit k cleared */
          long k = b;
          if (k < 0) k = 0;
          if (k > 63) k = 63;
          return (long)((unsigned long)a & ~(1ul << (unsigned)k));
        }
        if (strcmp(name,"BFLIP")==0 || strcmp(name,"BITFLIP")==0 ||
            strcmp(name,"BTGL")==0 || strcmp(name,"BITTGL")==0){
          /* BFLIP(val, k) → val with bit k toggled */
          long k = b;
          if (k < 0) k = 0;
          if (k > 63) k = 63;
          return (long)((unsigned long)a ^ (1ul << (unsigned)k));
        }
        if (strcmp(name,"AVG")==0) return (a + b) / 2;
        if (strcmp(name,"PCT")==0) return b ? (a * 100 / b) : 0; /* a is what % of b */
        if (strcmp(name,"CIRC")==0) return 2 * CUBALC_SCI_PI100 * a / 100; /* 2πr scaled */
        if (strcmp(name,"AREA_CIRCLE")==0) return CUBALC_SCI_PI100 * a * a / 100; /* πr² scaled */
        if (strcmp(name,"HYP")==0){ /* integer hypotenuse √(a²+b²) */
          long s = a*a + b*b;
          if (s < 0) return 0;
          long r = 0;
          while ((r+1)*(r+1) <= s) r++;
          return r;
        }
        if (strcmp(name,"WAVE_V")==0) return a * b; /* f * λ */
        if (strcmp(name,"LIGHT_T")==0) return b ? (a / b) : 0; /* dist / c → seconds if SI */
        if (strcmp(name,"BOYLE_P2")==0) return c ? (a * b / c) : 0; /* P1*V1/V2 */
        if (strcmp(name,"ORBIT_PERIOD")==0){
          /* Kepler-ish scale: T² ∝ a³ → T ~ a * sqrt(a) / k ; use T = a for unit AU demo */
          if (a <= 0) return 0;
          long aa = a * a * a;
          long r = 0;
          while ((r+1)*(r+1) <= aa) r++;
          return r; /* rough √(a³) for integer AU */
        }
        /* Universal integer bit algebra */
        if (strcmp(name,"BAND")==0) return a & b;
        if (strcmp(name,"BOR")==0) return a | b;
        if (strcmp(name,"BXOR")==0) return a ^ b;
        if (strcmp(name,"BNOT")==0) return ~a;
        if (strcmp(name,"BNAND")==0) return ~(a & b);
        if (strcmp(name,"BNOR")==0) return ~(a | b);
        if (strcmp(name,"BXNOR")==0 || strcmp(name,"XNOR")==0) return ~(a ^ b);
        if (strcmp(name,"ANDN")==0 || strcmp(name,"ANDNOT")==0 || strcmp(name,"BIC")==0)
          return a & ~b; /* bit clear: a AND NOT b */
        if (strcmp(name,"ORN")==0 || strcmp(name,"ORNOT")==0)
          return a | ~b;
        if (strcmp(name,"BSEL")==0 || strcmp(name,"BITSEL")==0 || strcmp(name,"BLEND")==0){
          /* BSEL(mask, a, b) — bit blend: (a & mask) | (b & ~mask) */
          unsigned long m = (unsigned long)a;
          unsigned long x = (unsigned long)b;
          unsigned long y = (unsigned long)c;
          return (long)((x & m) | (y & ~m));
        }
        if (strcmp(name,"SHL")==0){
          if (b < 0) b = 0; if (b > 62) b = 62;
          return a << b;
        }
        if (strcmp(name,"SHR")==0){
          if (b < 0) b = 0; if (b > 62) b = 62;
          return (long)((unsigned long)a >> (unsigned)b);
        }
        if (strcmp(name,"SAR")==0 || strcmp(name,"ASHR")==0){
          /* SAR/ASHR(a,n) — arithmetic (sign-preserving) right shift */
          if (b < 0) b = 0; if (b > 62) b = 62;
          return a >> b;
        }
        if (strcmp(name,"BITCOUNT")==0 || strcmp(name,"POPCNT")==0 ||
            strcmp(name,"POPCOUNT")==0){
          unsigned long u = (unsigned long)a; int n = 0;
          while (u) { n += (int)(u & 1u); u >>= 1; }
          return n;
        }
        if (strcmp(name,"HAMMING32")==0){
          unsigned long u = (unsigned long)(a ^ b); int n = 0;
          while (u) { n += (int)(u & 1u); u >>= 1; }
          return n;
        }
        /* Rotate (32-bit width — portable universal word) */
        if (strcmp(name,"ROTL")==0 || strcmp(name,"ROL")==0){
          unsigned int w = (unsigned int)a;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 31;
          if (k == 0) return (long)w;
          return (long)((w << k) | (w >> (32 - k)));
        }
        if (strcmp(name,"ROTR")==0 || strcmp(name,"ROR")==0){
          unsigned int w = (unsigned int)a;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 31;
          if (k == 0) return (long)w;
          return (long)((w >> k) | (w << (32 - k)));
        }
        if (strcmp(name,"ROTL8")==0 || strcmp(name,"ROL8")==0){
          unsigned int w = (unsigned int)a & 0xFFu;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 7;
          if (k == 0) return (long)w;
          return (long)(((w << k) | (w >> (8 - k))) & 0xFFu);
        }
        if (strcmp(name,"ROTR8")==0 || strcmp(name,"ROR8")==0){
          unsigned int w = (unsigned int)a & 0xFFu;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 7;
          if (k == 0) return (long)w;
          return (long)(((w >> k) | (w << (8 - k))) & 0xFFu);
        }
        if (strcmp(name,"ROTL16")==0 || strcmp(name,"ROL16")==0){
          unsigned int w = (unsigned int)a & 0xFFFFu;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 15;
          if (k == 0) return (long)w;
          return (long)(((w << k) | (w >> (16 - k))) & 0xFFFFu);
        }
        if (strcmp(name,"ROTR16")==0 || strcmp(name,"ROR16")==0){
          unsigned int w = (unsigned int)a & 0xFFFFu;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 15;
          if (k == 0) return (long)w;
          return (long)(((w >> k) | (w << (16 - k))) & 0xFFFFu);
        }
        if (strcmp(name,"SHL32")==0){
          /* 32-bit logical left shift (result zero-extended to long) */
          unsigned int w = (unsigned int)a;
          int k = (int)b;
          if (k < 0) k = 0;
          if (k >= 32) return 0;
          return (long)(w << k);
        }
        if (strcmp(name,"SHR32")==0){
          /* 32-bit logical right shift */
          unsigned int w = (unsigned int)a;
          int k = (int)b;
          if (k < 0) k = 0;
          if (k >= 32) return 0;
          return (long)(w >> k);
        }
        if (strcmp(name,"SEXT32")==0 || strcmp(name,"SEXTL")==0){
          /* sign-extend low 32 bits to signed long */
          long v = (long)(unsigned int)a;
          if (v & 0x80000000L) v |= ~0xFFFFFFFFL;
          else v &= 0xFFFFFFFFL;
          /* portable: cast through int32 */
          return (long)(int)(unsigned int)a;
        }
        if (strcmp(name,"ZEXT32")==0 || strcmp(name,"ZEXTL")==0){
          /* zero-extend low 32 bits */
          return (long)((unsigned long)a & 0xFFFFFFFFul);
        }
        if (strcmp(name,"ROTL64")==0 || strcmp(name,"ROL64")==0){
          /* 64-bit rotate left on unsigned long word */
          unsigned long w = (unsigned long)a;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 63;
          if (k == 0) return (long)w;
          return (long)((w << k) | (w >> (64 - k)));
        }
        if (strcmp(name,"ROTR64")==0 || strcmp(name,"ROR64")==0){
          /* 64-bit rotate right on unsigned long word */
          unsigned long w = (unsigned long)a;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 63;
          if (k == 0) return (long)w;
          return (long)((w >> k) | (w << (64 - k)));
        }
        if (strcmp(name,"ICMP")==0 || strcmp(name,"CMP3")==0){
          /* ICMP(a,b) → -1 if a<b, 0 if equal, 1 if a>b (signed) */
          if (a < b) return -1;
          if (a > b) return 1;
          return 0;
        }
        if (strcmp(name,"UCMP")==0 || strcmp(name,"UCMP3")==0){
          /* UCMP(a,b) → -1/0/1 as unsigned long compare */
          unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
          if (ua < ub) return -1;
          if (ua > ub) return 1;
          return 0;
        }
        if (strcmp(name,"ULT")==0)
          return ((unsigned long)a < (unsigned long)b) ? 1 : 0;
        if (strcmp(name,"UGT")==0)
          return ((unsigned long)a > (unsigned long)b) ? 1 : 0;
        if (strcmp(name,"ULE")==0)
          return ((unsigned long)a <= (unsigned long)b) ? 1 : 0;
        if (strcmp(name,"UGE")==0)
          return ((unsigned long)a >= (unsigned long)b) ? 1 : 0;
        /* digit-1 boolean logic (normalize to 0/1) + signed compare predicates */
        if (strcmp(name,"LAND")==0) return (a && b) ? 1 : 0;
        if (strcmp(name,"LOR")==0) return (a || b) ? 1 : 0;
        if (strcmp(name,"LXOR")==0) return ((a != 0) ^ (b != 0)) ? 1 : 0;
        if (strcmp(name,"LNOT")==0) return a ? 0 : 1;
        if (strcmp(name,"IMPLY")==0 || strcmp(name,"IMPLIES")==0)
          return (!a || b) ? 1 : 0;
        if (strcmp(name,"EQZ")==0 || strcmp(name,"ISZERO")==0) return a ? 0 : 1;
        if (strcmp(name,"NEZ")==0 || strcmp(name,"ISNZ")==0 || strcmp(name,"NONZERO")==0)
          return a ? 1 : 0;
        if (strcmp(name,"EQ")==0) return (a == b) ? 1 : 0;
        if (strcmp(name,"NE")==0) return (a != b) ? 1 : 0;
        if (strcmp(name,"LT")==0) return (a < b) ? 1 : 0;
        if (strcmp(name,"LE")==0) return (a <= b) ? 1 : 0;
        if (strcmp(name,"GT")==0) return (a > b) ? 1 : 0;
        if (strcmp(name,"GE")==0) return (a >= b) ? 1 : 0;
        /* Pack / unpack 16-bit halves → 32-bit word */
        if (strcmp(name,"PACK16")==0 || strcmp(name,"PACK")==0){
          unsigned int hi = (unsigned int)a & 0xFFFFu;
          unsigned int lo = (unsigned int)b & 0xFFFFu;
          return (long)((hi << 16) | lo);
        }
        if (strcmp(name,"HI16")==0 || strcmp(name,"HIWORD")==0)
          return (long)(((unsigned int)a >> 16) & 0xFFFFu);
        if (strcmp(name,"LO16")==0 || strcmp(name,"LOWORD")==0)
          return (long)((unsigned int)a & 0xFFFFu);
        /* ISEL(cond, then, else) — expression ternary (universal control in expr) */
        if (strcmp(name,"ISEL")==0 || strcmp(name,"SELECT")==0)
          return a ? b : c;
        /* digit-1 select/clamp: WITHIN half-open, BETWEEN inclusive, MEDIAN of 3 */
        if (strcmp(name,"WITHIN")==0){
          /* WITHIN(n, lo, hi) → 1 if lo <= n < hi (Forth-style) */
          return (a >= b && a < c) ? 1 : 0;
        }
        if (strcmp(name,"BETWEEN")==0 || strcmp(name,"INRANGE")==0){
          /* BETWEEN(n, lo, hi) → 1 if n in [lo,hi] (swap lo/hi if inverted) */
          long lo = b, hi = c;
          if (hi < lo){ long t = lo; lo = hi; hi = t; }
          return (a >= lo && a <= hi) ? 1 : 0;
        }
        if (strcmp(name,"MEDIAN")==0 || strcmp(name,"MID3")==0){
          /* MEDIAN(a,b,c) — middle of three (order-statistic select) */
          long x = a, y = b, z = c;
          if (x > y){ long t = x; x = y; y = t; }
          if (y > z){ long t = y; y = z; z = t; }
          if (x > y){ long t = x; x = y; y = t; }
          return y;
        }
        if (strcmp(name,"NEG")==0) return -a;
        if (strcmp(name,"CELL")==0 || strcmp(name,"SLOT")==0){
          if (a < 0) a = 0;
          if (a >= CUBALC_CELL_N) a = CUBALC_CELL_N - 1;
          return vm->cells[(int)a];
        }
        if (strcmp(name,"SETCELL")==0 || strcmp(name,"PUTCELL")==0 ||
            strcmp(name,"STORE")==0 || strcmp(name,"POKE")==0){
          /* SETCELL(i, v) — store v at cell i, return v */
          long i = a;
          if (i < 0) i = 0;
          if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
          vm->cells[(int)i] = b;
          return b;
        }
        if (strcmp(name,"INCCELL")==0 || strcmp(name,"INCELL")==0){
          /* INCCELL(i[, d]) — cells[i]+=d (default 1 when omitted), return new */
          long i = a;
          long step = (b == 0) ? 1 : b;
          if (i < 0) i = 0;
          if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
          vm->cells[(int)i] += step;
          return vm->cells[(int)i];
        }
        if (strcmp(name,"DECCELL")==0 || strcmp(name,"DECELL")==0){
          /* DECCELL(i[, d]) — cells[i]-=d (default 1), return new value */
          long i = a;
          long step = (b == 0) ? 1 : b;
          if (i < 0) i = 0;
          if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
          vm->cells[(int)i] -= step;
          return vm->cells[(int)i];
        }
        if (strcmp(name,"XCHGCELL")==0 || strcmp(name,"EXCHCELL")==0 ||
            strcmp(name,"SWAPC")==0){
          /* XCHGCELL(i, v) — swap cells[i] with v, return previous cells[i] */
          long i = a;
          if (i < 0) i = 0;
          if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
          long old = vm->cells[(int)i];
          vm->cells[(int)i] = b;
          return old;
        }
        if (strcmp(name,"SUMCELL")==0 || strcmp(name,"MINCELL")==0 ||
            strcmp(name,"MAXCELL")==0){
          long lo = a, hi = (b != 0 || L->cur.kind==TK_RPAREN) ? b : a;
          /* if only one arg, hi=a already; if two, b set */
          if (b == 0 && a == 0){ lo = 0; hi = CUBALC_CELL_N - 1; }
          else if (b == 0) hi = a;
          if (lo < 0) lo = 0;
          if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
          if (hi < lo){ long t=lo; lo=hi; hi=t; }
          long acc=0, mn=0, mx=0; int first=1;
          for (long i=lo;i<=hi;i++){
            long v=vm->cells[(int)i];
            acc += v;
            if (first){ mn=mx=v; first=0; }
            else { if (v<mn) mn=v; if (v>mx) mx=v; }
          }
          if (strcmp(name,"MINCELL")==0) return first?0:mn;
          if (strcmp(name,"MAXCELL")==0) return first?0:mx;
          return acc;
        }
        /* digit-5 cell memory: FINDCELL(val[,lo[,hi]]) · COUNTCELL(val[,lo[,hi]]) */
        if (strcmp(name,"FINDCELL")==0 || strcmp(name,"COUNTCELL")==0){
          long val = a;
          long lo = 0, hi = CUBALC_CELL_N - 1;
          if (b != 0 || c != 0){
            lo = b;
            hi = (c != 0) ? c : CUBALC_CELL_N - 1;
          }
          if (lo < 0) lo = 0;
          if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
          if (hi < lo){ long t=lo; lo=hi; hi=t; }
          if (strcmp(name,"FINDCELL")==0){
            for (long i=lo;i<=hi;i++)
              if (vm->cells[(int)i] == val) return i;
            return -1;
          }
          long cnt = 0;
          for (long i=lo;i<=hi;i++)
            if (vm->cells[(int)i] == val) cnt++;
          return cnt;
        }
        /* digit-9: MINIDX/ARGMIN(lo[,hi]) · MAXIDX/ARGMAX(lo[,hi]) */
        if (strcmp(name,"MINIDX")==0 || strcmp(name,"ARGMIN")==0 ||
            strcmp(name,"MAXIDX")==0 || strcmp(name,"ARGMAX")==0){
          long lo = a, hi = (b != 0) ? b : a;
          if (b == 0 && a == 0){ lo = 0; hi = CUBALC_CELL_N - 1; }
          else if (b == 0) hi = a;
          if (lo < 0) lo = 0;
          if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
          if (hi < lo){ long t=lo; lo=hi; hi=t; }
          int want_max = (strcmp(name,"MAXIDX")==0 || strcmp(name,"ARGMAX")==0);
          long best_i = lo, best_v = vm->cells[(int)lo];
          for (long i = lo + 1; i <= hi; i++){
            long v = vm->cells[(int)i];
            if (want_max){ if (v > best_v){ best_v = v; best_i = i; } }
            else { if (v < best_v){ best_v = v; best_i = i; } }
          }
          return best_i;
        }
        if (strcmp(name,"PEEK")==0){
          if (vm->sp <= 0) return 0;
          return vm->stack[vm->sp - 1];
        }
        if (strcmp(name,"STACKLEN")==0 || strcmp(name,"SP")==0)
          return (long)vm->sp;
        if (strcmp(name,"RAND")==0 || strcmp(name,"RND")==0 || strcmp(name,"IRAND")==0){
          /* xorshift32 → [0, a) if a>0 else 0..9 */
          uint32_t x = vm->rng;
          x ^= x << 13; x ^= x >> 17; x ^= x << 5;
          if (!x) x = 1;
          vm->rng = x;
          long m = a > 0 ? a : 10;
          return (long)(x % (uint32_t)m);
        }
        if (strcmp(name,"SEED")==0 || strcmp(name,"SETSEED")==0){
          /* SEED(n) — set RNG state; 0 maps to 1; return the seed used */
          uint32_t s = (uint32_t)a;
          if (!s) s = 1;
          vm->rng = s;
          return (long)s;
        }
        if (strcmp(name,"RNG")==0 || strcmp(name,"GETSEED")==0){
          /* current PRNG state (no advance); arg ignored */
          return (long)vm->rng;
        }
        /* Modular arithmetic (digit-2 math plane) */
        if (strcmp(name,"IDIV")==0) return b ? (a / b) : 0;
        if (strcmp(name,"IMOD")==0) return b ? (a % b) : 0;
        if (strcmp(name,"UDIV")==0 || strcmp(name,"UDIVIDE")==0){
          /* unsigned divide; divisor 0 → 0 */
          if (b == 0) return 0;
          return (long)((unsigned long)a / (unsigned long)b);
        }
        if (strcmp(name,"UMOD")==0 || strcmp(name,"UREM")==0){
          /* unsigned remainder; divisor 0 → 0 */
          if (b == 0) return 0;
          return (long)((unsigned long)a % (unsigned long)b);
        }
        if (strcmp(name,"MULHI")==0 || strcmp(name,"MULH")==0){
          /* high 64 bits of signed 64×64 → 128 product */
          __int128 p = (__int128)a * (__int128)b;
          return (long)(p >> 64);
        }
        if (strcmp(name,"UMULHI")==0 || strcmp(name,"UMULH")==0){
          /* high 64 bits of unsigned 64×64 product */
          unsigned __int128 p =
              (unsigned __int128)(unsigned long)a *
              (unsigned __int128)(unsigned long)b;
          return (long)(p >> 64);
        }
        if (strcmp(name,"ADDMOD")==0){
          long m = c; if (m <= 0) return 0;
          long x = a % m; if (x < 0) x += m;
          long y = b % m; if (y < 0) y += m;
          return (x + y) % m;
        }
        if (strcmp(name,"SUBMOD")==0){
          long m = c; if (m <= 0) return 0;
          long x = a % m; if (x < 0) x += m;
          long y = b % m; if (y < 0) y += m;
          return (x - y + m) % m;
        }
        if (strcmp(name,"MULMOD")==0){
          long m = c; if (m <= 0) return 0;
          long x = a % m; if (x < 0) x += m;
          long y = b % m; if (y < 0) y += m;
          /* careful multiply via binary for large values */
          long r = 0;
          while (y > 0){
            if (y & 1) r = (r + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          return r;
        }
        if (strcmp(name,"POWMOD")==0){
          long m = c; if (m <= 0) return 0;
          long base = a % m; if (base < 0) base += m;
          long exp = b; if (exp < 0) return 0;
          long r = 1 % m;
          while (exp > 0){
            if (exp & 1){
              long y = r, x = base, acc = 0;
              while (y > 0){
                if (y & 1) acc = (acc + x) % m;
                x = (x + x) % m;
                y >>= 1;
              }
              r = acc;
            }
            {
              long x = base, acc = 0, y = base;
              while (y > 0){
                if (y & 1) acc = (acc + x) % m;
                x = (x + x) % m;
                y >>= 1;
              }
              base = acc;
            }
            exp >>= 1;
          }
          return r;
        }
        if (strcmp(name,"FIB")==0 || strcmp(name,"FIBONACCI")==0){
          if (a <= 0) return 0;
          if (a == 1 || a == 2) return 1;
          if (a > 92) a = 92; /* stay in signed 64-bit */
          long f0 = 0, f1 = 1;
          for (long i = 2; i <= a; i++){
            long f2 = f0 + f1;
            f0 = f1; f1 = f2;
          }
          return f1;
        }
        if (strcmp(name,"ISPRIME")==0 || strcmp(name,"PRIMEP")==0){
          if (a <= 1) return 0;
          if (a <= 3) return 1;
          if ((a % 2) == 0 || (a % 3) == 0) return 0;
          for (long i = 5; i * i <= a; i += 6){
            if ((a % i) == 0 || (a % (i + 2)) == 0) return 0;
          }
          return 1;
        }
        if (strcmp(name,"IROOT")==0 || strcmp(name,"NTHROOT")==0){
          /* IROOT(a,k) — largest x s.t. x^k <= a (k>=1); a<0 only if k odd */
          long k = b;
          if (k < 1) return 0;
          if (k == 1) return a;
          int neg = 0;
          long n = a;
          if (n < 0){
            if ((k & 1L) == 0) return 0; /* even root of negative */
            neg = 1;
            n = -n;
          }
          if (n == 0 || n == 1) return neg ? -n : n;
          /* binary search in [1, n] */
          long lo = 1, hi = n, ans = 1;
          if (k == 2){
            /* faster isqrt: hi starts at min(n, 2^31-ish) */
            hi = n;
            if (hi > 3037000499L) hi = 3037000499L; /* sqrt(LLONG_MAX)~ */
          } else {
            /* bound hi so hi^k won't uselessly exceed */
            hi = n;
            if (hi > 1000000L && k >= 3) hi = 1000000L;
            if (k >= 4 && hi > 10000L) hi = 10000L;
            if (k >= 10 && hi > 100L) hi = 100L;
            if (k >= 40 && hi > 10L) hi = 10L;
            if (k >= 64) hi = 2;
          }
          while (lo <= hi){
            long mid = lo + (hi - lo) / 2;
            /* compute mid^k with overflow guard */
            __int128 p = 1;
            int ov = 0;
            for (long i = 0; i < k; i++){
              p *= (__int128)mid;
              if (p > (__int128)n){ ov = 1; break; }
            }
            if (!ov && p <= (__int128)n){
              ans = mid;
              lo = mid + 1;
            } else {
              hi = mid - 1;
            }
          }
          return neg ? -ans : ans;
        }
        if (strcmp(name,"ISSQUARE")==0 || strcmp(name,"ISQUARE")==0 ||
            strcmp(name,"ISSQR")==0){
          /* ISSQUARE(a) → 1 if a is perfect square (incl 0) */
          if (a < 0) return 0;
          if (a <= 1) return 1;
          long r = 0;
          {
            long lo = 1, hi = a;
            if (hi > 3037000499L) hi = 3037000499L;
            while (lo <= hi){
              long mid = lo + (hi - lo) / 2;
              __int128 p = (__int128)mid * (__int128)mid;
              if (p == (__int128)a){ r = 1; break; }
              if (p < (__int128)a) lo = mid + 1;
              else hi = mid - 1;
            }
          }
          return r;
        }
        if (strcmp(name,"ISCUBE")==0 || strcmp(name,"ISCUB")==0){
          if (a == 0 || a == 1 || a == -1) return 1;
          int neg = a < 0;
          long n = neg ? -a : a;
          long lo = 1, hi = n;
          if (hi > 2097151L) hi = 2097151L; /* cbrt(2^63) ~ */
          while (lo <= hi){
            long mid = lo + (hi - lo) / 2;
            __int128 p = (__int128)mid * mid * mid;
            if (p == (__int128)n) return 1;
            if (p < (__int128)n) lo = mid + 1;
            else hi = mid - 1;
          }
          return 0;
        }
        if (strcmp(name,"NEXTPRIME")==0 || strcmp(name,"NXTPRIME")==0){
          /* NEXTPRIME(n) — smallest prime strictly > n; guard search */
          long x = a + 1;
          if (x <= 2) return 2;
          if ((x & 1L) == 0) x++;
          for (long guard = 0; guard < 200000; guard++, x += 2){
            long t = x;
            int okp = 1;
            if (t <= 3) return t;
            if ((t % 3) == 0){ okp = 0; }
            else {
              for (long i = 5; i * i <= t; i += 6){
                if ((t % i) == 0 || (t % (i + 2)) == 0){ okp = 0; break; }
              }
            }
            if (okp) return t;
          }
          return 0;
        }
        if (strcmp(name,"PREVPRIME")==0 || strcmp(name,"PRVPRIME")==0){
          /* PREVPRIME(n) — largest prime strictly < n; 0 if none */
          if (a <= 2) return 0;
          if (a == 3) return 2;
          long x = a - 1;
          if ((x & 1L) == 0) x--;
          for (long guard = 0; guard < 200000 && x >= 2; guard++, x -= 2){
            long t = x;
            if (t == 2) return 2;
            if (t == 3) return 3;
            int okp = 1;
            if ((t % 3) == 0) okp = 0;
            else {
              for (long i = 5; i * i <= t; i += 6){
                if ((t % i) == 0 || (t % (i + 2)) == 0){ okp = 0; break; }
              }
            }
            if (okp) return t;
          }
          return 0;
        }
        if (strcmp(name,"NDIVS")==0 || strcmp(name,"NUMDIV")==0 ||
            strcmp(name,"DIVCOUNT")==0 || strcmp(name,"TAUD")==0){
          /* NDIVS(n) — number of positive divisors τ(n); n<=0 → 0 */
          if (a <= 0) return 0;
          long n = a, cnt = 0;
          for (long i = 1; i * i <= n; i++){
            if ((n % i) == 0){
              cnt++;
              if (i * i != n) cnt++;
            }
          }
          return cnt;
        }
        if (strcmp(name,"SIGMA")==0 || strcmp(name,"DIVSUM")==0 ||
            strcmp(name,"SIGMA1")==0){
          /* SIGMA(n) — sum of positive divisors σ(n); n<=0 → 0 */
          if (a <= 0) return 0;
          long n = a;
          long sum = 0;
          for (long i = 1; i * i <= n; i++){
            if ((n % i) == 0){
              sum += i;
              if (i * i != n) sum += n / i;
            }
          }
          return sum;
        }
        if (strcmp(name,"ALIQUOT")==0 || strcmp(name,"PROPERSIGMA")==0 ||
            strcmp(name,"S0")==0 || strcmp(name,"SIGMA0STAR")==0 ||
            strcmp(name,"ISPERFECT")==0 || strcmp(name,"PERFECTP")==0 ||
            strcmp(name,"ISABUNDANT")==0 || strcmp(name,"ABUNDANTP")==0 ||
            strcmp(name,"ISDEFICIENT")==0 || strcmp(name,"DEFICIENTP")==0){
          /* ALIQUOT(n)=σ(n)-n; ISPERFECT/ISABUNDANT/ISDEFICIENT classification
           * n<=0 → 0; n=1 aliquot=0 (no proper divisors >0 except convention s(1)=0) */
          if (a <= 0) return 0;
          long n = a;
          long sum = 0;
          for (long i = 1; i * i <= n; i++){
            if ((n % i) == 0){
              sum += i;
              if (i * i != n) sum += n / i;
            }
          }
          long s = sum - n; /* proper divisor sum (exclude n itself) */
          if (strcmp(name,"ALIQUOT")==0 || strcmp(name,"PROPERSIGMA")==0 ||
              strcmp(name,"S0")==0 || strcmp(name,"SIGMA0STAR")==0)
            return s;
          if (strcmp(name,"ISPERFECT")==0 || strcmp(name,"PERFECTP")==0)
            return (s == n) ? 1 : 0;
          if (strcmp(name,"ISABUNDANT")==0 || strcmp(name,"ABUNDANTP")==0)
            return (s > n) ? 1 : 0;
          /* ISDEFICIENT / DEFICIENTP */
          return (s < n) ? 1 : 0;
        }
        if (strcmp(name,"PHI")==0 || strcmp(name,"TOTIENT")==0 ||
            strcmp(name,"EULERPHI")==0){
          /* PHI(n) — Euler's totient φ(n); n<=0 → 0 */
          if (a <= 0) return 0;
          if (a == 1) return 1;
          long n = a;
          long r = n;
          if ((n % 2) == 0){
            while ((n % 2) == 0) n /= 2;
            r -= r / 2;
          }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){
              while ((n % p) == 0) n /= p;
              r -= r / p;
            }
          }
          if (n > 1) r -= r / n;
          return r;
        }
        if (strcmp(name,"MOBIUS")==0 || strcmp(name,"MU")==0){
          /* MOBIUS(n) — μ(n): 0 if square factor; else (-1)^k for k distinct primes; n<=0→0 */
          if (a <= 0) return 0;
          if (a == 1) return 1;
          long n = a;
          int k = 0;
          if ((n % 2) == 0){
            n /= 2; k++;
            if ((n % 2) == 0) return 0;
          }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){
              n /= p; k++;
              if ((n % p) == 0) return 0;
            }
          }
          if (n > 1) k++;
          return (k & 1) ? -1 : 1;
        }
        if (strcmp(name,"RADICAL")==0 || strcmp(name,"RAD")==0){
          /* RADICAL(n) — product of distinct prime factors; n<=0→0; n=1→1 */
          if (a <= 0) return 0;
          if (a == 1) return 1;
          long n = a;
          long r = 1;
          if ((n % 2) == 0){
            r *= 2;
            while ((n % 2) == 0) n /= 2;
          }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){
              r *= p;
              while ((n % p) == 0) n /= p;
            }
          }
          if (n > 1) r *= n;
          return r;
        }
        if (strcmp(name,"ISSQUAREFREE")==0 || strcmp(name,"SQFREE")==0 ||
            strcmp(name,"SQUAREFREE")==0){
          /* ISSQUAREFREE(n) — 1 if no squared prime divides n; n<=0→0; n=1→1 */
          if (a <= 0) return 0;
          if (a == 1) return 1;
          long n = a;
          if ((n % 2) == 0){
            n /= 2;
            if ((n % 2) == 0) return 0;
          }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){
              n /= p;
              if ((n % p) == 0) return 0;
            }
          }
          return 1;
        }
        if (strcmp(name,"COPRIME")==0 || strcmp(name,"ISCOPRIME")==0){
          /* COPRIME(a,b) — 1 if gcd(|a|,|b|)==1 */
          long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
          if (x == 0 && y == 0) return 0;
          while (y){ long t = x % y; x = y; y = t; }
          return x == 1 ? 1 : 0;
        }
        if (strcmp(name,"CEILPOW2")==0 || strcmp(name,"NEXTPOW2")==0 ||
            strcmp(name,"CPOW2")==0){
          /* CEILPOW2(n) — smallest power of 2 ≥ n; n<=0→0; overflow→0 */
          if (a <= 0) return 0;
          if (a == 1) return 1;
          unsigned long u = (unsigned long)a;
          if ((u & (u - 1ul)) == 0ul) return a;
          /* next power of two; max 2^62 in signed 64 */
          if (a > (1L << 62)) return 0;
          long r = 1;
          while (r < a){
            if (r > (1L << 61)) return 0;
            r <<= 1;
          }
          return r;
        }
        /* digit-2/6 extended math: log2 / log10 / odd-even / bit counts / pow2 */
        if (strcmp(name,"ILOG2")==0 || strcmp(name,"LOG2")==0){
          if (a <= 0) return -1;
          unsigned long u = (unsigned long)a;
          long r = -1;
          while (u){ r++; u >>= 1; }
          return r;
        }
        if (strcmp(name,"ILOG10")==0 || strcmp(name,"LOG10")==0){
          /* floor(log10(a)); a<=0 → -1 */
          if (a <= 0) return -1;
          long r = 0;
          long x = a;
          while (x >= 10){ r++; x /= 10; }
          return r;
        }
        if (strcmp(name,"ODD")==0) return (a & 1L) ? 1 : 0;
        if (strcmp(name,"EVEN")==0) return (a & 1L) ? 0 : 1;
        if (strcmp(name,"CTZ")==0 || strcmp(name,"NTZ")==0){
          /* count trailing zeros (0 → 64) */
          if (a == 0) return 64;
          unsigned long u = (unsigned long)a;
          long n = 0;
          while ((u & 1ul) == 0){ n++; u >>= 1; }
          return n;
        }
        if (strcmp(name,"CLZ")==0 || strcmp(name,"NLZ")==0){
          /* count leading zeros in 64-bit word (0 → 64) */
          if (a == 0) return 64;
          unsigned long u = (unsigned long)a;
          long n = 0;
          for (int i = 63; i >= 0; i--){
            if (u & (1ul << i)) break;
            n++;
          }
          return n;
        }
        if (strcmp(name,"FFS")==0 || strcmp(name,"FINDLS")==0){
          /* find first set: 1-based index of lowest 1-bit; 0 if a==0 */
          if (a == 0) return 0;
          unsigned long u = (unsigned long)a;
          long n = 1;
          while ((u & 1ul) == 0){ n++; u >>= 1; }
          return n;
        }
        if (strcmp(name,"FLS")==0 || strcmp(name,"MSB")==0 || strcmp(name,"FINDMSB")==0){
          /* find last set: 1-based index of highest 1-bit; 0 if a==0 */
          if (a == 0) return 0;
          unsigned long u = (unsigned long)a;
          for (int i = 63; i >= 0; i--){
            if (u & (1ul << (unsigned)i)) return (long)(i + 1);
          }
          return 0;
        }
        if (strcmp(name,"CLO")==0){
          /* count leading ones in 64-bit word (all ones → 64) */
          unsigned long u = (unsigned long)a;
          long n = 0;
          for (int i = 63; i >= 0; i--){
            if ((u & (1ul << (unsigned)i)) == 0) break;
            n++;
          }
          return n;
        }
        if (strcmp(name,"CTO")==0){
          /* count trailing ones (all ones → 64) */
          unsigned long u = (unsigned long)a;
          long n = 0;
          while (u & 1ul){ n++; u >>= 1; if (n >= 64) break; }
          return n;
        }
        if (strcmp(name,"BWIDTH")==0 || strcmp(name,"BITWIDTH")==0){
          /* minimal bits to represent unsigned a (0 → 0) */
          if (a == 0) return 0;
          unsigned long u = (unsigned long)a;
          long n = 0;
          for (int i = 63; i >= 0; i--){
            if (u & (1ul << (unsigned)i)){ n = i + 1; break; }
          }
          return n;
        }
        if (strcmp(name,"ISPOW2")==0){
          if (a <= 0) return 0;
          unsigned long u = (unsigned long)a;
          return (u & (u - 1ul)) == 0ul ? 1 : 0;
        }
        if (strcmp(name,"POW2")==0){
          if (a < 0 || a > 62) return 0;
          return 1L << a;
        }
        if (strcmp(name,"POW10")==0 || strcmp(name,"TENPOW")==0){
          /* 10^a for a in 0..18; out of range → 0 */
          if (a < 0 || a > 18) return 0;
          long r = 1;
          for (long i = 0; i < a; i++) r *= 10;
          return r;
        }
        if (strcmp(name,"NDIGITS")==0){
          long x = a < 0 ? -a : a;
          if (x == 0) return 1;
          long n = 0;
          while (x){ n++; x /= 10; }
          return n;
        }
        if (strcmp(name,"DIGSUM")==0){
          long x = a < 0 ? -a : a;
          long s = 0;
          if (x == 0) return 0;
          while (x){ s += x % 10; x /= 10; }
          return s;
        }
        if (strcmp(name,"MODINV")==0 || strcmp(name,"INVMOD")==0){
          /* modular inverse a^{-1} mod b via extended Euclid; 0 if none */
          long m = b;
          if (m <= 1) return 0;
          long aa = a % m; if (aa < 0) aa += m;
          if (aa == 0) return 0;
          long t = 0, nt = 1;
          long r = m, nr = aa;
          while (nr != 0){
            long q = r / nr;
            long tmp = nt; nt = t - q * nt; t = tmp;
            tmp = nr; nr = r - q * nr; r = tmp;
          }
          if (r > 1) return 0; /* not invertible */
          if (t < 0) t += m;
          return t;
        }
        /* digit-2 modular duals: JACOBI / LEGENDRE / MODDIV / SPF */
        if (strcmp(name,"JACOBI")==0 || strcmp(name,"LEGENDRE")==0){
          /* JACOBI(a,n) / LEGENDRE(a,p) → -1/0/1; n must be odd positive */
          long n = b;
          if (n <= 0 || (n & 1L) == 0) return 0;
          long aa = a % n; if (aa < 0) aa += n;
          int res = 1;
          while (aa != 0){
            while ((aa & 1L) == 0){
              aa >>= 1;
              long n8 = n & 7L;
              if (n8 == 3 || n8 == 5) res = -res;
            }
            long tmp = aa; aa = n; n = tmp;
            if ((aa & 3L) == 3 && (n & 3L) == 3) res = -res;
            aa %= n;
          }
          return (n == 1) ? (long)res : 0;
        }
        if (strcmp(name,"MODDIV")==0 || strcmp(name,"DIVMODM")==0){
          /* MODDIV(a,b,m) → a * b^{-1} mod m; 0 if no inverse or m<=0 */
          long m = c;
          if (m <= 0) return 0;
          long bb = b % m; if (bb < 0) bb += m;
          if (bb == 0) return 0;
          long t = 0, nt = 1;
          long rr = m, nr = bb;
          while (nr != 0){
            long q = rr / nr;
            long tmp = nt; nt = t - q * nt; t = tmp;
            tmp = nr; nr = rr - q * nr; rr = tmp;
          }
          if (rr != 1) return 0;
          if (t < 0) t += m;
          long x = a % m; if (x < 0) x += m;
          long y = t, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          return acc;
        }
        if (strcmp(name,"SPF")==0 || strcmp(name,"SMALLPF")==0 ||
            strcmp(name,"MINPF")==0){
          /* SPF(n) — smallest prime factor; n<=1 → 0 */
          long n = a < 0 ? -a : a;
          if (n <= 1) return 0;
          if ((n & 1L) == 0) return 2;
          if ((n % 3L) == 0) return 3;
          for (long i = 5; i * i <= n; i += 6){
            if ((n % i) == 0) return i;
            if ((n % (i + 2)) == 0) return i + 2;
          }
          return n;
        }
        /* digit-2 factor metrics: VALUATION / OMEGA / OMEGA0 */
        if (strcmp(name,"VALUATION")==0 || strcmp(name,"PVAL")==0 ||
            strcmp(name,"VP")==0){
          /* VALUATION(n,p) — largest k with p^k | n; p<=1 or n==0 → 0 */
          long n = a < 0 ? -a : a;
          long p = b < 0 ? -b : b;
          if (n == 0 || p <= 1) return 0;
          long k = 0;
          while (n % p == 0){ n /= p; k++; if (n == 0) break; }
          return k;
        }
        if (strcmp(name,"OMEGA")==0 || strcmp(name,"BIGOMEGA")==0){
          /* OMEGA(n) — total prime factors with multiplicity Ω(n); n<=1 → 0 */
          long n = a < 0 ? -a : a;
          if (n <= 1) return 0;
          long k = 0;
          while ((n & 1L) == 0){ n >>= 1; k++; }
          for (long p = 3; p * p <= n; p += 2){
            while ((n % p) == 0){ n /= p; k++; }
          }
          if (n > 1) k++;
          return k;
        }
        if (strcmp(name,"OMEGA0")==0 || strcmp(name,"LITTLEOMEGA")==0 ||
            strcmp(name,"NUOMEGA")==0){
          /* OMEGA0(n) — distinct prime factors ω(n); n<=1 → 0 */
          long n = a < 0 ? -a : a;
          if (n <= 1) return 0;
          long k = 0;
          if ((n & 1L) == 0){
            k++;
            while ((n & 1L) == 0) n >>= 1;
          }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){
              k++;
              while ((n % p) == 0) n /= p;
            }
          }
          if (n > 1) k++;
          return k;
        }
        if (strcmp(name,"SOPF")==0){
          /* SOPF(n) — sum of distinct prime factors; n<=1 → 0 */
          long n = a < 0 ? -a : a;
          if (n <= 1) return 0;
          long s = 0;
          if ((n & 1L) == 0){
            s += 2;
            while ((n & 1L) == 0) n >>= 1;
          }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){
              s += p;
              while ((n % p) == 0) n /= p;
            }
          }
          if (n > 1) s += n;
          return s;
        }
        if (strcmp(name,"SOPFR")==0){
          /* SOPFR(n) — sum of prime factors with multiplicity; n<=1 → 0 */
          long n = a < 0 ? -a : a;
          if (n <= 1) return 0;
          long s = 0;
          while ((n & 1L) == 0){ s += 2; n >>= 1; }
          for (long p = 3; p * p <= n; p += 2){
            while ((n % p) == 0){ s += p; n /= p; }
          }
          if (n > 1) s += n;
          return s;
        }
        if (strcmp(name,"CARMICHAEL")==0 || strcmp(name,"LAMBDA")==0 ||
            strcmp(name,"CARM")==0){
          /* CARMICHAEL/LAMBDA(n) — Carmichael λ(n); n<=0 → 0; n==1 → 1 */
          long n = a < 0 ? -a : a;
          if (n <= 0) return 0;
          if (n == 1) return 1;
          long res = 1;
          long e2 = 0;
          while ((n & 1L) == 0){ n >>= 1; e2++; }
          if (e2){
            if (e2 == 1) res = 1;
            else if (e2 == 2) res = 2;
            else res = 1L << (e2 - 2);
          }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){
              long pk = 1;
              while ((n % p) == 0){ n /= p; pk *= p; }
              long lam = (pk / p) * (p - 1);
              long x = res, y = lam;
              while (y){ long t = x % y; x = y; y = t; }
              long g = x;
              res = (g == 0) ? 0 : (res / g) * lam;
            }
          }
          if (n > 1){
            long lam = n - 1;
            long x = res, y = lam;
            while (y){ long t = x % y; x = y; y = t; }
            long g = x;
            res = (g == 0) ? 0 : (res / g) * lam;
          }
          return res;
        }
        if (strcmp(name,"ORDER")==0 || strcmp(name,"MULTORDER")==0){
          /* ORDER(a,m) — mult. order of a mod m; 0 if gcd!=1 or m<=1 */
          long m = b;
          if (m <= 1) return 0;
          long aa = a % m; if (aa < 0) aa += m;
          if (aa == 0) return 0;
          long x = aa, y = m;
          while (y){ long t = x % y; x = y; y = t; }
          if (x != 1) return 0;
          long mm = m, res = 1, e2 = 0;
          while ((mm & 1L) == 0){ mm >>= 1; e2++; }
          if (e2){
            if (e2 == 1) res = 1;
            else if (e2 == 2) res = 2;
            else res = 1L << (e2 - 2);
          }
          for (long p = 3; p * p <= mm; p += 2){
            if ((mm % p) == 0){
              long pk = 1;
              while ((mm % p) == 0){ mm /= p; pk *= p; }
              long lam = (pk / p) * (p - 1);
              long gx = res, gy = lam;
              while (gy){ long t = gx % gy; gx = gy; gy = t; }
              res = (gx == 0) ? 0 : (res / gx) * lam;
            }
          }
          if (mm > 1){
            long lam = mm - 1;
            long gx = res, gy = lam;
            while (gy){ long t = gx % gy; gx = gy; gy = t; }
            res = (gx == 0) ? 0 : (res / gx) * lam;
          }
          long lam = res;
          for (long k = 1; k <= lam; k++){
            long base = aa, exp = k, r = 1 % m;
            while (exp > 0){
              if (exp & 1){
                long y2 = r, x2 = base, acc = 0;
                while (y2 > 0){
                  if (y2 & 1) acc = (acc + x2) % m;
                  x2 = (x2 + x2) % m;
                  y2 >>= 1;
                }
                r = acc;
              }
              {
                long x2 = base, acc = 0, y2 = base;
                while (y2 > 0){
                  if (y2 & 1) acc = (acc + x2) % m;
                  x2 = (x2 + x2) % m;
                  y2 >>= 1;
                }
                base = acc;
              }
              exp >>= 1;
            }
            if (r == 1) return k;
          }
          return 0;
        }
        if (strcmp(name,"SQPART")==0 || strcmp(name,"LARGESQ")==0 ||
            strcmp(name,"MAXSQ")==0){
          /* SQPART(n) — largest square dividing n; n<=0 → 0; n==1 → 1 */
          long n = a < 0 ? -a : a;
          if (n == 0) return 0;
          if (n == 1) return 1;
          long r = 1;
          long e = 0;
          while ((n & 1L) == 0){ n >>= 1; e++; }
          e = (e / 2) * 2;
          for (long i = 0; i < e; i++) r *= 2;
          for (long p = 3; p * p <= n; p += 2){
            e = 0;
            while ((n % p) == 0){ n /= p; e++; }
            e = (e / 2) * 2;
            for (long i = 0; i < e; i++) r *= p;
          }
          /* remaining prime power p^1 has no square factor */
          return r;
        }
        if (strcmp(name,"ISPRIMITIVE")==0 || strcmp(name,"ISPRROOT")==0 ||
            strcmp(name,"PRIMROOTP")==0){
          /* ISPRIMITIVE(a,m) — 1 if a is a primitive root mod m (order==φ(m)) */
          long m = b;
          if (m <= 1) return 0;
          long aa = a % m; if (aa < 0) aa += m;
          if (aa == 0) return 0;
          long gx = aa, gy = m;
          while (gy){ long t = gx % gy; gx = gy; gy = t; }
          if (gx != 1) return 0;
          /* φ(m) */
          long mm = m, phi = m;
          if ((mm & 1L) == 0){
            phi -= phi / 2;
            while ((mm & 1L) == 0) mm >>= 1;
          }
          for (long p = 3; p * p <= mm; p += 2){
            if ((mm % p) == 0){
              phi -= phi / p;
              while ((mm % p) == 0) mm /= p;
            }
          }
          if (mm > 1) phi -= phi / mm;
          /* order(a,m) via scan 1..phi; prim root iff order==phi */
          for (long k = 1; k <= phi; k++){
            long base = aa, exp = k, r = 1 % m;
            while (exp > 0){
              if (exp & 1){
                long y2 = r, x2 = base, acc = 0;
                while (y2 > 0){
                  if (y2 & 1) acc = (acc + x2) % m;
                  x2 = (x2 + x2) % m;
                  y2 >>= 1;
                }
                r = acc;
              }
              {
                long x2 = base, acc = 0, y2 = base;
                while (y2 > 0){
                  if (y2 & 1) acc = (acc + x2) % m;
                  x2 = (x2 + x2) % m;
                  y2 >>= 1;
                }
                base = acc;
              }
              exp >>= 1;
            }
            if (r == 1) return (k == phi) ? 1 : 0;
          }
          return 0;
        }
        if (strcmp(name,"ISPOWER")==0 || strcmp(name,"PERFPOW")==0 ||
            strcmp(name,"ISPOW")==0){
          /* ISPOWER(n) — 1 if n = b^e for some e>=2, b>0; n<=1 → 0 */
          long n = a;
          if (n <= 1) return 0;
          /* try exponents 2..62 */
          for (long e = 2; e <= 62; e++){
            /* binary search root: largest r with r^e <= n */
            long lo = 1, hi = n, ans = 1;
            if (e == 2){
              hi = n;
              if (hi > 3037000499L) hi = 3037000499L;
            } else {
              /* rough upper bound */
              hi = n;
              if (e >= 3 && hi > 1000000L) hi = 1000000L;
              if (e >= 10 && hi > 10000L) hi = 10000L;
              if (e >= 20 && hi > 100L) hi = 100L;
              if (e >= 40 && hi > 4L) hi = 4L;
            }
            while (lo <= hi){
              long mid = lo + (hi - lo) / 2;
              /* compute mid^e with overflow guard */
              long p = 1;
              int ov = 0;
              for (long i = 0; i < e; i++){
                if (mid != 0 && p > n / mid){ ov = 1; break; }
                p *= mid;
              }
              if (ov || p > n) hi = mid - 1;
              else { ans = mid; lo = mid + 1; }
            }
            long p = 1, ov = 0;
            for (long i = 0; i < e; i++){
              if (ans != 0 && p > n / ans){ ov = 1; break; }
              p *= ans;
            }
            if (!ov && p == n && ans > 1) return 1;
          }
          return 0;
        }
        if (strcmp(name,"ISPRIMEPOWER")==0 || strcmp(name,"IPP")==0 ||
            strcmp(name,"PRIMEPOWERP")==0){
          /* ISPRIMEPOWER(n) — 1 if n = p^k for prime p, k>=1 */
          long n = a < 0 ? -a : a;
          if (n <= 1) return 0;
          if ((n & 1L) == 0){
            while ((n & 1L) == 0) n >>= 1;
            return n == 1 ? 1 : 0;
          }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){
              while ((n % p) == 0) n /= p;
              return n == 1 ? 1 : 0;
            }
          }
          return 1; /* n itself prime */
        }
        if (strcmp(name,"NTHPRIME")==0 || strcmp(name,"PRIMEN")==0 ||
            strcmp(name,"PRIMEK")==0){
          /* NTHPRIME(k) — k-th prime (1-indexed: 1→2); k<=0 → 0; cap k=10000 */
          long k = a;
          if (k <= 0) return 0;
          if (k > 10000) k = 10000;
          if (k == 1) return 2;
          long found = 1; /* already counted 2 */
          for (long n = 3; ; n += 2){
            int prime = 1;
            if ((n % 3) == 0){ if (n != 3) prime = 0; }
            else {
              for (long i = 5; i * i <= n; i += 6){
                if ((n % i) == 0 || (n % (i + 2)) == 0){ prime = 0; break; }
              }
            }
            if (prime){
              found++;
              if (found == k) return n;
            }
          }
        }
        if (strcmp(name,"PRIMECOUNT")==0 || strcmp(name,"PRIMEPI")==0 ||
            strcmp(name,"PIN")==0 || strcmp(name,"COUNTPRIMES")==0){
          /* PRIMECOUNT(n) / π(n) — number of primes ≤ n; n<2 → 0; cap n=200000 */
          long n = a;
          if (n < 2) return 0;
          if (n > 200000L) n = 200000L;
          long cnt = 0;
          for (long x = 2; x <= n; x++){
            int okp = 1;
            if (x <= 3){ cnt++; continue; }
            if ((x % 2) == 0 || (x % 3) == 0){ okp = 0; }
            else {
              for (long i = 5; i * i <= x; i += 6){
                if ((x % i) == 0 || (x % (i + 2)) == 0){ okp = 0; break; }
              }
            }
            if (okp) cnt++;
          }
          return cnt;
        }
        if (strcmp(name,"PRIMEGAP")==0 || strcmp(name,"PGAP")==0 ||
            strcmp(name,"NEXTGAP")==0){
          /* PRIMEGAP(n) — nextprime(n) - n (strictly next prime distance); n large → 0 */
          long x = a + 1;
          if (x <= 2) return 2 - a; /* if a < 2, gap to 2 */
          if ((x & 1L) == 0) x++;
          for (long guard = 0; guard < 200000; guard++, x += 2){
            long t = x;
            int okp = 1;
            if (t <= 3) return t - a;
            if ((t % 3) == 0){ okp = 0; }
            else {
              for (long i = 5; i * i <= t; i += 6){
                if ((t % i) == 0 || (t % (i + 2)) == 0){ okp = 0; break; }
              }
            }
            if (okp) return t - a;
          }
          return 0;
        }
        if (strcmp(name,"ISCOMPOSITE")==0 || strcmp(name,"COMPOSITEP")==0 ||
            strcmp(name,"COMPP")==0){
          /* ISCOMPOSITE(n) — 1 if n has a non-trivial factor (n>1 and not prime) */
          long n = a < 0 ? -a : a;
          if (n <= 3) return 0;
          if ((n % 2) == 0 || (n % 3) == 0) return 1;
          for (long i = 5; i * i <= n; i += 6){
            if ((n % i) == 0 || (n % (i + 2)) == 0) return 1;
          }
          return 0;
        }
        /* digit-0 foundation bitfields */
        if (strcmp(name,"BEXT")==0 || strcmp(name,"BITEXT")==0){
          /* BEXT(val, pos, width) — extract width bits starting at bit pos */
          long pos = b, width = c;
          if (pos < 0) pos = 0;
          if (pos > 62) return 0;
          if (width < 1) return 0;
          if (width > 63 - pos) width = 63 - pos;
          if (width <= 0) return 0;
          unsigned long mask = (width >= 63) ? ~0ul : ((1ul << width) - 1ul);
          return (long)(((unsigned long)a >> (unsigned)pos) & mask);
        }
        if (strcmp(name,"BDEP")==0 || strcmp(name,"BITDEP")==0){
          /* BDEP(base, field, pos) — deposit low 8 bits of field at bit pos */
          long pos = c;
          long width = 8;
          if (pos < 0) pos = 0;
          if (pos > 62) return a;
          if (width > 63 - pos) width = 63 - pos;
          if (width <= 0) return a;
          unsigned long mask = (width >= 63) ? ~0ul : ((1ul << width) - 1ul);
          unsigned long base = (unsigned long)a;
          unsigned long field = (unsigned long)b & mask;
          base = (base & ~(mask << (unsigned)pos)) | (field << (unsigned)pos);
          return (long)base;
        }
        if (strcmp(name,"BYTE")==0){
          /* BYTE(val, i) — i-th byte little-endian (0=LSB) */
          long i = b;
          if (i < 0) i = 0;
          if (i > 7) i = 7;
          return (long)(((unsigned long)a >> (unsigned)(i * 8)) & 0xFFul);
        }
        if (strcmp(name,"LOBYTE")==0)
          return (long)((unsigned long)a & 0xFFul);
        if (strcmp(name,"HIBYTE")==0)
          return (long)(((unsigned long)a >> 8) & 0xFFul);
        if (strcmp(name,"MASK")==0 || strcmp(name,"BITMASK")==0){
          /* MASK(n) — low n bits set; n<=0 → 0; n>=63 → all ones (signed long) */
          if (a <= 0) return 0;
          if (a >= 63) return (long)~0ul;
          return (long)((1ul << (unsigned)a) - 1ul);
        }
        if (strcmp(name,"ISDIV")==0 || strcmp(name,"DIVISIBLE")==0){
          /* ISDIV(a,b) → 1 if b!=0 and a is multiple of b */
          if (b == 0) return 0;
          return (a % b) == 0 ? 1 : 0;
        }
        /* digit-1 word data path: BSWAP BITREV PARITY NIBBLE */
        if (strcmp(name,"BSWAP")==0 || strcmp(name,"BSWAP32")==0){
          unsigned int w = (unsigned int)a;
          w = ((w & 0x000000FFu) << 24) | ((w & 0x0000FF00u) << 8) |
              ((w & 0x00FF0000u) >> 8) | ((w & 0xFF000000u) >> 24);
          return (long)w;
        }
        if (strcmp(name,"BSWAP16")==0){
          unsigned int w = (unsigned int)a & 0xFFFFu;
          w = ((w & 0x00FFu) << 8) | ((w & 0xFF00u) >> 8);
          return (long)w;
        }
        if (strcmp(name,"BSWAP64")==0){
          unsigned long w = (unsigned long)a;
          w = ((w & 0x00000000000000FFul) << 56) | ((w & 0x000000000000FF00ul) << 40) |
              ((w & 0x0000000000FF0000ul) << 24) | ((w & 0x00000000FF000000ul) << 8) |
              ((w & 0x000000FF00000000ul) >> 8) | ((w & 0x0000FF0000000000ul) >> 24) |
              ((w & 0x00FF000000000000ul) >> 40) | ((w & 0xFF00000000000000ul) >> 56);
          return (long)w;
        }
        if (strcmp(name,"BITREV")==0 || strcmp(name,"REVBITS")==0){
          unsigned int w = (unsigned int)a;
          unsigned int r = 0;
          for (int i = 0; i < 32; i++){
            r = (r << 1) | (w & 1u);
            w >>= 1;
          }
          return (long)r;
        }
        if (strcmp(name,"PARITY")==0){
          unsigned long u = (unsigned long)a;
          int n = 0;
          while (u){ n ^= (int)(u & 1u); u >>= 1; }
          return (long)n;
        }
        if (strcmp(name,"NIBBLE")==0 || strcmp(name,"NIB")==0){
          /* NIBBLE(val, i) — i-th 4-bit nibble little-endian (0=LSB) */
          long i = b;
          if (i < 0) i = 0;
          if (i > 15) i = 15;
          return (long)(((unsigned long)a >> (unsigned)(i * 4)) & 0xFul);
        }
        if (strcmp(name,"DIST")==0 || strcmp(name,"ABSDIFF")==0){
          long d = a - b;
          return d < 0 ? -d : d;
        }
        /* digit-8: SEXT/ZEXT — extend bottom width bits; SEXT8/16 fixed width */
        if (strcmp(name,"SEXT8")==0 || strcmp(name,"SEXTB")==0){
          long v = a & 0xFFL;
          if (v & 0x80L) v |= ~0xFFL;
          return v;
        }
        if (strcmp(name,"SEXT16")==0 || strcmp(name,"SEXTW")==0){
          long v = a & 0xFFFFL;
          if (v & 0x8000L) v |= ~0xFFFFL;
          return v;
        }
        if (strcmp(name,"ZEXT")==0 || strcmp(name,"ZEROEXT")==0){
          long w = b;
          if (w <= 0) return 0;
          if (w >= 63) return a;
          unsigned long mask = (1ul << (unsigned)w) - 1ul;
          return (long)((unsigned long)a & mask);
        }
        if (strcmp(name,"SEXT")==0 || strcmp(name,"SIGNEXT")==0){
          long w = b;
          if (w <= 0) return 0;
          if (w >= 63) return a;
          unsigned long mask = (1ul << (unsigned)w) - 1ul;
          unsigned long v = (unsigned long)a & mask;
          unsigned long sign = 1ul << (unsigned)(w - 1);
          if (v & sign) v |= ~mask;
          return (long)v;
        }
        if (strcmp(name,"PACK8")==0 || strcmp(name,"PACKB")==0){
          /* PACK8(hi, lo) — two bytes → 16-bit word */
          unsigned int h = (unsigned int)a & 0xFFu;
          unsigned int l = (unsigned int)b & 0xFFu;
          return (long)((h << 8) | l);
        }
        if (strcmp(name,"PACKNIB")==0 || strcmp(name,"PACK4")==0){
          /* PACKNIB(hi, lo) — two nibbles → byte */
          unsigned int h = (unsigned int)a & 0xFu;
          unsigned int l = (unsigned int)b & 0xFu;
          return (long)((h << 4) | l);
        }
        if (strcmp(name,"SETNIB")==0 || strcmp(name,"SETNIBBLE")==0){
          /* SETNIB(val, field, i) — deposit 4-bit field at nibble index i (LE) */
          long i = c;
          if (i < 0) i = 0;
          if (i > 15) i = 15;
          unsigned long base = (unsigned long)a;
          unsigned long field = (unsigned long)b & 0xFul;
          unsigned long shift = (unsigned long)(i * 4);
          base = (base & ~(0xFul << shift)) | (field << shift);
          return (long)base;
        }
        if (strcmp(name,"SETBYTE")==0 || strcmp(name,"SETB")==0){
          /* SETBYTE(val, byte, i) — deposit 8-bit at byte index i (LE) */
          long i = c;
          if (i < 0) i = 0;
          if (i > 7) i = 7;
          unsigned long base = (unsigned long)a;
          unsigned long field = (unsigned long)b & 0xFFul;
          unsigned long shift = (unsigned long)(i * 8);
          base = (base & ~(0xFFul << shift)) | (field << shift);
          return (long)base;
        }
        if (strcmp(name,"ALIGN")==0 || strcmp(name,"ROUNDUP")==0){
          /* ALIGN(val, a) — smallest multiple of a that is >= val; a<=0 → val */
          long al = b;
          if (al <= 0) return a;
          long q = a / al, r = a % al;
          if (r == 0) return a;
          if (a > 0) return (q + 1) * al;
          return q * al; /* a<0: C trunc toward 0 → q*al is ceil toward +inf */
        }
        if (strcmp(name,"ALIGNDN")==0 || strcmp(name,"ROUNDDN")==0){
          /* ALIGNDN(val, a) — largest multiple of a that is <= val */
          long al = b;
          if (al <= 0) return a;
          long q = a / al, r = a % al;
          if (r == 0) return a;
          if (a > 0) return q * al;
          return (q - 1) * al; /* a<0: floor toward -inf */
        }
        if (strcmp(name,"DIVCEIL")==0 || strcmp(name,"CEILDIV")==0){
          /* ceil(a/b); 0 if b==0. Non-neg exact; mixed → C trunc (ok for ceil when <0). */
          if (b == 0) return 0;
          if (a >= 0 && b > 0) return (a + b - 1) / b;
          if (a <= 0 && b < 0){
            long aa = -a, bb = -b;
            return (aa + bb - 1) / bb;
          }
          return a / b;
        }
        if (strcmp(name,"SQR")==0 || strcmp(name,"SQUARE")==0)
          return a * a;
        if (strcmp(name,"DIVFLOOR")==0 || strcmp(name,"FLOORDIV")==0){
          /* floor(a/b); 0 if b==0 */
          if (b == 0) return 0;
          long q = a / b, r = a % b;
          if (r != 0 && ((a < 0) != (b < 0))) q--; /* toward -inf when signs differ */
          return q;
        }
        if (strcmp(name,"BINOM")==0 || strcmp(name,"CHOOSE")==0){
          /* C(n,k) multiplicative formula; 0 if invalid */
          long n = a, k = b;
          if (n < 0 || k < 0 || k > n) return 0;
          if (k > n - k) k = n - k;
          long r = 1;
          for (long i = 1; i <= k; i++){
            /* keep intermediate exact: multiply then divide by i */
            r = r * (n - k + i) / i;
          }
          return r;
        }
        if (strcmp(name,"PERM")==0 || strcmp(name,"PNR")==0){
          /* P(n,k) = n!/(n-k)! */
          long n = a, k = b;
          if (n < 0 || k < 0 || k > n) return 0;
          long r = 1;
          for (long i = 0; i < k; i++) r *= (n - i);
          return r;
        }
        return 0;
      }
      /* zero-arg: PEEK() STACKLEN() SP() */
      if (strcmp(name,"PEEK")==0){
        if (vm->sp <= 0) return 0;
        return vm->stack[vm->sp - 1];
      }
      if (strcmp(name,"STACKLEN")==0 || strcmp(name,"SP")==0)
        return (long)vm->sp;
    }
    if (strcmp(name,"SET")==0 || strcmp(name,"POPCOUNT")==0 ||
        strcmp(name,"ENERGY")==0 || strcmp(name,"DIGIT")==0 ||
        strcmp(name,"BIT")==0 || strcmp(name,"FLOWED")==0 ||
        strcmp(name,"COMPILED")==0 || strcmp(name,"PARENT")==0 ||
        strcmp(name,"NESTED")==0 || strcmp(name,"PORTS")==0 ||
        strcmp(name,"NPORTS")==0 || strcmp(name,"PLUGGED")==0 ||
        strcmp(name,"BITS")==0 || strcmp(name,"WIDTH")==0){
      if (L->cur.kind==TK_LPAREN){
        lex_next(L);
        char id[48]={0};
        if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
        int bit=-1;
        if (strcmp(name,"BIT")==0 && L->cur.kind==TK_COMMA){
          lex_next(L);
          if (L->cur.kind==TK_NUM){ bit=(int)L->cur.num; lex_next(L); }
        }
        if (L->cur.kind==TK_RPAREN) lex_next(L);
        int ix=find_cube(vm,id);
        if (ix<0) return 0;
        cubalc_cube *c=&vm->ch.cubes[ix];
        if (strcmp(name,"SET")==0 || strcmp(name,"POPCOUNT")==0)
          return cubalc_matrix_popcount(&c->atom.matrix);
        if (strcmp(name,"ENERGY")==0) return (long)lround(c->atom.energy*100.0);
        if (strcmp(name,"DIGIT")==0) return (long)c->atom.digit;
        if (strcmp(name,"BIT")==0 && bit>=0)
          return cubalc_matrix_get(&c->atom.matrix, bit)?1:0;
        if (strcmp(name,"FLOWED")==0) return c->flowed ? 1 : 0;
        if (strcmp(name,"COMPILED")==0) return c->compiled ? 1 : 0;
        if (strcmp(name,"PARENT")==0) return (long)c->parent;
        if (strcmp(name,"NESTED")==0) return c->parent >= 0 ? 1 : 0;
        if (strcmp(name,"PORTS")==0 || strcmp(name,"NPORTS")==0) return (long)c->n_ports;
        if (strcmp(name,"PLUGGED")==0) return (long)c->plugged;
        if (strcmp(name,"BITS")==0 || strcmp(name,"WIDTH")==0)
          return c->atom.matrix.n ? (long)c->atom.matrix.n : (long)CUBALC_ATOM_BITS;
        return 0;
      }
    }
    if (strcmp(name,"COMPAT")==0){
      if (L->cur.kind==TK_LPAREN){
        lex_next(L);
        char a[48]={0},b[48]={0};
        if (L->cur.kind==TK_IDENT){ snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L); }
        if (L->cur.kind==TK_COMMA) lex_next(L);
        if (L->cur.kind==TK_IDENT){ snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L); }
        if (L->cur.kind==TK_RPAREN) lex_next(L);
        int ia=find_cube(vm,a), ib=find_cube(vm,b);
        if (ia<0||ib<0) return 0;
        return (long)lround(cubalc_matrix_compat(
          &vm->ch.cubes[ia].atom.matrix,&vm->ch.cubes[ib].atom.matrix)*100.0);
      }
    }
    for (int i=0;i<vm->n_vars;i++) if (strcmp(vm->vars[i].name,name)==0) return vm->vars[i].val;
    int ix=find_cube(vm,name);
    if (ix>=0) return cubalc_matrix_popcount(&vm->ch.cubes[ix].atom.matrix);
    return 0;
  }
  return 0;
}
static long parse_term(VM *vm, Lex *L){
  long v=parse_prim(vm,L);
  for(;;){
    if (L->cur.kind==TK_STAR){ lex_next(L); v*=parse_prim(vm,L); }
    else if (L->cur.kind==TK_SLASH){ lex_next(L); long d=parse_prim(vm,L); v=d?v/d:0; }
    else if (L->cur.kind==TK_PERCENT){ lex_next(L); long d=parse_prim(vm,L); v=d?v%d:0; }
    else break;
  }
  return v;
}
static long parse_add(VM *vm, Lex *L){
  long v=parse_term(vm,L);
  for(;;){
    if (L->cur.kind==TK_PLUS){ lex_next(L); v+=parse_term(vm,L); }
    else if (L->cur.kind==TK_MINUS){ lex_next(L); v-=parse_term(vm,L); }
    else break;
  }
  return v;
}
/* String operand for == / != : literal, LAST (string), or is_str var.
 * Returns 1 and advances lexer on success; 0 and leaves L unchanged. */
static int try_str_operand(VM *vm, Lex *L, char *out, size_t outn){
  if (L->cur.kind == TK_STR) {
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 1;
  }
  if (L->cur.kind != TK_IDENT) return 0;
  if (strcmp(L->cur.text, "LAST") == 0) {
    Var *lv = var_get(vm, "LAST", 0);
    if (lv && lv->is_str) {
      snprintf(out, outn, "%s", lv->sval);
      lex_next(L);
      return 1;
    }
    /* host mirror of sticky LAST string (GETF / SYS often set both) */
    if (vm->last_str[0] || (lv && lv->is_str)) {
      snprintf(out, outn, "%s", vm->last_str);
      lex_next(L);
      return 1;
    }
    return 0;
  }
  {
    Var *sv = var_get(vm, L->cur.text, 0);
    if (sv && sv->is_str) {
      snprintf(out, outn, "%s", sv->sval);
      lex_next(L);
      return 1;
    }
  }
  return 0;
}
static long parse_cmp(VM *vm, Lex *L){
  /* Content string equality: "a" == "b", LAST == x, s == t (is_str).
   * Numeric path kept for lengths / numbers (s.val == strlen when used alone).
   * Records last_cmp_* for ASSERT/EXPECT agent-readable got/expected. */
  vm->last_cmp_kind = 0;
  vm->last_cmp_op[0] = 0;
  vm->last_cmp_left[0] = 0;
  vm->last_cmp_right[0] = 0;
  {
    Lex save = *L;
    char left[512], right[512];
    if (try_str_operand(vm, L, left, sizeof left)) {
      if (L->cur.kind == TK_EQEQ || L->cur.kind == TK_NE) {
        int ne = (L->cur.kind == TK_NE);
        lex_next(L);
        if (try_str_operand(vm, L, right, sizeof right)) {
          int eq = (strcmp(left, right) == 0);
          long r = ne ? !eq : eq;
          vm->last_cmp_kind = 2;
          snprintf(vm->last_cmp_op, sizeof vm->last_cmp_op, "%s", ne ? "!=" : "==");
          snprintf(vm->last_cmp_left, sizeof vm->last_cmp_left, "%s", left);
          snprintf(vm->last_cmp_right, sizeof vm->last_cmp_right, "%s", right);
          return r;
        }
      }
      *L = save; /* not string-cmp form — fall through to numeric */
    }
  }
  long v=parse_add(vm,L);
  if (L->cur.kind==TK_EQEQ){
    lex_next(L);
    long r = parse_add(vm,L);
    vm->last_cmp_kind = 1;
    snprintf(vm->last_cmp_op, sizeof vm->last_cmp_op, "==");
    snprintf(vm->last_cmp_left, sizeof vm->last_cmp_left, "%ld", v);
    snprintf(vm->last_cmp_right, sizeof vm->last_cmp_right, "%ld", r);
    return v==r;
  }
  if (L->cur.kind==TK_NE){
    lex_next(L);
    long r = parse_add(vm,L);
    vm->last_cmp_kind = 1;
    snprintf(vm->last_cmp_op, sizeof vm->last_cmp_op, "!=");
    snprintf(vm->last_cmp_left, sizeof vm->last_cmp_left, "%ld", v);
    snprintf(vm->last_cmp_right, sizeof vm->last_cmp_right, "%ld", r);
    return v!=r;
  }
  if (L->cur.kind==TK_LT){
    lex_next(L);
    long r = parse_add(vm,L);
    vm->last_cmp_kind = 1;
    snprintf(vm->last_cmp_op, sizeof vm->last_cmp_op, "<");
    snprintf(vm->last_cmp_left, sizeof vm->last_cmp_left, "%ld", v);
    snprintf(vm->last_cmp_right, sizeof vm->last_cmp_right, "%ld", r);
    return v<r;
  }
  if (L->cur.kind==TK_LE){
    lex_next(L);
    long r = parse_add(vm,L);
    vm->last_cmp_kind = 1;
    snprintf(vm->last_cmp_op, sizeof vm->last_cmp_op, "<=");
    snprintf(vm->last_cmp_left, sizeof vm->last_cmp_left, "%ld", v);
    snprintf(vm->last_cmp_right, sizeof vm->last_cmp_right, "%ld", r);
    return v<=r;
  }
  if (L->cur.kind==TK_GT){
    lex_next(L);
    long r = parse_add(vm,L);
    vm->last_cmp_kind = 1;
    snprintf(vm->last_cmp_op, sizeof vm->last_cmp_op, ">");
    snprintf(vm->last_cmp_left, sizeof vm->last_cmp_left, "%ld", v);
    snprintf(vm->last_cmp_right, sizeof vm->last_cmp_right, "%ld", r);
    return v>r;
  }
  if (L->cur.kind==TK_GE){
    lex_next(L);
    long r = parse_add(vm,L);
    vm->last_cmp_kind = 1;
    snprintf(vm->last_cmp_op, sizeof vm->last_cmp_op, ">=");
    snprintf(vm->last_cmp_left, sizeof vm->last_cmp_left, "%ld", v);
    snprintf(vm->last_cmp_right, sizeof vm->last_cmp_right, "%ld", r);
    return v>=r;
  }
  /* bare truthiness — record value for ASSERT got N (falsey) */
  vm->last_cmp_kind = 3;
  snprintf(vm->last_cmp_left, sizeof vm->last_cmp_left, "%ld", v);
  vm->last_cmp_op[0] = 0;
  vm->last_cmp_right[0] = 0;
  return v;
}
long cubalc_lang_parse_expr(VM *vm, Lex *L){
  long v = parse_cmp(vm, L);
  for(;;){
    if (kw(&L->cur,"AND")){
      lex_next(L);
      long r = parse_cmp(vm, L);
      v = (v && r) ? 1 : 0;
    } else if (kw(&L->cur,"OR")){
      lex_next(L);
      long r = parse_cmp(vm, L);
      v = (v || r) ? 1 : 0;
    } else break;
  }
  /* digit-1 control expr: cond ? then : else  (C-style ternary; right-assoc) */
  if (L->cur.kind==TK_QMARK){
    lex_next(L);
    long t = parse_expr(vm, L);
    if (L->cur.kind!=TK_COLON){
      /* soft: missing colon → treat as look? keep value t if cond else 0 */
      fail(vm,"ternary needs ':' (cond ? then : else)");
      return 0;
    }
    lex_next(L);
    long e = parse_expr(vm, L);
    v = v ? t : e;
  }
  return v;
}

