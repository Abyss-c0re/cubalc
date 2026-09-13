feature LIFE_CELL_SACM1L version 1.15.1846-usability PASS prior OSBP
# MEANINGFUL_ITER 2345

feature LIFE_CELL_OSBP version 1.15.1845-usability PASS prior STARD3
- **2347** LIFE_CELL_CERT — METHOD  after sacm1l; proofs sum 656 fail 0
- **2348** LIFE_CELL_SMS1 — METHOD sms1 n after cert; proofs sum 661 fail 0
- 2348: LIFE_CELL_SMS1 — SMS1/SGMS1 Golgi sphingomyelin synthase after CERT on sterol continuum
- **2349** `LIFE_CELL_SMS2` — SMS2/SGMS2 plasma-membrane sphingomyelin synthase after SMS1 on sterol continuum (`sms2 n`); proofs 666/0
- MI 2349 LIFE_CELL_SMS2: METHOD sms2 n after sms1 — PASS sum 666

## MI 2351 LIFE_CELL_B4GALT

B4GALT5/6 LacCer synthase after UGCG on glycosphingolipid continuum.
Proofs sum 676 fail 0.
B4GALT commits GlcCer into Golgi LacCer under ASSERT after UGCG GlcCer ward so free energy keeps moving into neutral glycosphingolipid pools without hollow monoglycosylceramide stall.

## MI 2352 — LIFE_CELL_ST3GAL5
- method: `st3gal5 n`
- continuum: `...->ugcg->b4galt->st3gal5`
- proof: `programs/proof/2352_life_cell_st3gal5.cubalc`
