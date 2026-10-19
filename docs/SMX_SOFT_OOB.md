# SMX soft-OOB mesh stability

Soft-OOB heal path keeps mesh life-force above floor under replay/compat stress.
Fail-closed without key. HOLD_FLASH sticky. Recovery frames carry SOFT_OOB flag.

cubalc_smx_mesh_exchange seals A to B, opens on B, soft-heals recoverable
replay/compat stress, then applies proton matrix transfer under life-force floor.

Proofs:
- ./out/cubalc smx-soft-oob -> ok life_force>=0.35 mesh=stable mesh_exchange=ok
- ./out/cubalc smx-mesh-exchange -> ok mesh=stable fail_closed=true

MEANINGFUL_ITER 1156 · VERSION 1.5.4-smx-mesh-exchange

stable_exchanges counts successful mesh_exchange completions (life-force restore on each).
MEANINGFUL_ITER 1157 · VERSION 1.5.4-smx-mesh-exchange
