# SHIP MI 2369 LIFE_CELL_CPT1A PASS
feature: LIFE_CELL_CPT1A
method: cpt1a n
continuum: ...->neu3->glb1->hexa->hexb->gba->asah1->sphk1->sgpl1->aldh3a2->acsl1->cpt1a
domain: post-ACSL1 OMM carnitine palmitoyltransferase CPT1A ward (hexadecenoyl-CoA to hexadecenoyl-carnitine limb toward CACT/CPT2 matrix entry)
gift: CPT1A conjugates post-ACSL1 hexadecenoyl-CoA to carnitine forming hexadecenoyl-carnitine so free energy exits the OMM acyl-CoA pool into the carnitine shuttle under ASSERT without hollow CoA-lock (CPT1A-deficiency ward).
proofs: life=54 atomic=147 mesh=144 total=345 fail=0
files:
  programs/lib/life_cell.cubalc
  programs/proof/2369_life_cell_cpt1a.cubalc
  programs/science/integration/atomic_printer_life_cpt1a.cubalc
  programs/science/integration/smx_mesh_vesicle_cpt1a_life.cubalc
law: cube_is_sot; free_energy_must_flow; hollow_we_heresy; ct101_forever
status: PASS
