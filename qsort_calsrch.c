/****************************************************************************
* Qsort routine to sort the elements of a calibration search array by the
* MJD difference between the science and cal. frames
****************************************************************************/

#include "UVES_headsort.h"

int qsort_calsrch(const void *csrch1, const void *csrch2) {

  if (((calsrch *)csrch1)->dmjd > ((calsrch *)csrch2)->dmjd) return 1;
  else if (((calsrch *)csrch1)->dmjd == ((calsrch *)csrch2)->dmjd) return 0;
  else return -1;

}
