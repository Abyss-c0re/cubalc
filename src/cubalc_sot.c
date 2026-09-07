#include "cubalc_sot.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define N 8
#define CELLS 512
#define NAMES 16
static char g_name[NAMES][16];
static unsigned char g_on[NAMES];
static int g_nn;
static unsigned cell_of(const char *n){
  unsigned h=2166136261u;
  while(*n){ h^=(unsigned char)tolower((unsigned char)*n++); h*=16777619u; }
  return h%CELLS;
}
static void mkdir_p(const char *d){
  char t[256]; char *p; snprintf(t,sizeof t,"%s",d);
  for(p=t+1;*p;p++){ if(*p=='/'){ *p=0; mkdir(t,0755); *p='/'; } }
  mkdir(t,0755);
}
static const char *dirp(void){
  static char d[256];
  const char *e=getenv("CUBALC_SOT_DIR");
  if(e&&e[0]) return e;
  if(!d[0]){
#ifdef __ANDROID__
    snprintf(d,sizeof d,"/data/local/tmp/cubebrain_viz");
#else
    const char *h=getenv("HOME");
    if(h&&h[0]) snprintf(d,sizeof d,"%s/.local/share/cubebrain_viz",h);
    else snprintf(d,sizeof d,"/tmp/cubebrain_viz");
#endif
  }
  return d;
}
void cubalc_sot_out(const char *name, int on){
  int i; if(!name||!name[0]) return;
  for(i=0;i<g_nn;i++) if(strcasecmp(g_name[i],name)==0){ g_on[i]=on?1:0; goto wr; }
  if(g_nn<NAMES){ snprintf(g_name[g_nn],16,"%.15s",name); g_on[g_nn]=on?1:0; g_nn++; }
wr: {
    unsigned char cells[CELLS]; FILE *f; char path[300], tmp[300]; int k;
    memset(cells,0,sizeof cells);
    for(i=0;i<g_nn;i++) cells[cell_of(g_name[i])]=g_on[i]?5:0;
    mkdir_p(dirp());
    snprintf(path,sizeof path,"%s/cells.bin",dirp());
    snprintf(tmp,sizeof tmp,"%s.tmp",path);
    f=fopen(tmp,"wb"); if(!f) return;
    fputc(N,f); fwrite(cells,1,CELLS,f); fclose(f); rename(tmp,path);
    snprintf(path,sizeof path,"%s/nodes.tsv",dirp());
    f=fopen(path,"w"); if(f){ fprintf(f,"# idx\tname\n");
      for(i=0;i<g_nn;i++) fprintf(f,"%u\t%s\n",cell_of(g_name[i]),g_name[i]); fclose(f); }
    (void)k;
  }
}
int cubalc_sot_write_cpu(void){
  cubalc_sot_out("cpu",1); return 0;
}
