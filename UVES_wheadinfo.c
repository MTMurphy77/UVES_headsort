/****************************************************************************
* Write some information about the list of fits headers to a file
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "UVES_headsort.h"
#include "file.h"
#include "error.h"

int UVES_wheadinfo(header *hdrs, int nhdrs, char *outfile) {

  double   temp=0.0,cwl=0.0;
  int      i=0;
  FILE     *out_file;

  /* Open output file */
  if ((out_file=faskwopen("Header info. output file?",outfile,4))==NULL)
    errormsg("UVES_wheadinfo(): Cannot open header info output\n\
\tfile %s for writing",outfile);

  /* Loop over headers */
  for (i=0; i<nhdrs; i++) {
    /* Identify which science arm we are using, red or blue and define temp */
    if (!strncmp(hdrs[i].cwl,"blue",4)) temp=hdrs[i].tb;
    else if (!strncmp(hdrs[i].cwl,"red",3)) temp=hdrs[i].tr;
    else {
      if (sscanf(hdrs[i].cwl,"%lf",&(cwl))!=1)
	errormsg("UVES_calsrch(): Incorrect format of central wavelength \n\
\tof frame\n\t%s",hdrs[i].file); 
      if (!hdrs[i].arm) temp=hdrs[i].tb;
      else temp=hdrs[i].tr;
    }
    fprintf(out_file,
	    "%-36s  %-15.15s  %-3.3s  %-4.4s  %-4.4s  %7.2lf  %dx%d  %4.1lf  %.8lf  \
%4.1lf  %5.1lf  %d\n",
	    hdrs[i].abfile,hdrs[i].obj,hdrs[i].typ,hdrs[i].mod,hdrs[i].cwl,
	    hdrs[i].et,hdrs[i].binx,hdrs[i].biny,hdrs[i].sw,hdrs[i].mjd,
	    temp,hdrs[i].p,hdrs[i].enc);
  }
  fclose(out_file);
  
  return 1;
}
