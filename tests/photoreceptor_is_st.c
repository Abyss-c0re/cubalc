#include <stdio.h>
#include <string.h>
const char *cubalc_smx_photoreceptor_is_feature(void);
const char *cubalc_smx_photoreceptor_is_ship(void);
int cubalc_smx_photoreceptor_is_soft_oob_fail_closed(void);
int cubalc_smx_photoreceptor_is_mesh_bonds_complete4(void);
int cubalc_smx_photoreceptor_is_selftest(void);
int main(void) {
  if (!cubalc_smx_photoreceptor_is_selftest()) { puts("FAIL selftest"); return 1; }
  if (cubalc_smx_photoreceptor_is_soft_oob_fail_closed()!=1) return 2;
  if (cubalc_smx_photoreceptor_is_mesh_bonds_complete4()!=6) return 3;
  printf("PASS feature=%s ship=%s soft_oob=1 bonds=6\n",
         cubalc_smx_photoreceptor_is_feature(),
         cubalc_smx_photoreceptor_is_ship());
  return 0;
}
