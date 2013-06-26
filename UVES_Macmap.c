/****************************************************************************
* Create a file which specifies the mapping between object names and
* file indices in Versions <=0.31 and >=0.40.
****************************************************************************/

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "UVES_headsort.h"
#include "file.h"
#include "error.h"

int UVES_Macmap(header *hdrs, int nhdrs, scihdr *scis, int nscis) {

  int    first=0;
  int    i=0,j=0;
  char   listfile[NAMELEN]="\0";
  FILE   *list_file;

  /* Loop over science exposures */
  for (i=0; i<nscis; i++) {
    /* See if this is the first time this object has been encountered */
    first=1;
    j=0; while (first && j<i)
      if (!strcmp(scis[j++].hdr.obj,scis[i].hdr.obj)) first=0;

    /* Is this the first time this object has been encountered */
    if (first) {
      /* Define Macmap file name and open it for writing */
      sprintf(listfile,"%s.macmap",scis[i].hdr.obj);
      if ((list_file=faskwopen("Macmap file for new object?",listfile,4))==NULL)
	errormsg("UVES_Macmap(): Cannot open Macmap file for\n\
\tobject %s for writing",scis[i].hdr.obj);

      /* Loop over all science exposures of this object */
      for (j=i; j<nscis; j++) {
	if (!strcmp(scis[i].hdr.obj,scis[j].hdr.obj))
	  fprintf(list_file,"%-20s %3s %02d  %-20s %3s %02d\n",
		  scis[j].hdr.obj_31,scis[j].hdr.cwl,scis[j].sciind_31,
		  scis[j].hdr.obj,scis[j].hdr.cwl,scis[j].sciind);
      }

      /* Close list file */
      fclose(list_file);
    }
  }

  return 1;

}
