# SMX soft-OOB mesh stability

Soft-OOB heal path keeps mesh life-force above floor under replay/compat stress.
Fail-closed without key. HOLD_FLASH sticky. Recovery frames carry SOFT_OOB flag.

Proof: ./out/cubalc smx-soft-oob  → ok life_force>=0.35 mesh=stable
