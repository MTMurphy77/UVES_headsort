/****************************************************************************
* Initialize (zero) parameters set
****************************************************************************/

#include "UVES_headsort.h"

int UVES_params_init(calprd *cprd) {

  cprd->nhrscal_f=cprd->nhrscal_b=-1.0;
  cprd->nbias=cprd->nflat=cprd->nwav=cprd->nord=cprd->nfmt=cprd->nstd=-1;

  return 1;

}
