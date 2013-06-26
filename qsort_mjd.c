/****************************************************************************
* Qsort routine to sort headers in order of increasing MJD
****************************************************************************/

#include "UVES_headsort.h"

int qsort_mjd(const void *hdr1, const void *hdr2) {

  if (((header *)hdr1)->mjd > ((header *)hdr2)->mjd) return 1;
  else if (((header *)hdr1)->mjd == ((header *)hdr2)->mjd) return 0;
  else return -1;

}
