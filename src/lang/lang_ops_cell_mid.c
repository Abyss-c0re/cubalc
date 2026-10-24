/* CubalC CELL mid energy·mat·energyflow meta-try — B_cell_peel from cell.c.
 * Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_mid.h"
#include "lang/cubalc_lang_ops_cell_energy.h"
#include "lang/cubalc_lang_ops_cell_mat.h"
#include "lang/cubalc_lang_ops_cell_energyflow.h"

int cubalc_lang_ops_cell_try_mid(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_cell_try_energy(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_mat(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_energyflow(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
