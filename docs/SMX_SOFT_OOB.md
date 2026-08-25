# SMX soft-OOB mesh stability

Soft-OOB heal path keeps mesh life-force above floor under replay/compat stress.
Fail-closed without key. HOLD_FLASH sticky. Recovery frames carry SOFT_OOB flag.

`cubalc_smx_mesh_exchange` seals A→B, opens on B, soft-heals recoverable
replay/compat stress, then applies proton matrix transfer under life-force floor.

Proof: `./out/cubalc smx-soft-oob` → ok life_force>=0.35 mesh=stable mesh_exchange=ok
