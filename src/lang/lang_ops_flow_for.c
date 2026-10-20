/* CubalC lang — lang_ops_flow_for.c
 * FLOW FOR/EACH/FORCELL/FORBIT residual (A3 peel).
 * Mesh vote A3_carve_lang_ops_flow · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_flow_for.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include "cubalc_sot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#endif

int cubalc_lang_ops_flow_try_for(VM *vm, Lex *L) {
  /* FLOW loop residual. Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* FOR i = a TO b [STEP s] ... END
     FOR i = a DOWNTO b [STEP s] ... END  (digit-1: default step -1) */

  if (kw(&L->cur,"FOR")){
    lex_next(L);
    /* EXG fold hold: FOR <seconds> body END. Count loop stays FOR i = a TO b. */
    {
      Lex hold_save = *L;
      int is_count = 0;
      if (L->cur.kind==TK_IDENT){
        lex_next(L);
        if (L->cur.kind==TK_EQ) is_count = 1;
        *L = hold_save;
      }
      if (!is_count){
        long sec = parse_expr(vm,L);
        if (sec < 0) sec = 0;
        if (kw(&L->cur,"THEN") || kw(&L->cur,"SEC") || kw(&L->cur,"SECONDS") ||
            kw(&L->cur,"S"))
          lex_next(L);
        skip_nl(L);
        Lex body_start=*L;
        int depth=1;
        while (L->cur.kind!=TK_EOF){
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth!=0){ fail(vm,"FOR without END"); return -1; }
        struct timespec t0, t1, sl;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        long deadline_ms = (long)t0.tv_sec*1000L + t0.tv_nsec/1000000L + sec*1000L;
        long guard=0;
        for (;;){
          vm->break_loop=0; vm->continue_loop=0;
          Lex body=body_start;
          if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
          if (vm->break_loop){ vm->break_loop=0; break; }
          vm->continue_loop=0;
          clock_gettime(CLOCK_MONOTONIC, &t1);
          long now = (long)t1.tv_sec*1000L + t1.tv_nsec/1000000L;
          if (now >= deadline_ms || guard++>100000) break;
          sl.tv_sec = 0; sl.tv_nsec = 20L*1000L*1000L;
          nanosleep(&sl, NULL);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        var_set_num(vm,"LAST_N", sec);
        bump(vm); return 1;
      }
    }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FOR var = a TO b"); return -1; }
    char vname[48]; snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"FOR var ="); return -1; }
    lex_next(L);
    long lo=parse_expr(vm,L);
    if (!kw(&L->cur,"TO") && !kw(&L->cur,"DOWNTO") && !kw(&L->cur,"DOWN") &&
        !kw(&L->cur,"..") && !(L->cur.kind==TK_IDENT && strcmp(L->cur.text,"TO")==0)){
      /* allow FOR i = n as 0..n-1 */
      long hi=lo-1; lo=0;
      long step=1;
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"FOR without END"); return -1; }
      for (long i=lo;i<=hi && !vm->fatal && !vm->halt;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    int downto = 0;
    if (kw(&L->cur,"DOWNTO")||kw(&L->cur,"DOWN")){ downto = 1; lex_next(L); }
    else if (kw(&L->cur,"TO")||kw(&L->cur,"..")) lex_next(L);
    long hi=parse_expr(vm,L);
    long step = downto ? -1 : 1;
    if (kw(&L->cur,"STEP")||kw(&L->cur,"BY")){
      lex_next(L);
      step=parse_expr(vm,L);
      if (!step) step = downto ? -1 : 1;
      /* DOWNTO with positive step â force negative direction */
      if (downto && step > 0) step = -step;
    }
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FOR without END"); return -1; }
    if (step>0){
      for (long i=lo;i<=hi && !vm->fatal && !vm->halt;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
    } else {
      for (long i=lo;i>=hi && !vm->fatal && !vm->halt;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* EACH CUBE as name ... END  |  EACH CELL [as name] [FROM lo TO hi] ... END
   * EACH LINE [as name] [IN str|LAST|obj field] ... END â walk newline bags.
   * EACH PROP|ATTR|OBJFIELD OF obj|Class [AS name] ... END â walk schema field names.
   * EACH METHOD|MSG|HANDLER OF obj|Class [AS name] ... END â walk schema method names.
   * EACH OBJ [Class] [AS name] ... END â walk live OOP objects (optional class filter).
   * EACH KEY|KEYS [AS name] OF|FROM plate|LAST|PLATE [path] ... END â walk plate keys.
   * EACH KEYNEST nest [FROM plate] | EACH KEY OF NEST nest â walk nested object keys.
   * Nest path may be dotted/slash: EACH KEYNEST "cfg.flags" Â· EACH KEY OF PLATE "meta".
   * Soft empty walk if path miss / non-object. No KEYSP/KEYSOBJ+EACH LINE glue.
   * EACH FLAT|LEAF [AS name] [needle] [OF|FROM plate] ... END â walk matching leaf path:value.
   * Empty needle â all leaves. Binds PATH + VALUE. No PATHSFLAT+EACH LINE+GETP glue.
   * Note: EACH FIELD/FIELDS stays a LINE synonym (bag walk), not schema props.
   * digit-4 control: cell-range iterator binds value to name, IT=index, VAL=value */
  if (kw(&L->cur,"EACH")||kw(&L->cur,"FOREACH")){
    lex_next(L);
    int is_cell = (kw(&L->cur,"CELL")||kw(&L->cur,"CELLS")||kw(&L->cur,"SLOT")||kw(&L->cur,"SLOTS"));
    int is_cube = (kw(&L->cur,"CUBE")||kw(&L->cur,"CUBES"));
    int is_prop = (kw(&L->cur,"PROP")||kw(&L->cur,"PROPS")||kw(&L->cur,"ATTR")||
                   kw(&L->cur,"ATTRS")||kw(&L->cur,"MEMBER")||kw(&L->cur,"MEMBERS")||
                   kw(&L->cur,"OBJFIELD")||kw(&L->cur,"OBJFIELDS")||
                   kw(&L->cur,"SCHEMAFIELD")||kw(&L->cur,"SCHEMAFIELDS")||
                   kw(&L->cur,"CLASSFIELD")||kw(&L->cur,"CLASSFIELDS"));
    int is_meth = (kw(&L->cur,"METHOD")||kw(&L->cur,"METHODS")||kw(&L->cur,"METH")||
                   kw(&L->cur,"MSG")||kw(&L->cur,"MSGS")||kw(&L->cur,"MESSAGE")||
                   kw(&L->cur,"MESSAGES")||kw(&L->cur,"HANDLER")||kw(&L->cur,"HANDLERS")||
                   kw(&L->cur,"OBJMETHOD")||kw(&L->cur,"OBJMETHODS")||
                   kw(&L->cur,"CLASSMETHOD")||kw(&L->cur,"CLASSMETHODS"));
    int is_line = (kw(&L->cur,"LINE")||kw(&L->cur,"LINES")||kw(&L->cur,"FIELD")||
                   kw(&L->cur,"FIELDS")||kw(&L->cur,"ROW")||kw(&L->cur,"ROWS")||
                   kw(&L->cur,"ENTRY")||kw(&L->cur,"ENTRIES"));
    int is_obj = (kw(&L->cur,"OBJ")||kw(&L->cur,"OBJS")||kw(&L->cur,"OBJECT")||
                  kw(&L->cur,"OBJECTS")||kw(&L->cur,"INST")||kw(&L->cur,"INSTANCE")||
                  kw(&L->cur,"INSTANCES")||kw(&L->cur,"OOP"));
    int is_keynest = (kw(&L->cur,"KEYNEST")||kw(&L->cur,"NESTKEY")||
                      kw(&L->cur,"NESTKEYS")||kw(&L->cur,"KEYINNEST")||
                      kw(&L->cur,"NESTPAIR")||kw(&L->cur,"NESTPAIRS")||
                      kw(&L->cur,"PLATEKEYNEST")||kw(&L->cur,"JSONKEYNEST"));
    int is_key = is_keynest ||
                 (kw(&L->cur,"KEY")||kw(&L->cur,"KEYS")||kw(&L->cur,"PLATEKEY")||
                  kw(&L->cur,"PLATEKEYS")||kw(&L->cur,"JSONKEY")||kw(&L->cur,"JSONKEYS")||
                  kw(&L->cur,"K")||kw(&L->cur,"PAIR")||kw(&L->cur,"PAIRS")||
                  kw(&L->cur,"KV")||kw(&L->cur,"KVS"));
    int is_flat = (kw(&L->cur,"FLAT")||kw(&L->cur,"FLATS")||kw(&L->cur,"LEAF")||
                   kw(&L->cur,"LEAVES")||kw(&L->cur,"LEAFPATH")||kw(&L->cur,"LEAFPATHS")||
                   kw(&L->cur,"DOTPATH")||kw(&L->cur,"DOTPATHS")||kw(&L->cur,"FLATPATH")||
                   kw(&L->cur,"FLATPATHS")||kw(&L->cur,"PATHLEAF")||kw(&L->cur,"PATHLEAVES")||
                   kw(&L->cur,"LEAFKV")||kw(&L->cur,"FLATLEAF")||kw(&L->cur,"MATCHLEAF")||
                   kw(&L->cur,"MATCHLEAVES"));
    int is_lib = (kw(&L->cur,"LIB")||kw(&L->cur,"LIBS")||kw(&L->cur,"STEM")||
                  kw(&L->cur,"STEMS")||kw(&L->cur,"MODULE")||kw(&L->cur,"MODULES")||
                  kw(&L->cur,"LIBSTEM")||kw(&L->cur,"LIBSTEMS")||kw(&L->cur,"STDLIB")||
                  kw(&L->cur,"MATCHLIB")||kw(&L->cur,"MATCHLIBS"));
    int is_topic = (kw(&L->cur,"TOPIC")||kw(&L->cur,"TOPICS")||kw(&L->cur,"DISCOVERY")||
                    kw(&L->cur,"DISCOVERIES")||kw(&L->cur,"PLAYBOOK")||kw(&L->cur,"PLAYBOOKS")||
                    kw(&L->cur,"SURFACE")||kw(&L->cur,"SURFACES")||
                    kw(&L->cur,"DISCOVERY_TOPIC")||kw(&L->cur,"DISCOVERY_TOPICS"));
    if (!is_cell && !is_cube && !is_line && !is_obj && !is_prop && !is_meth && !is_key &&
        !is_flat && !is_lib && !is_topic){
      fail(vm,"EACH CUBE|CELL|LINE|PROP|METHOD|OBJ|KEY|KEYNEST|FLAT|LIB|TOPIC as name"); return -1;
    }
    lex_next(L);
    if (is_topic){
      /* EACH TOPIC [AS name] [MATCH|FILTER needle] â¦ END
       * Walk curated discovery topics (same set as LISTTOPICS / TIPS / RUNSNIP).
       * Optional MATCH filter (case-insensitive substring on id).
       * Binds name (default TOPIC) + LINE Â· IT 0-based Â· TOPIC_N 1-based Â· EACHTOPICS_N.
       * Usability: no LISTTOPICS + EACH LINE glue for TOPIC/TIPS/RUNSNIP walks. */
      static const char *topics[] = {
        "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
      };
      char tname[48], needle[96], fup[96];
      char hits[16][32];
      int nall = (int)(sizeof topics / sizeof topics[0]);
      int nh = 0, i;
      long idx = 0;
      size_t a;
      snprintf(tname, sizeof tname, "TOPIC");
      needle[0] = 0;
      fup[0] = 0;
      if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
        lex_next(L);
        if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH TOPIC as name"); return -1; }
        snprintf(tname, sizeof tname, "%s", L->cur.text); lex_next(L);
      } else if (L->cur.kind==TK_IDENT &&
                 strcmp(L->cur.text,"MATCH")!=0 && strcmp(L->cur.text,"FILTER")!=0 &&
                 strcmp(L->cur.text,"WHERE")!=0 && strcmp(L->cur.text,"WITH")!=0 &&
                 strcmp(L->cur.text,"OF")!=0 && strcmp(L->cur.text,"IN")!=0 &&
                 strcmp(L->cur.text,"FROM")!=0 && strcmp(L->cur.text,"END")!=0){
        snprintf(tname, sizeof tname, "%s", L->cur.text); lex_next(L);
      }
      if (kw(&L->cur,"MATCH")||kw(&L->cur,"FILTER")||kw(&L->cur,"WHERE")||
          kw(&L->cur,"WITH")||kw(&L->cur,"OF")||kw(&L->cur,"IN")||kw(&L->cur,"FROM")||
          kw(&L->cur,"LIKE")||kw(&L->cur,"FOR")){
        lex_next(L);
        if (L->cur.kind == TK_STR) {
          snprintf(needle, sizeof needle, "%s", L->cur.text);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(needle, sizeof needle, "%s", vv->sval);
          else if (strcmp(L->cur.text, "LAST") == 0)
            snprintf(needle, sizeof needle, "%s", vm->last_str);
          else
            snprintf(needle, sizeof needle, "%s", L->cur.text);
          lex_next(L);
        }
      }
      if (needle[0]) {
        for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
          char c = needle[a];
          if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
          fup[a] = c;
        }
        fup[a] = 0;
      }
      for (i = 0; i < nall && nh < 16; i++) {
        if (fup[0]) {
          char low[32];
          size_t b;
          for (b = 0; topics[i][b] && b + 1 < sizeof low; b++) {
            char c = topics[i][b];
            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            low[b] = c;
          }
          low[b] = 0;
          if (!strstr(low, fup)) continue;
        }
        snprintf(hits[nh++], sizeof hits[0], "%s", topics[i]);
      }
      skip_nl(L);
      {
        Lex body_start = *L;
        int depth = 1;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "EACH TOPIC without END"); return -1; }
        var_set_num(vm, "EACHTOPICS_N", nh);
        var_set_num(vm, "LISTTOPICS_N", nh);
        if (needle[0]) {
          var_set_str(vm, "EACHTOPICS_FILTER", needle);
          var_set_str(vm, "LISTTOPICS_FILTER", needle);
        } else {
          var_set_str(vm, "EACHTOPICS_FILTER", "");
        }
        for (i = 0; i < nh && !vm->fatal && !vm->halt; i++) {
          var_set_str(vm, tname, hits[i]);
          var_set_str(vm, "TOPIC", hits[i]);
          var_set_str(vm, "LINE", hits[i]);
          var_set_str(vm, "TOPIC_NAME", hits[i]);
          var_set_num(vm, "IT", idx);
          var_set_num(vm, "IDX", idx);
          var_set_num(vm, "TOPIC_N", idx + 1);
          var_set_num(vm, "LINE_N", idx + 1);
          var_set_num(vm, "OK", 1);
          vm->break_loop = 0;
          vm->continue_loop = 0;
          {
            Lex body = body_start;
            if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
          }
          if (vm->break_loop) { vm->break_loop = 0; break; }
          vm->continue_loop = 0;
          idx++;
        }
        if (kw(&L->cur, "END")) lex_next(L);
        var_set_num(vm, "LAST_N", nh);
        var_set_num(vm, "EACH_N", nh);
        var_set_num(vm, "EACHTOPICS_N", nh);
        var_set_num(vm, "OK", 1);
        {
          char bag[256];
          size_t o = 0;
          int k;
          bag[0] = 0;
          for (k = 0; k < nh; k++) {
            size_t ln = strlen(hits[k]);
            if (o && o + 1 < sizeof bag) bag[o++] = '\n';
            if (o + ln < sizeof bag) { memcpy(bag + o, hits[k], ln); o += ln; }
            bag[o] = 0;
          }
          var_set_str(vm, "LAST", bag);
          var_set_str(vm, "EACHTOPICS", bag);
          var_set_str(vm, "LISTTOPICS", bag);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
        }
        bump(vm);
        return 1;
      }
    }
    if (is_lib){
      /* EACH LIB [AS name] [MATCH|FILTER needle] â¦ END
       * Walk sorted lib stems (programs/lib + CUBALC_INCLUDE_PATH).
       * Optional MATCH filter (case-insensitive stem/path) Â· no filter = all stems.
       * Binds name (default LIB) + LINE/STEM Â· IT 0-based Â· LIB_N 1-based Â· EACHLIBS_N.
       * Usability: no MATCHLIBS/LISTLIBS + EACH LINE glue for INCLUDE/recipe walks. */
      char lname[48], needle[96], fup[96];
      char stems[96][96];
      int nstem = 0, i, j;
      long idx = 0;
      size_t a;
      DIR *d;
      struct dirent *ent;
      const char *ip;
      snprintf(lname, sizeof lname, "LIB");
      needle[0] = 0;
      fup[0] = 0;
      if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
        lex_next(L);
        if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH LIB as name"); return -1; }
        snprintf(lname, sizeof lname, "%s", L->cur.text); lex_next(L);
      } else if (L->cur.kind==TK_IDENT &&
                 strcmp(L->cur.text,"MATCH")!=0 && strcmp(L->cur.text,"FILTER")!=0 &&
                 strcmp(L->cur.text,"WHERE")!=0 && strcmp(L->cur.text,"WITH")!=0 &&
                 strcmp(L->cur.text,"OF")!=0 && strcmp(L->cur.text,"IN")!=0 &&
                 strcmp(L->cur.text,"FROM")!=0 && strcmp(L->cur.text,"END")!=0){
        /* bare bind name if not a filter keyword */
        snprintf(lname, sizeof lname, "%s", L->cur.text); lex_next(L);
      }
      if (kw(&L->cur,"MATCH")||kw(&L->cur,"FILTER")||kw(&L->cur,"WHERE")||
          kw(&L->cur,"WITH")||kw(&L->cur,"OF")||kw(&L->cur,"IN")||kw(&L->cur,"FROM")||
          kw(&L->cur,"LIKE")||kw(&L->cur,"FOR")){
        lex_next(L);
        if (L->cur.kind == TK_STR) {
          snprintf(needle, sizeof needle, "%s", L->cur.text);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(needle, sizeof needle, "%s", vv->sval);
          else if (strcmp(L->cur.text, "LAST") == 0)
            snprintf(needle, sizeof needle, "%s", vm->last_str);
          else
            snprintf(needle, sizeof needle, "%s", L->cur.text);
          lex_next(L);
        }
      }
      if (needle[0]) {
        for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
          char c = needle[a];
          if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
          fup[a] = c;
        }
        fup[a] = 0;
      }
      {
        char dirs[10][160];
        int nd = 0, di;
        snprintf(dirs[nd++], sizeof dirs[0], "%s", "programs/lib");
        ip = getenv("CUBALC_INCLUDE_PATH");
        if (ip && ip[0]) {
          const char *p = ip;
          while (*p && nd < 10) {
            char dir[160];
            size_t len = 0;
            while (*p == ':' || *p == ' ' || *p == '\t') p++;
            if (!*p) break;
            while (p[len] && p[len] != ':' && len + 1 < sizeof dir) {
              dir[len] = p[len];
              len++;
            }
            dir[len] = 0;
            p += len;
            if (dir[0])
              snprintf(dirs[nd++], sizeof dirs[0], "%s", dir);
          }
        }
        for (di = 0; di < nd; di++) {
          d = opendir(dirs[di]);
          if (!d) continue;
          while ((ent = readdir(d)) != NULL && nstem < 96) {
            size_t len = strlen(ent->d_name);
            char stem[96], hay[192];
            int dup = 0;
            size_t b;
            if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
              continue;
            if (ent->d_name[0] == '.') continue;
            if (len - 7 >= sizeof stem) continue;
            memcpy(stem, ent->d_name, len - 7);
            stem[len - 7] = 0;
            for (j = 0; j < nstem; j++) {
              if (strcmp(stems[j], stem) == 0) { dup = 1; break; }
            }
            if (dup) continue;
            if (fup[0]) {
              snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
              for (b = 0; hay[b]; b++)
                if (hay[b] >= 'A' && hay[b] <= 'Z')
                  hay[b] = (char)(hay[b] - 'A' + 'a');
              {
                char stem_l[96];
                for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
                  char c = stem[b];
                  if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
                  stem_l[b] = c;
                }
                stem_l[b] = 0;
                if (!strstr(stem_l, fup) && !strstr(hay, fup))
                  continue;
              }
            }
            snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
          }
          closedir(d);
        }
      }
      for (i = 1; i < nstem; i++) {
        char tmp[96];
        snprintf(tmp, sizeof tmp, "%s", stems[i]);
        j = i;
        while (j > 0 && strcmp(stems[j - 1], tmp) > 0) {
          snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
          j--;
        }
        snprintf(stems[j], sizeof stems[0], "%s", tmp);
      }
      skip_nl(L);
      {
        Lex body_start = *L;
        int depth = 1;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "EACH LIB without END"); return -1; }
        var_set_num(vm, "EACHLIBS_N", nstem);
        var_set_num(vm, "MATCHLIBS_N", nstem);
        if (needle[0]) {
          var_set_str(vm, "MATCHLIBS_FILTER", needle);
          var_set_str(vm, "EACHLIBS_FILTER", needle);
        } else {
          var_set_str(vm, "EACHLIBS_FILTER", "");
        }
        for (i = 0; i < nstem && !vm->fatal && !vm->halt; i++) {
          var_set_str(vm, lname, stems[i]);
          var_set_str(vm, "LIB", stems[i]);
          var_set_str(vm, "LINE", stems[i]);
          var_set_str(vm, "STEM", stems[i]);
          var_set_num(vm, "IT", idx);
          var_set_num(vm, "IDX", idx);
          var_set_num(vm, "LIB_N", idx + 1);
          var_set_num(vm, "LINE_N", idx + 1);
          var_set_num(vm, "OK", 1);
          vm->break_loop = 0;
          vm->continue_loop = 0;
          {
            Lex body = body_start;
            if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
          }
          if (vm->break_loop) { vm->break_loop = 0; break; }
          vm->continue_loop = 0;
          idx++;
        }
        if (kw(&L->cur, "END")) lex_next(L);
        var_set_num(vm, "LAST_N", nstem);
        var_set_num(vm, "EACH_N", nstem);
        var_set_num(vm, "OK", 1);
        {
          char bag[4096];
          size_t o = 0;
          bag[0] = 0;
          for (i = 0; i < nstem; i++) {
            size_t ln = strlen(stems[i]);
            if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
            if (o + ln < sizeof bag) {
              memcpy(bag + o, stems[i], ln);
              o += ln;
            }
            bag[o] = 0;
          }
          var_set_str(vm, "LAST", bag);
          var_set_str(vm, "EACHLIBS", bag);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
          vm->last_n = nstem;
        }
        bump(vm);
        return 1;
      }
    }
    if (is_key){
      /* EACH KEY|KEYS [AS name] OF|FROM plate|LAST|PLATE [path] ... END
       * EACH KEYNEST|NESTKEY [AS name] nest [FROM plate] ... END
       * EACH KEY OF NEST nest [FROM plate] ... END
       * Binds KEY (+ VALUE/VAL) per field. KEYNEST peels nested object first.
       * Nest path dotted/slash ok via path_obj (soft miss â empty walk):
       *   EACH KEYNEST "cfg.flags" ... END
       *   EACH KEY OF PLATE "meta" ... END
       *   EACH KEY OF NEST "cfg" FROM PEER ... END
       * Soft empty if non-object. Usability: no KEYSP/KEYSOBJ+EACH LINE glue.
       */
      char bind[48], plate[CUBALC_HOST_STR_MAX], keys_snap[CUBALC_HOST_STR_MAX];
      char nestk[192], outer[CUBALC_HOST_STR_MAX];
      cubalc_host_result kr, gr, ngr;
      const char *p, *startp;
      long idx = 0, nkeys = 0;
      int have_src = 0, nest_mode = is_keynest ? 1 : 0;
      Var *pv;
      snprintf(bind, sizeof bind, "KEY");
      plate[0] = 0; keys_snap[0] = 0; nestk[0] = 0; outer[0] = 0;
      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH KEY as name"); return -1; }
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !kw(&L->cur, "OF") &&
                 !kw(&L->cur, "ON") && !kw(&L->cur, "IN") &&
                 !kw(&L->cur, "FROM") && !kw(&L->cur, "END") &&
                 !kw(&L->cur, "NEST") &&
                 strcmp(L->cur.text, "->") != 0) {
        /* bare bind only for top-level EACH KEY name OF â¦ â not nest key */
        if (!nest_mode) {
          snprintf(bind, sizeof bind, "%s", L->cur.text);
          lex_next(L);
        }
      }
      if (nest_mode) {
        /* EACH KEYNEST [AS name] nest [FROM plate] */
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long kv = parse_expr(vm, L);
          snprintf(nestk, sizeof nestk, "%ld", kv);
        } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (kw(&L->cur, "FROM") || kw(&L->cur, "OF") || kw(&L->cur, "END")) {
            fail(vm, "EACH KEYNEST nest â need nest key");
            return -1;
          }
          if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
            nestk[0] = 0;
        } else {
          fail(vm, "EACH KEYNEST nest â need nest key");
          return -1;
        }
        if (kw(&L->cur, "FROM") || kw(&L->cur, "OF") || kw(&L->cur, "ON") ||
            kw(&L->cur, "IN") || kw(&L->cur, "USING")) {
          lex_next(L);
          have_src = 1;
          if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
            snprintf(outer, sizeof outer, "%s", vm->last_str);
            lex_next(L);
          } else if (L->cur.kind == TK_IDENT) {
            pv = var_get(vm, L->cur.text, 0);
            if (pv && pv->is_str) {
              snprintf(outer, sizeof outer, "%s", pv->sval);
              lex_next(L);
            } else if (pv) {
              snprintf(outer, sizeof outer, "%ld", pv->val);
              lex_next(L);
            } else if (resolve_str_arg(vm, L, outer, sizeof outer) != 0) {
              outer[0] = 0;
            }
          } else if (resolve_str_arg(vm, L, outer, sizeof outer) != 0) {
            snprintf(outer, sizeof outer, "%s", vm->last_str);
          }
        } else {
          pv = var_get(vm, "PLATE", 0);
          if (pv && pv->is_str && pv->sval[0])
            snprintf(outer, sizeof outer, "%s", pv->sval);
          else
            snprintf(outer, sizeof outer, "%s", vm->last_str);
        }
      } else if (kw(&L->cur, "OF") || kw(&L->cur, "ON") || kw(&L->cur, "IN") ||
          kw(&L->cur, "FROM") || kw(&L->cur, "FOR") || kw(&L->cur, "OVER")) {
        lex_next(L);
        if (kw(&L->cur, "NEST") || kw(&L->cur, "NESTED") ||
            kw(&L->cur, "SUB") || kw(&L->cur, "CHILD") || kw(&L->cur, "INNER")) {
          /* EACH KEY OF NEST nest [FROM plate] â not EACH KEY OF OBJ (prop plane) */
          lex_next(L);
          nest_mode = 1;
          if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
            long kv = parse_expr(vm, L);
            snprintf(nestk, sizeof nestk, "%ld", kv);
          } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
            if (kw(&L->cur, "FROM") || kw(&L->cur, "END")) {
              fail(vm, "EACH KEY OF NEST nest â need nest key");
              return -1;
            }
            if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
              nestk[0] = 0;
          } else {
            fail(vm, "EACH KEY OF NEST nest â need nest key");
            return -1;
          }
          if (kw(&L->cur, "FROM") || kw(&L->cur, "USING") || kw(&L->cur, "OF")) {
            lex_next(L);
            have_src = 1;
            if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
              snprintf(outer, sizeof outer, "%s", vm->last_str);
              lex_next(L);
            } else if (L->cur.kind == TK_IDENT) {
              pv = var_get(vm, L->cur.text, 0);
              if (pv && pv->is_str) {
                snprintf(outer, sizeof outer, "%s", pv->sval);
                lex_next(L);
              } else if (pv) {
                snprintf(outer, sizeof outer, "%ld", pv->val);
                lex_next(L);
              } else if (resolve_str_arg(vm, L, outer, sizeof outer) != 0) {
                outer[0] = 0;
              }
            } else if (resolve_str_arg(vm, L, outer, sizeof outer) != 0) {
              snprintf(outer, sizeof outer, "%s", vm->last_str);
            }
          } else {
            pv = var_get(vm, "PLATE", 0);
            if (pv && pv->is_str && pv->sval[0])
              snprintf(outer, sizeof outer, "%s", pv->sval);
            else
              snprintf(outer, sizeof outer, "%s", vm->last_str);
          }
        } else {
          have_src = 1;
          if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
            snprintf(plate, sizeof plate, "%s", vm->last_str);
            lex_next(L);
          } else if (L->cur.kind == TK_IDENT) {
            pv = var_get(vm, L->cur.text, 0);
            if (pv && pv->is_str) {
              snprintf(plate, sizeof plate, "%s", pv->sval);
              lex_next(L);
            } else if (pv) {
              snprintf(plate, sizeof plate, "%ld", pv->val);
              lex_next(L);
            } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
              plate[0] = 0;
            }
          } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
            snprintf(plate, sizeof plate, "%s", vm->last_str);
          }
          /* optional nest path after plate: EACH KEY OF PLATE "cfg.flags" */
          if (L->cur.kind == TK_STR ||
              (L->cur.kind == TK_IDENT &&
               !kw(&L->cur, "AS") && !kw(&L->cur, "END") &&
               !kw(&L->cur, "FROM") && !kw(&L->cur, "OF") &&
               strcmp(L->cur.text, "->") != 0)) {
            if (resolve_str_arg(vm, L, nestk, sizeof nestk) == 0 && nestk[0]) {
              nest_mode = 1;
              snprintf(outer, sizeof outer, "%s", plate);
            }
          }
        }
      } else {
        /* bare: conventional PLATE, else LAST object */
        pv = var_get(vm, "PLATE", 0);
        if (pv && pv->is_str && pv->sval[0])
          snprintf(plate, sizeof plate, "%s", pv->sval);
        else
          snprintf(plate, sizeof plate, "%s", vm->last_str);
        /* optional nest path after bare: EACH KEY "cfg" (only if string / not AS) */
        if (L->cur.kind == TK_STR) {
          if (resolve_str_arg(vm, L, nestk, sizeof nestk) == 0 && nestk[0]) {
            nest_mode = 1;
            snprintf(outer, sizeof outer, "%s", plate);
          }
        }
      }
      if (nest_mode) {
        const char *bp;
        if (!nestk[0]) {
          snprintf(plate, sizeof plate, "%s", "{}");
        } else {
          bp = outer[0] ? outer : plate;
          while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
          if (*bp != '{')
            snprintf(outer, sizeof outer, "%s", "{}");
          else if (!outer[0])
            snprintf(outer, sizeof outer, "%s", plate);
          /* path-aware peel: "cfg.flags" / "a/b" Â· soft miss â {} */
          memset(&ngr, 0, sizeof ngr);
          cubalc_host_json_path_obj(outer, nestk, &ngr);
          snprintf(plate, sizeof plate, "%s", ngr.str);
        }
        var_set_str(vm, "NEST", nestk);
        var_set_str(vm, "KEYNEST", nestk);
        var_set_str(vm, "KEY_PATH", nestk);
      }
      {
        const char *bp = plate;
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
        if (*bp != '{')
          snprintf(plate, sizeof plate, "%s", "{}");
      }
      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH KEY as name"); return -1; }
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      }
      skip_nl(L);
      {
        Lex body_start = *L;
        int depth = 1;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "EACH KEY without END"); return -1; }
        memset(&kr, 0, sizeof kr);
        if (cubalc_host_json_keys(plate, &kr) == 0 && kr.str[0])
          snprintf(keys_snap, sizeof keys_snap, "%s", kr.str);
        p = keys_snap;
        while (*p && !vm->fatal && !vm->halt) {
          char key[256];
          size_t kn = 0;
          while (*p == '\n' || *p == '\r') p++;
          if (!*p) break;
          startp = p;
          while (*p && *p != '\n' && *p != '\r') p++;
          kn = (size_t)(p - startp);
          if (kn >= sizeof key) kn = sizeof key - 1;
          memcpy(key, startp, kn);
          key[kn] = 0;
          if (!key[0]) {
            if (*p == '\n' || *p == '\r') p++;
            continue;
          }
          var_set_str(vm, bind, key);
          var_set_str(vm, "KEY", key);
          var_set_str(vm, "K", key);
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get(plate, key, &gr) == 0) {
            var_set_str(vm, "VALUE", gr.str);
            var_set_str(vm, "VAL", gr.str);
            var_set_str(vm, "V", gr.str);
          } else {
            var_set_str(vm, "VALUE", "");
            var_set_str(vm, "VAL", "");
            var_set_str(vm, "V", "");
          }
          var_set_num(vm, "IT", idx);
          var_set_num(vm, "IDX", idx);
          var_set_num(vm, "KEY_N", idx + 1);
          var_set_num(vm, "OK", 1);
          vm->break_loop = 0;
          vm->continue_loop = 0;
          {
            Lex body = body_start;
            if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
          }
          if (vm->break_loop) { vm->break_loop = 0; idx++; nkeys = idx; break; }
          vm->continue_loop = 0;
          idx++;
          nkeys = idx;
          if (*p == '\n' || *p == '\r') p++;
        }
        if (kw(&L->cur, "END")) lex_next(L);
        var_set_num(vm, "LAST_N", nkeys);
        var_set_num(vm, "EACH_N", nkeys);
        var_set_num(vm, "NKEYS", nkeys);
        var_set_num(vm, "KEYNEST_MODE", nest_mode ? 1 : 0);
        var_set_num(vm, "OK", 1);
        (void)have_src;
        bump(vm);
        return 1;
      }
    }
    if (is_flat){
      /* EACH FLAT|LEAF [AS name] [needle] [OF|FROM plate] ... END
       * Walk recursive leaf path:value pairs whose path contains needle.
       * Empty needle â all leaves. Soft empty walk.
       * Binds PATH (default bind) + VALUE/VAL/V + LEAF + IT/IDX + FLAT_N.
       * Usability: nest leaf walk without PATHSFLAT+EACH LINE+GETP glue:
       *   EACH FLAT "role" ... END
       *   EACH LEAF FROM PEER ... END
       *   EACH FLAT OF PLATE "host" AS p ... END
       *   EACH FLAT AS p "name" FROM PEER ... END
       */
      char bind[48], plate[CUBALC_HOST_STR_MAX], bag[CUBALC_HOST_STR_MAX];
      char needle[192], from_name[96];
      cubalc_host_result hr;
      const char *p, *line;
      long idx = 0, nleaf = 0;
      int have_src = 0, have_needle = 0;
      Var *pv;
      snprintf(bind, sizeof bind, "PATH");
      plate[0] = 0; bag[0] = 0; needle[0] = 0; from_name[0] = 0;

      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH FLAT as name"); return -1; }
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !kw(&L->cur, "OF") &&
                 !kw(&L->cur, "ON") && !kw(&L->cur, "IN") &&
                 !kw(&L->cur, "FROM") && !kw(&L->cur, "END") &&
                 !kw(&L->cur, "USING") && !kw(&L->cur, "OVER") &&
                 strcmp(L->cur.text, "->") != 0) {
        /* bare bind name before OF/FROM/needle â not a plate var used as source */
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      }

      /* optional needle before source */
      if (L->cur.kind == TK_STR) {
        if (resolve_str_arg(vm, L, needle, sizeof needle) == 0)
          have_needle = 1;
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", kv);
        have_needle = 1;
      }

      if (kw(&L->cur, "OF") || kw(&L->cur, "ON") || kw(&L->cur, "IN") ||
          kw(&L->cur, "FROM") || kw(&L->cur, "FOR") || kw(&L->cur, "OVER") ||
          kw(&L->cur, "USING")) {
        lex_next(L);
        have_src = 1;
        if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
          snprintf(plate, sizeof plate, "%s", vm->last_str);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          pv = var_get(vm, L->cur.text, 0);
          if (pv && pv->is_str) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(plate, sizeof plate, "%s", pv->sval);
            lex_next(L);
          } else if (pv) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(plate, sizeof plate, "%ld", pv->val);
            lex_next(L);
          } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
            plate[0] = 0;
          }
        } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
          snprintf(plate, sizeof plate, "%s", vm->last_str);
        }
      } else {
        /* bare: conventional PLATE, else LAST object */
        pv = var_get(vm, "PLATE", 0);
        if (pv && pv->is_str && pv->sval[0])
          snprintf(plate, sizeof plate, "%s", pv->sval);
        else
          snprintf(plate, sizeof plate, "%s", vm->last_str);
      }

      /* optional needle after source */
      if (!have_needle) {
        if (L->cur.kind == TK_STR) {
          if (resolve_str_arg(vm, L, needle, sizeof needle) == 0)
            have_needle = 1;
        } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long kv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", kv);
          have_needle = 1;
        } else if (L->cur.kind == TK_IDENT &&
                   !kw(&L->cur, "AS") && !kw(&L->cur, "END") &&
                   !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
                   !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
                   !kw(&L->cur, "IF") && !kw(&L->cur, "PASS") &&
                   !kw(&L->cur, "EACH") && !kw(&L->cur, "FOR") &&
                   strcmp(L->cur.text, "->") != 0) {
          /* only accept string-ish idents as needle, not statement starters */
          if (resolve_str_arg(vm, L, needle, sizeof needle) == 0)
            have_needle = 1;
        }
      }

      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH FLAT as name"); return -1; }
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      }

      {
        const char *bp = plate;
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
        if (*bp != '{')
          snprintf(plate, sizeof plate, "%s", "{}");
      }

      skip_nl(L);
      {
        Lex body_start = *L;
        int depth = 1;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "EACH FLAT without END"); return -1; }

        memset(&hr, 0, sizeof hr);
        /* invert=0, icase=0 â path needle filter Â· empty needle â all leaves */
        cubalc_host_json_leaf_grep(plate, needle, 0, 0, &hr);
        if (hr.str[0])
          snprintf(bag, sizeof bag, "%s", hr.str);
        else
          bag[0] = 0;

        p = bag;
        while (*p && !vm->fatal && !vm->halt) {
          char path[512], val[CUBALC_HOST_STR_MAX / 2];
          size_t kn = 0, vn = 0;
          const char *sep, *vs, *ve;
          while (*p == '\n' || *p == '\r') p++;
          if (!*p) break;
          line = p;
          while (*p && *p != '\n' && *p != '\r') p++;
          sep = NULL;
          {
            const char *s = line;
            while (s < p) {
              if (*s == ':' || *s == '=') {
                sep = s;
                break;
              }
              s++;
            }
          }
          kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
          while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
          {
            size_t sk = 0;
            while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
            if (sk) {
              line += sk;
              kn -= sk;
            }
          }
          if (kn == 0 || kn >= sizeof path) {
            if (*p == '\n' || *p == '\r') p++;
            continue;
          }
          memcpy(path, line, kn);
          path[kn] = 0;
          vs = sep ? sep + 1 : p;
          ve = p;
          while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
          while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
          vn = (size_t)(ve - vs);
          if (vn >= sizeof val) vn = sizeof val - 1;
          memcpy(val, vs, vn);
          val[vn] = 0;

          var_set_str(vm, bind, path);
          var_set_str(vm, "PATH", path);
          var_set_str(vm, "LEAF", path);
          var_set_str(vm, "LEAFPATH", path);
          var_set_str(vm, "FLAT_PATH", path);
          var_set_str(vm, "VALUE", val);
          var_set_str(vm, "VAL", val);
          var_set_str(vm, "V", val);
          var_set_str(vm, "FLAT_VALUE", val);
          var_set_num(vm, "IT", idx);
          var_set_num(vm, "IDX", idx);
          var_set_num(vm, "FLAT_N", idx + 1);
          var_set_num(vm, "LEAF_N", idx + 1);
          var_set_str(vm, "FLAT_NEEDLE", needle);
          var_set_num(vm, "OK", 1);
          vm->break_loop = 0;
          vm->continue_loop = 0;
          {
            Lex body = body_start;
            if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
          }
          if (vm->break_loop) { vm->break_loop = 0; idx++; nleaf = idx; break; }
          vm->continue_loop = 0;
          idx++;
          nleaf = idx;
          if (*p == '\n' || *p == '\r') p++;
        }
        if (kw(&L->cur, "END")) lex_next(L);
        var_set_num(vm, "LAST_N", nleaf);
        var_set_num(vm, "EACH_N", nleaf);
        var_set_num(vm, "NFLAT", nleaf);
        var_set_num(vm, "NLEAVES", nleaf);
        var_set_str(vm, "FLAT_NEEDLE", needle);
        var_set_num(vm, "FLAT_FROM", have_src ? 1 : 0);
        var_set_str(vm, "FLAT_SRC",
                    from_name[0] ? from_name : (have_src ? "" : "PLATE"));
        var_set_num(vm, "OK", 1);
        (void)have_needle;
        bump(vm);
        return 1;
      }
    }
    if (is_prop){
      /* EACH PROP|ATTR|OBJFIELD OF obj|Class [AS name] ... END
       * Binds each CLASS field name to name (default PROP); IT=0-based; PROP_N=1-based.
       * OF required: live object (uses its class) or CLASS name.
       * Soft empty walk if unknown. Usability: no LISTFIELDS+EACH LINE glue for schema walk.
       * Distinct from EACH FIELD (LINE synonym â bag lines). */
      char bind[48], target[48];
      ClassDef *cd = NULL;
      ObjInst *ob = NULL;
      int i, n = 0;
      snprintf(bind, sizeof bind, "PROP");
      target[0] = 0;
      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH PROP as name"); return -1; }
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !kw(&L->cur, "OF") &&
                 !kw(&L->cur, "ON") && !kw(&L->cur, "IN") &&
                 !kw(&L->cur, "FROM") && !kw(&L->cur, "END") &&
                 strcmp(L->cur.text, "->") != 0) {
        /* EACH PROP fname OF obj â bind name before OF */
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      }
      if (kw(&L->cur, "OF") || kw(&L->cur, "ON") || kw(&L->cur, "IN") ||
          kw(&L->cur, "FROM") || kw(&L->cur, "FOR"))
        lex_next(L);
      /* target: THIS/SELF, live obj, or Class name */
      if (strcasecmp(L->cur.text, "THIS") == 0 ||
          strcasecmp(L->cur.text, "SELF") == 0) {
        if (vm->this_obj[0])
          snprintf(target, sizeof target, "%s", vm->this_obj);
        lex_next(L);
      } else if (L->cur.kind == TK_STR) {
        snprintf(target, sizeof target, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        /* prefer live object, then class; expand string var if live obj */
        Var *sv;
        snprintf(target, sizeof target, "%s", L->cur.text);
        lex_next(L);
        sv = var_get(vm, target, 0);
        if (sv && sv->is_str && sv->sval[0] && oop_find_obj(vm, sv->sval))
          snprintf(target, sizeof target, "%s", sv->sval);
      } else {
        fail(vm, "EACH PROP OF obj|Class"); return -1;
      }
      if (target[0]) {
        ob = oop_find_obj(vm, target);
        if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
          cd = &vm->classes[ob->class_idx];
        else
          cd = oop_find_class(vm, target);
      }
      /* AS after OF: EACH PROP OF q AS f */
      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH PROP as name"); return -1; }
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      }
      skip_nl(L);
      {
        Lex body_start = *L;
        int depth = 1;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "EACH PROP without END"); return -1; }
        if (cd) {
          for (i = 0; i < cd->n_fields && !vm->fatal && !vm->halt; i++) {
            const char *fname = cd->fields[i].name;
            var_set_str(vm, bind, fname);
            var_set_str(vm, "PROP", fname);
            var_set_str(vm, "FIELD", fname);
            var_set_str(vm, "ATTR", fname);
            var_set_str(vm, "CLASS", cd->name);
            if (ob) {
              var_set_str(vm, "OBJECT", ob->name);
              var_set_str(vm, "OBJ", ob->name);
            }
            var_set_num(vm, "IT", n);
            var_set_num(vm, "IDX", n);
            var_set_num(vm, "PROP_N", n + 1);
            var_set_num(vm, "FIELD_N", n + 1);
            var_set_num(vm, "OK", 1);
            vm->break_loop = 0;
            vm->continue_loop = 0;
            {
              Lex body = body_start;
              if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
            }
            if (vm->break_loop) { vm->break_loop = 0; n++; break; }
            vm->continue_loop = 0;
            n++;
          }
        }
        if (kw(&L->cur, "END")) lex_next(L);
        var_set_num(vm, "LAST_N", n);
        var_set_num(vm, "EACH_N", n);
        var_set_num(vm, "NFIELDS", n);
        var_set_num(vm, "OK", cd ? 1 : 0);
        if (!cd) {
          var_set_str(vm, "LAST_ERR", "EACH PROP: unknown obj/class");
          var_set_str(vm, "ERR", "EACH PROP: unknown obj/class");
        }
        bump(vm);
        return 1;
      }
    }
    if (is_meth){
      /* EACH METHOD|MSG|HANDLER OF obj|Class [AS name] ... END
       * Binds each CLASS method name to name (default METHOD); IT=0-based; METHOD_N=1-based.
       * OF required: live object or CLASS name. Soft empty if unknown.
       * Usability: no LISTMETHODS+EACH LINE glue before SEND/TRYSEND
       * (twin of EACH PROP; METHOD/THIS). */
      char bind[48], target[48];
      ClassDef *cd = NULL;
      ObjInst *ob = NULL;
      int i, n = 0;
      snprintf(bind, sizeof bind, "METHOD");
      target[0] = 0;
      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH METHOD as name"); return -1; }
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !kw(&L->cur, "OF") &&
                 !kw(&L->cur, "ON") && !kw(&L->cur, "IN") &&
                 !kw(&L->cur, "FROM") && !kw(&L->cur, "END") &&
                 strcmp(L->cur.text, "->") != 0) {
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      }
      if (kw(&L->cur, "OF") || kw(&L->cur, "ON") || kw(&L->cur, "IN") ||
          kw(&L->cur, "FROM") || kw(&L->cur, "FOR"))
        lex_next(L);
      if (strcasecmp(L->cur.text, "THIS") == 0 ||
          strcasecmp(L->cur.text, "SELF") == 0) {
        if (vm->this_obj[0])
          snprintf(target, sizeof target, "%s", vm->this_obj);
        lex_next(L);
      } else if (L->cur.kind == TK_STR) {
        snprintf(target, sizeof target, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *sv;
        snprintf(target, sizeof target, "%s", L->cur.text);
        lex_next(L);
        sv = var_get(vm, target, 0);
        if (sv && sv->is_str && sv->sval[0] && oop_find_obj(vm, sv->sval))
          snprintf(target, sizeof target, "%s", sv->sval);
      } else {
        fail(vm, "EACH METHOD OF obj|Class"); return -1;
      }
      if (target[0]) {
        ob = oop_find_obj(vm, target);
        if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
          cd = &vm->classes[ob->class_idx];
        else
          cd = oop_find_class(vm, target);
      }
      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH METHOD as name"); return -1; }
        snprintf(bind, sizeof bind, "%s", L->cur.text);
        lex_next(L);
      }
      skip_nl(L);
      {
        Lex body_start = *L;
        int depth = 1;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "EACH METHOD without END"); return -1; }
        if (cd) {
          for (i = 0; i < cd->n_methods && !vm->fatal && !vm->halt; i++) {
            const char *mname = cd->methods[i].name;
            var_set_str(vm, bind, mname);
            var_set_str(vm, "METHOD", mname);
            var_set_str(vm, "MSG", mname);
            var_set_str(vm, "HANDLER", mname);
            var_set_str(vm, "CLASS", cd->name);
            if (ob) {
              var_set_str(vm, "OBJECT", ob->name);
              var_set_str(vm, "OBJ", ob->name);
            }
            var_set_num(vm, "IT", n);
            var_set_num(vm, "IDX", n);
            var_set_num(vm, "METHOD_N", n + 1);
            var_set_num(vm, "MSG_N", n + 1);
            var_set_num(vm, "OK", 1);
            vm->break_loop = 0;
            vm->continue_loop = 0;
            {
              Lex body = body_start;
              if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
            }
            if (vm->break_loop) { vm->break_loop = 0; n++; break; }
            vm->continue_loop = 0;
            n++;
          }
        }
        if (kw(&L->cur, "END")) lex_next(L);
        var_set_num(vm, "LAST_N", n);
        var_set_num(vm, "EACH_N", n);
        var_set_num(vm, "NMETHODS", n);
        var_set_num(vm, "OK", cd ? 1 : 0);
        if (!cd) {
          var_set_str(vm, "LAST_ERR", "EACH METHOD: unknown obj/class");
          var_set_str(vm, "ERR", "EACH METHOD: unknown obj/class");
        }
        bump(vm);
        return 1;
      }
    }
    if (is_obj){
      /* EACH OBJ [ClassName] [AS name] ... END
       * Binds each live object name to name (default OBJ); IT=0-based; OBJ_N=1-based.
       * Optional Class filter like LISTOBJS. Usability: no LISTOBJS+EACH LINE glue. */
      char oname_bind[48], filt[48];
      int has_filt = 0, i, n = 0;
      snprintf(oname_bind, sizeof oname_bind, "OBJ");
      filt[0] = 0;
      /* optional class filter before AS (bare IDENT = class filter, even if
       * unknown â empty walk; use EACH OBJ AS name for unfiltered bind). */
      if (L->cur.kind == TK_IDENT && !kw(&L->cur, "AS") &&
          strcmp(L->cur.text, "->") != 0 && !kw(&L->cur, "OF") &&
          !kw(&L->cur, "END")) {
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      } else if (L->cur.kind == TK_STR) {
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
      if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
        lex_next(L);
        if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
          snprintf(filt, sizeof filt, "%s", L->cur.text);
          lex_next(L);
          has_filt = 1;
        }
      }
      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH OBJ as name"); return -1; }
        snprintf(oname_bind, sizeof oname_bind, "%s", L->cur.text);
        lex_next(L);
      }
      /* class filter after AS: EACH OBJ AS c OF Cell */
      if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
        lex_next(L);
        if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
          snprintf(filt, sizeof filt, "%s", L->cur.text);
          lex_next(L);
          has_filt = 1;
        }
      }
      skip_nl(L);
      {
        Lex body_start = *L;
        int depth = 1;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "EACH OBJ without END"); return -1; }
        for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
          ObjInst *ob = &vm->objs[i];
          ClassDef *cd;
          if (!ob->live) continue;
          if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
          cd = &vm->classes[ob->class_idx];
          if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
          var_set_str(vm, oname_bind, ob->name);
          var_set_str(vm, "OBJ", ob->name);
          var_set_str(vm, "OBJECT", ob->name);
          var_set_str(vm, "CLASS", cd->name);
          var_set_num(vm, "IT", n);
          var_set_num(vm, "IDX", n);
          var_set_num(vm, "OBJ_N", n + 1);
          var_set_num(vm, "OK", 1);
          vm->break_loop = 0;
          vm->continue_loop = 0;
          {
            Lex body = body_start;
            if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
          }
          if (vm->break_loop) { vm->break_loop = 0; n++; break; }
          vm->continue_loop = 0;
          n++;
        }
        if (kw(&L->cur, "END")) lex_next(L);
        var_set_num(vm, "LAST_N", n);
        var_set_num(vm, "EACH_N", n);
        var_set_num(vm, "NOBJS", n);
        var_set_num(vm, "OK", 1);
        bump(vm);
        return 1;
      }
    }
    if (is_line){
      /* EACH LINE [AS name] [IN|OF|FROM str|obj field] ... END
       * Binds each newline field to name (default LINE); IT=0-based, LINE_N=1-based.
       * Source defaults to LAST/last_str â chain after SYS LIST / SYS GREP.
       * OF obj field: walk bag-in-field without GETF first (METHOD/THIS).
       * Snapshot source before loop so body may clobber LAST. */
      char lname[48];
      char src[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      long idx = 0, nlines = 0;
      snprintf(lname, sizeof lname, "LINE");
      if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
        lex_next(L);
        if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH LINE as name"); return -1; }
        snprintf(lname, sizeof lname, "%s", L->cur.text); lex_next(L);
      } else if (L->cur.kind==TK_IDENT &&
                 strcmp(L->cur.text,"IN")!=0 && strcmp(L->cur.text,"OF")!=0 &&
                 strcmp(L->cur.text,"FROM")!=0 && strcmp(L->cur.text,"IN")!=0){
        snprintf(lname, sizeof lname, "%s", L->cur.text); lex_next(L);
      }
      src[0] = 0;
      if (kw(&L->cur,"IN")||kw(&L->cur,"OF")||kw(&L->cur,"FROM")||kw(&L->cur,"OVER")){
        Lex save;
        char oname[48], fname[48];
        int got_field = 0;
        lex_next(L);
        /* Try OF obj field / IN obj field when first name is a live object. */
        save = *L;
        oname[0] = 0;
        fname[0] = 0;
        if (oop_resolve_obj_name(vm, L, oname, sizeof oname) == 0 && oname[0]) {
          ObjInst *ob = oop_find_obj(vm, oname);
          if (ob && (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
              !oop_stmt_kw(L) &&
              !kw(&L->cur, "END") && !kw(&L->cur, "ASSERT") &&
              !kw(&L->cur, "LET") && !kw(&L->cur, "PRINT") &&
              !kw(&L->cur, "SYS") && !kw(&L->cur, "IF") &&
              !kw(&L->cur, "EACH") && !kw(&L->cur, "FOR")) {
            ClassDef *cd;
            int fi;
            if (L->cur.kind == TK_STR) {
              snprintf(fname, sizeof fname, "%s", L->cur.text);
              lex_next(L);
            } else {
              char id[48];
              Var *vv;
              snprintf(id, sizeof id, "%s", L->cur.text);
              lex_next(L);
              vv = var_get(vm, id, 0);
              if (vv && vv->is_str && vv->sval[0])
                snprintf(fname, sizeof fname, "%s", vv->sval);
              else
                snprintf(fname, sizeof fname, "%s", id);
            }
            if (ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
              cd = &vm->classes[ob->class_idx];
              fi = oop_field_idx(cd, fname);
              if (fi >= 0) {
                if (ob->fis_str[fi])
                  snprintf(src, sizeof src, "%s", ob->fstr[fi]);
                else
                  snprintf(src, sizeof src, "%ld", ob->fnum[fi]);
                got_field = 1;
              }
            }
          }
        }
        if (!got_field) {
          *L = save;
          if (resolve_str_arg(vm, L, src, sizeof src) != 0)
            snprintf(src, sizeof src, "%s", vm->last_str);
        }
      } else {
        /* Prefer last_str (full host buffer after LIST/GREP); fallback LAST var. */
        if (vm->last_str[0])
          snprintf(src, sizeof src, "%s", vm->last_str);
        else {
          Var *lv = var_get(vm, "LAST", 0);
          if (lv && lv->is_str)
            snprintf(src, sizeof src, "%s", lv->sval);
        }
      }
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"EACH LINE without END"); return -1; }
      /* Walk fields like SYS NTH/GREP: trailing newline does not add empty last field. */
      if (src[0]) {
        p = src;
        while (*p && !vm->fatal && !vm->halt) {
          start = p;
          while (*p && *p != '\n') p++;
          /* end-of-string with empty span after a newline â stop (no field) */
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            char field[512];
            if (flen >= sizeof field) flen = sizeof field - 1;
            memcpy(field, start, flen);
            field[flen] = 0;
            var_set_str(vm, lname, field);
            var_set_str(vm, "LINE", field);
            var_set_num(vm, "IT", idx);
            var_set_num(vm, "IDX", idx);
            var_set_num(vm, "LINE_N", idx + 1);
            var_set_num(vm, "OK", 1);
            vm->break_loop=0; vm->continue_loop=0;
            Lex body=body_start;
            if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
            if (vm->break_loop){ vm->break_loop=0; idx++; nlines = idx; break; }
            vm->continue_loop=0;
            idx++;
            nlines = idx;
          }
          if (*p == '\n') p++;
        }
      }
      if (kw(&L->cur,"END")) lex_next(L);
      var_set_num(vm, "LAST_N", nlines);
      var_set_num(vm, "EACH_N", nlines);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (is_cube){
      if (kw(&L->cur,"AS")||kw(&L->cur,"->")){ lex_next(L); }
      if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH CUBE as name"); return -1; }
      char cname[48]; snprintf(cname,sizeof cname,"%s",L->cur.text); lex_next(L);
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"EACH without END"); return -1; }
      ensure_world(vm);
      for (int i=0;i<vm->ch.n_cubes && !vm->fatal && !vm->halt;i++){
        var_set_str(vm, cname, vm->ch.cubes[i].id);
        var_set_num(vm, "IT", i);
        var_set_num(vm, "DIGIT", vm->ch.cubes[i].atom.digit);
        var_set_num(vm, "ENERGY", (long)lround(vm->ch.cubes[i].atom.energy*100));
        var_set_num(vm, "SET", cubalc_matrix_popcount(&vm->ch.cubes[i].atom.matrix));
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    /* EACH CELL [AS name] [FROM lo TO hi | lo TO hi | lo hi] */
    char vname[48]; snprintf(vname,sizeof vname,"VAL");
    if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
      lex_next(L);
      if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH CELL as name"); return -1; }
      snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    } else if (L->cur.kind==TK_IDENT &&
               strcmp(L->cur.text,"FROM")!=0 && strcmp(L->cur.text,"TO")!=0 &&
               strcmp(L->cur.text,"FROM")!=0){
      /* EACH CELL name lo hi  (optional bare name) */
      /* only if next tokens look like range â keep default VAL when FROM present */
      /* treat bare IDENT as name when not a keyword */
      if (strcmp(L->cur.text,"FROM")!=0){
        snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
      }
    }
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (kw(&L->cur,"FROM")){
      lex_next(L);
      lo = parse_expr(vm,L);
      if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")) lex_next(L);
      hi = parse_expr(vm,L);
    } else if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
               L->cur.kind==TK_MINUS){
      lo = parse_expr(vm,L);
      if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")) lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS)
        hi = parse_expr(vm,L);
      else hi = lo;
    }
    if (lo < 0) lo = 0;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"EACH CELL without END"); return -1; }
    for (long i=lo;i<=hi && !vm->fatal && !vm->halt;i++){
      long val = vm->cells[(int)i];
      var_set_num(vm, vname, val);
      var_set_num(vm, "VAL", val);
      var_set_num(vm, "IT", i);
      var_set_num(vm, "IDX", i);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* FORCELL [name] lo hi ... END â compact cell-range loop (digit-4) */
  if (kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||kw(&L->cur,"FOREACHCELL")){
    lex_next(L);
    char vname[48]; snprintf(vname,sizeof vname,"VAL");
    /* optional bind name if IDENT followed by another expr token */
    if (L->cur.kind==TK_IDENT){
      /* peek: if only one ident then it's lo as var ref â keep VAL */
      char maybe[48]; snprintf(maybe,sizeof maybe,"%s",L->cur.text);
      Lex save=*L;
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS || kw(&L->cur,"TO")||kw(&L->cur,"FROM")){
        snprintf(vname,sizeof vname,"%s",maybe);
      } else {
        *L = save; /* restore â treat as lo expression start */
      }
    }
    long lo = parse_expr(vm,L);
    if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")||kw(&L->cur,"FROM"))
      lex_next(L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FORCELL without END"); return -1; }
    for (long i=lo;i<=hi && !vm->fatal && !vm->halt;i++){
      long val = vm->cells[(int)i];
      var_set_num(vm, vname, val);
      var_set_num(vm, "VAL", val);
      var_set_num(vm, "IT", i);
      var_set_num(vm, "IDX", i);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-4 control: FORBIT/EACHBIT cube [AS name] ... END â iterate set-bit indices */
  if (kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FOREACHBIT")||
      kw(&L->cur,"EACHBITS")||kw(&L->cur,"FORBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FORBIT cube [AS name]"); return -1; }
    char cid[48]; snprintf(cid,sizeof cid,"%s",L->cur.text); lex_next(L);
    char vname[48]; snprintf(vname,sizeof vname,"IT");
    if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
      lex_next(L);
      if (L->cur.kind!=TK_IDENT){ fail(vm,"FORBIT cube AS name"); return -1; }
      snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    }
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FORBIT without END"); return -1; }
    int ix=find_cube(vm,cid);
    if (ix<0){
      /* empty iteration */
      if (kw(&L->cur,"END")) lex_next(L);
      var_set_num(vm,"OK",0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long count = 0;
    for (int bi=0; bi<n && !vm->fatal && !vm->halt; bi++){
      if (!cubalc_matrix_get(m, bi)) continue;
      var_set_num(vm, vname, bi);
      var_set_num(vm, "IT", bi);
      var_set_num(vm, "IDX", bi);
      var_set_num(vm, "BIT", bi);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      count++;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"LAST_N",count); vm->last_n=count;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }

  return 0;
}
