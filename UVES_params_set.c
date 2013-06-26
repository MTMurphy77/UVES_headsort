/****************************************************************************
* Set unset parameters to default values
****************************************************************************/

#include "UVES_headsort.h"

int UVES_params_set(calprd *cprd) {

  if (cprd->nhrscal_f==-1.0) cprd->nhrscal_f=NHRSCAL_F;
  if (cprd->nhrscal_b==-1.0) cprd->nhrscal_b=NHRSCAL_B;
  if (cprd->nbias==-1) cprd->nbias=NBIAS;
  if (cprd->nflat==-1) cprd->nflat=NFLAT;
  if (cprd->nwav==-1) cprd->nwav=NWAV;
  if (cprd->nord==-1) cprd->nord=NORD;
  if (cprd->nfmt==-1) cprd->nfmt=NFMT;
  if (cprd->nstd==-1) cprd->nstd=NSTD;

  return 1;

}
