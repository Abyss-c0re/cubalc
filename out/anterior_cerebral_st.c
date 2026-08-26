#include <stdio.h>
#include <string.h>
const char *cubalc_smx_anterior_cerebral_feature(void);
const char *cubalc_smx_anterior_cerebral_ship(void);
int cubalc_smx_anterior_cerebral_soft_oob_fail_closed(void);
int cubalc_smx_anterior_cerebral_mesh_bonds_complete4(void);
int cubalc_smx_anterior_cerebral_selftest(void);
int main(void) {
  int ok = cubalc_smx_anterior_cerebral_selftest();
  printf("selftest=%d feature=%s ship=%s soft_oob=%d bonds=%d\n", ok,
    cubalc_smx_anterior_cerebral_feature(), cubalc_smx_anterior_cerebral_ship(),
    cubalc_smx_anterior_cerebral_soft_oob_fail_closed(), cubalc_smx_anterior_cerebral_mesh_bonds_complete4());
  return ok ? 0 : 1;
}
