/****************************************************************************
* Create list of relevant files for each science object in input list
****************************************************************************/

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "UVES_headsort.h"
#include "memory.h"
#include "file.h"
#include "error.h"

int UVES_list(header *hdrs, int nhdrs, scihdr *scis, int nscis) {

  int    first=0,nidx=0;
  int    i=0,j=0,k=0,l=0;
  int    *idx=NULL;
  char   listfile[NAMELEN]="\0";
  FILE   *list_file;

  /* Allocate memory for index recording array */
  if ((idx=iarray(nhdrs))==NULL)
    errormsg("UVES_list(): Cannot allocate memory for idx\n\tarray of size %d",
	     nhdrs);

  /* Loop over science exposures */
  for (i=0; i<nscis; i++) {
    /* See if this is the first time this object has been encountered */
    first=1;
    j=0; while (first && j<i)
      if (!strcmp(scis[j++].hdr.obj,scis[i].hdr.obj)) first=0;

    /* Is this the first time this object has been encountered */
    if (first) {
      /* Define list file name and open it for writing */
      sprintf(listfile,"%s.list",scis[i].hdr.obj);
      if ((list_file=faskwopen("File list for new object?",listfile,4))==NULL)
	errormsg("UVES_list(): Cannot open file list for\n\
\tobject %s for writing",scis[i].hdr.obj);

      /* Initialise index counter for this object */
      nidx=0;

      /* Loop over all science exposures of this object */
      for (j=i; j<nscis; j++) { if (!strcmp(scis[i].hdr.obj,scis[j].hdr.obj)) {
	fprintf(list_file,"%s\n",scis[j].hdr.file);
	for (k=0; k<scis[j].ns; k++) {
	  for (l=0; l<nidx; l++) if (idx[l]==scis[j].sind[k]) break;
	  if (l==nidx) {
	    idx[nidx++]=scis[j].sind[k];
	    fprintf(list_file,"%s\n",hdrs[scis[j].sind[k]].file);
	  }
	}
	for (k=0; k<scis[j].nw; k++) {
	  for (l=0; l<nidx; l++) if (idx[l]==scis[j].wind[k]) break;
	  if (l==nidx) {
	    idx[nidx++]=scis[j].wind[k];
	    fprintf(list_file,"%s\n",hdrs[scis[j].wind[k]].file);
	  }
	}
	for (k=0; k<scis[j].no; k++) {
	  for (l=0; l<nidx; l++) if (idx[l]==scis[j].oind[k]) break;
	  if (l==nidx) {
	    idx[nidx++]=scis[j].oind[k];
	    fprintf(list_file,"%s\n",hdrs[scis[j].oind[k]].file);
	  }
	}
	for (k=0; k<scis[j].nfm; k++) {
	  for (l=0; l<nidx; l++) if (idx[l]==scis[j].fmind[k]) break;
	  if (l==nidx) {
	    idx[nidx++]=scis[j].fmind[k];
	    fprintf(list_file,"%s\n",hdrs[scis[j].fmind[k]].file);
	  }
	}
	for (k=0; k<scis[j].nfl; k++) {
	  for (l=0; l<nidx; l++) if (idx[l]==scis[j].flind[k]) break;
	  if (l==nidx) {
	    idx[nidx++]=scis[j].flind[k];
	    fprintf(list_file,"%s\n",hdrs[scis[j].flind[k]].file);
	  }
	}
	for (k=0; k<scis[j].nb; k++) {
	  for (l=0; l<nidx; l++) if (idx[l]==scis[j].bind[k]) break;
	  if (l==nidx) {
	    idx[nidx++]=scis[j].bind[k];
	    fprintf(list_file,"%s\n",hdrs[scis[j].bind[k]].file);
	  }
	}
      } }

      /* Close list file */
      fclose(list_file);
    }
  }

  /* Clean up */
  free(idx);

  return 1;

}
