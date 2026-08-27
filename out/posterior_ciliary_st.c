#include <stdio.h>
extern const char *cubalc_smx_posterior_ciliary_feature(void);
extern const char *cubalc_smx_posterior_ciliary_ship(void);
extern int cubalc_smx_posterior_ciliary_soft_oob_fail_closed(void);
extern int cubalc_smx_posterior_ciliary_mesh_bonds_complete4(void);
extern int cubalc_smx_posterior_ciliary_selftest(void);
int main(void) {
  if (!cubalc_smx_posterior_ciliary_selftest()) { puts("FAIL"); return 1; }
  printf("PASS %s %s soft_oob=%d bonds=%d\n",
    cubalc_smx_posterior_ciliary_feature(),
    cubalc_smx_posterior_ciliary_ship(),
    cubalc_smx_posterior_ciliary_soft_oob_fail_closed(),
    cubalc_smx_posterior_ciliary_mesh_bonds_complete4());
  return 0;
}
