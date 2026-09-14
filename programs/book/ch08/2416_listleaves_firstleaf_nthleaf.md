# MI 2416 — LISTLEAVES / FIRSTLEAF / LASTLEAF / NTHLEAF

Multi-file CLASS EXTEND leaf-bag probes.

## Forms
- `LISTLEAVES Class|obj` — newline bag of leaves in subtree (self if leaf)
- `FIRSTLEAF` / `LASTLEAF` — ends of that bag
- `NTHLEAF Class|obj N` — 0-based leaf; soft empty OOB

## Complements
LEAFCOUNT, ISLEAF, DESCENDANTS, NTHCHILD/FIRSTCHILD/LASTCHILD.

## Proof
`programs/proof/2416_multifile_extend_listleaves_firstleaf_nthleaf_link3.cubalc` — asserts_ok=110 fail=0.

VERSION `1.16.17-listleaves-firstleaf-nthleaf`.
