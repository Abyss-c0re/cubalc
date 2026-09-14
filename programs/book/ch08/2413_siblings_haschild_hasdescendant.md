# Ch08 — MI 2413 SIBLINGS / HASCHILD / HASDESCENDANT

Multi-file CLASS EXTEND usability probes for peer and progeny walks.

- SIBLINGS TagGreeter → TwinGreet (same parent Greeter)
- HASCHILD Greeter TagGreeter → 1; HASCHILD Greeter LoudTag → 0 (not direct)
- HASDESCENDANT Greeter LoudTag → 1
- CHILDREN Greeter → TagGreeter, TwinGreet
- Cross-check: COMMONANCESTOR / CHAINDIST / PATHBETWEEN on LoudTag↔SoftTag

Proof: `programs/proof/2413_multifile_extend_siblings_haschild_hasdescendant_link3.cubalc` (65/0 PASS).
Cube is SoT. Free energy must flow.
