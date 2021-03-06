/****************************************************************************
* Search for calibration frames associated with each science frame and
* store relationship information
****************************************************************************/

#include <string.h>
#include <math.h>
#include "UVES_headsort.h"
#include "error.h"

int UVES_calsrch(header *hdrs, int nhdrs, scihdr *scis, int nscis,
		 calprd *cprd, int ncal) {

  int      ncsrch=0;      /* Number of calibrations over total cal. period */
  int      max_ncsrch=0;  /* Maximum # calibrations over total cal. period */
  int      i=0,j=0,k=0;
  calsrch  *csrch;        /* Array of calibration search structures */

  /* Determine size of calibration search array and allocate memory */
  max_ncsrch=NCALBLK*ncal;
  if (!(csrch=(calsrch *)malloc((size_t)(max_ncsrch*sizeof(calsrch)))))
    errormsg("UVES_calsrch(): Could not allocate memory for calibration\n\
\tsearch array of size %d.",max_ncsrch);

  /* Find all science frames and flesh-out relevant info (science arm etc.) */
  j=0; for (i=0; i<nhdrs; i++) {
    /* Identify science exposure */
    if (!strcmp(hdrs[i].typ,"sci")) {
      /* Copy header structure to science header */
      scis[j].hdr=hdrs[i];
      /* Identify which science arm we are using, red or blue */
      if (!scis[j].hdr.arm) strcpy(scis[j].arm,"blue\0");
      else strcpy(scis[j].arm,"red\0");
      /* Determine slit width string for naming of master flatfield in
	 MIDAS reduction script */
      sprintf(scis[j].swid,"s%2.2d",(int)(10.01*scis[j].hdr.sw));
      /* Determine science frame index number */
      scis[j].sciind=1;
      for (k=0; k<j; k++) {
	if (!strcmp(scis[k].hdr.obj,scis[j].hdr.obj) &&
	    !strcmp(scis[k].hdr.cwl,scis[j].hdr.cwl)) scis[j].sciind++;
      }
      /* Determine science frame index number for Versions <=0.31 */
      scis[j].sciind_31=1;
      for (k=0; k<j; k++) {
	if (!strcmp(scis[k].hdr.obj_31,scis[j].hdr.obj_31) &&
	    !strcmp(scis[k].hdr.cwl,scis[j].hdr.cwl)) scis[j].sciind_31++;
      }
      j++;
    }
  }

  for (i=0; i<nscis; i++) {

    /** BIAS **/
    /* Go though header list and determine bias calibration search array */
    for (j=0,k=0; j<nhdrs; j++) {
      if (!strcmp(hdrs[j].obj,"bias")) {
	if (hdrs[j].mjd>scis[i].hdr.mjd-cprd->ndscal_b &&
	    hdrs[j].mjd<scis[i].hdr.mjd+cprd->ndscal_f &&
	    !strcmp(hdrs[j].cwl,scis[i].arm) &&
	    hdrs[j].binx==scis[i].hdr.binx && hdrs[j].biny==scis[i].hdr.biny) {
	  csrch[k].ind=j;
	  if (hdrs[j].mjd_e<scis[i].hdr.mjd)
	    csrch[k].dmjd=scis[i].hdr.mjd-hdrs[j].mjd_e;
	  else if (hdrs[j].mjd>scis[i].hdr.mjd_e)
	    csrch[k].dmjd=hdrs[j].mjd-scis[i].hdr.mjd_e;
	  else {
	    warnmsg("UVES_calsrch(): BIAS frame\n\t%s,\n\
\twhich runs between MJD=%lf-%lf, appears to overlap with associated science frame\n\
\t%s\n\twhich runs between MJD=%lf-%lf.\n\
\tSetting time difference relative to middle of science frame.",hdrs[j].file,
		    hdrs[j].mjd,hdrs[j].mjd_e,scis[i].hdr.file,scis[i].hdr.mjd,
		    scis[i].hdr.mjd_e);
	    csrch[k].dmjd=fabs(0.5*(hdrs[j].mjd+hdrs[j].mjd_e)-
			       0.5*(scis[i].hdr.mjd+scis[i].hdr.mjd_e));
	  }
	  k++;
	  if (k==max_ncsrch)
	    errormsg("UVES_calsrch(): Maximum number of elements in cal. search\n\
\tarray exceeded. Increase NCALBLK in UVES_headsort.h");
	}
      }
    }
    if (!(ncsrch=k) && cprd->nbias>0) {
      warnmsg("UVES_calsrch(): No BIASes found in cal. period for\n\t%s.\n\
\tIncrease calibration period using -c option",scis[i].hdr.file);
      scis[i].nb=0; /* Indicates error for notes file writing */
    }
    else if (cprd->nbias>0) {
      if (ncsrch<cprd->nbias)
	warnmsg("UVES_calsrch(): %d BIASes requested but only %d found for\n\t%s",
		cprd->nbias,ncsrch,scis[i].hdr.file);
      /* Sort the cal. search array in order of increasing DMJD */
      qsort(csrch,ncsrch,sizeof(calsrch),qsort_calsrch);
      /* Fill the bias index array with relevant file numbers */
      scis[i].nb=MIN(ncsrch,cprd->nbias);
      for (j=0; j<scis[i].nb; j++) scis[i].bind[j]=csrch[j].ind;
    }
    else if (!cprd->nbias) scis[i].nb=0;

    /** FLAT **/
    /* Go though header list and determine flat calibration search array */
    for (j=0,k=0; j<nhdrs; j++) {
      if (!strcmp(hdrs[j].obj,"flat")) {
	if (hdrs[j].mjd>scis[i].hdr.mjd-cprd->ndscal_b &&
	    hdrs[j].mjd<scis[i].hdr.mjd+cprd->ndscal_f &&
	    !strcmp(hdrs[j].cwl,scis[i].hdr.cwl) &&
	    !strcmp(hdrs[j].mod,scis[i].hdr.mod) &&
	    hdrs[j].binx==scis[i].hdr.binx &&
	    hdrs[j].biny==scis[i].hdr.biny &&
	    hdrs[j].sw==scis[i].hdr.sw) {	    
	  csrch[k].ind=j;
	  if (hdrs[j].mjd_e<scis[i].hdr.mjd)
	    csrch[k].dmjd=scis[i].hdr.mjd-hdrs[j].mjd_e;
	  else if (hdrs[j].mjd>scis[i].hdr.mjd_e)
	    csrch[k].dmjd=hdrs[j].mjd-scis[i].hdr.mjd_e;
	  else {
	    warnmsg("UVES_calsrch(): FLAT frame\n\t%s,\n\
\twhich runs between MJD=%lf-%lf, appears to overlap with associated science frame\n\
\t%s\n\twhich runs between MJD=%lf-%lf.\n\
\tSetting time difference relative to middle of science frame.",hdrs[j].file,
		    hdrs[j].mjd,hdrs[j].mjd_e,scis[i].hdr.file,scis[i].hdr.mjd,
		    scis[i].hdr.mjd_e);
	    csrch[k].dmjd=fabs(0.5*(hdrs[j].mjd+hdrs[j].mjd_e)-
			  0.5*(scis[i].hdr.mjd+scis[i].hdr.mjd_e));
	  }
	  k++;
	  if (k==max_ncsrch)
	    errormsg("UVES_calsrch(): Maximum number of elements in cal. search\n\
\tarray exceeded. Increase NCALBLK in UVES_headsort.h");
	}
      }
    }
    if (!(ncsrch=k) && cprd->nflat>0) {
      warnmsg("UVES_calsrch(): No FLATs found in cal. period for\n\t%s.\n\
\tIncrease calibration period using -c option",scis[i].hdr.file);
      scis[i].nfl=0; /* Indicates error for notes file writing */
    }
    else if (cprd->nflat>0) {
      if (ncsrch<cprd->nflat)
	warnmsg("UVES_calsrch(): %d FLATs requested but only %d found for\n\t%s",
		cprd->nflat,ncsrch,scis[i].hdr.file);
      /* Sort the cal. search array in order of increasing DMJD */
      qsort(csrch,ncsrch,sizeof(calsrch),qsort_calsrch);
      /* Fill the flat index array with relevant file numbers */
      scis[i].nfl=MIN(ncsrch,cprd->nflat);
      for (j=0; j<scis[i].nfl; j++) scis[i].flind[j]=csrch[j].ind;
    }
    else if (!cprd->nflat) scis[i].nfl=0;

    /** WAV **/
    /* Go though header list and determine wav calibration search array */
    for (j=0,k=0; j<nhdrs; j++) {
      if (!strcmp(hdrs[j].typ,"wav")) {
	if (hdrs[j].mjd>scis[i].hdr.mjd-cprd->ndscal_b &&
	    hdrs[j].mjd<scis[i].hdr.mjd+cprd->ndscal_f &&
	    !strcmp(hdrs[j].cwl,scis[i].hdr.cwl) &&
	    !strcmp(hdrs[j].mod,scis[i].hdr.mod) &&
	    hdrs[j].binx==scis[i].hdr.binx &&
	    hdrs[j].biny==scis[i].hdr.biny &&
	    hdrs[j].sw==scis[i].hdr.sw) {
	  csrch[k].ind=j;
	  if (hdrs[j].mjd_e<scis[i].hdr.mjd)
	    csrch[k].dmjd=scis[i].hdr.mjd-hdrs[j].mjd_e;
	  else if (hdrs[j].mjd>scis[i].hdr.mjd_e)
	    csrch[k].dmjd=hdrs[j].mjd-scis[i].hdr.mjd_e;
	  else {
	    warnmsg("UVES_calsrch(): WAV frame\n\t%s,\n\
\twhich runs between MJD=%lf-%lf, appears to overlap with associated science frame\n\
\t%s\n\twhich runs between MJD=%lf-%lf.\n\
\ttSetting time difference relative to middle of science frame.",hdrs[j].file,
		    hdrs[j].mjd,hdrs[j].mjd_e,scis[i].hdr.file,scis[i].hdr.mjd,
		    scis[i].hdr.mjd_e);
	    csrch[k].dmjd=fabs(0.5*(hdrs[j].mjd+hdrs[j].mjd_e)-
			  0.5*(scis[i].hdr.mjd+scis[i].hdr.mjd_e));
	  }
	  k++;
	  if (k==max_ncsrch)
	    errormsg("UVES_calsrch(): Maximum number of elements in cal. search\n\
\tarray exceeded. Increase NCALBLK in UVES_headsort.h");
	}
      }
    }
    if (!(ncsrch=k) && cprd->nwav>0) {
      warnmsg("UVES_calsrch(): No WAVs found in cal. period for\n\t%s.\n\
\tIncrease calibration period using -c option",scis[i].hdr.file);
      scis[i].nw=0; /* Indicates error for notes file writing */
    }
    else if (cprd->nwav>0) {
      if (ncsrch<cprd->nwav)
	warnmsg("UVES_calsrch(): %d WAVs requested but only %d found for\n\t%s",
		cprd->nwav,ncsrch,scis[i].hdr.file);
      /* Sort the cal. search array in order of increasing DMJD */
      qsort(csrch,ncsrch,sizeof(calsrch),qsort_calsrch);
      scis[i].nw=MIN(ncsrch,cprd->nwav);
      /* Check whether there are any wavelength cals within the
	 attached calibration period which have the same encoder value
	 as the science exposures. If a cal was taken within 1/5th of
	 the (forward) attached cal period of the (calculated) end of
	 the science exposure, that is taken as the best calibration
	 file to use. Otherwise the attached cal closest in time to
	 the science exposure (within the attached cal period) is
	 selected. */
      if (ncsrch>1) {
	/* First check for att cal within 1/5th of att cal period after sci */
	j=0; k=-1; while (j<ncsrch && k==-1 && csrch[j].dmjd<0.2*cprd->ndsacal_f) {
	  if (scis[i].hdr.mjd_e<hdrs[csrch[j].ind].mjd &&
	      scis[i].hdr.enc==hdrs[csrch[j].ind].enc) k=j;
	  j++;
	}
	/* Now see if there's any within the full att cal period & select closest */
	if (k==-1) {
	  j=0; while (j<ncsrch && k==-1) {
	    if (scis[i].hdr.enc==hdrs[csrch[j].ind].enc &&
		((scis[i].hdr.mjd_e<hdrs[csrch[j].ind].mjd &&
		  csrch[j].dmjd<cprd->ndsacal_f) ||
		 (scis[i].hdr.mjd>hdrs[csrch[j].ind].mjd_e &&
		  csrch[j].dmjd<cprd->ndsacal_b))) k=j;
	    j++;
	  }
	}
	/* If either of the above checks identified a better cal than
	   just the closest one (in time) to the science exposure, put
	   it at the top of the list (and reorder the rest of the
	   list) */
	if (k>=0) {
	  scis[i].wind[0]=csrch[k].ind; j=1;
	} else j=0;
	/* Fill the wav index array with relevant file numbers */
	while (j<scis[i].nw) {
	  if (j!=k) scis[i].wind[j]=csrch[j].ind;
	  j++;
	}
      } else scis[i].wind[0]=csrch[0].ind;
    }
    else if (!cprd->nwav) scis[i].nw=0;

    /** ORD **/
    /* Go though header list and determine ord calibration search array */
    for (j=0,k=0; j<nhdrs; j++) {
      if (!strcmp(hdrs[j].typ,"ord")) {
	if (hdrs[j].mjd>scis[i].hdr.mjd-cprd->ndscal_b &&
	    hdrs[j].mjd<scis[i].hdr.mjd+cprd->ndscal_f &&
	    !strcmp(hdrs[j].cwl,scis[i].hdr.cwl) &&
	    !strcmp(hdrs[j].mod,scis[i].hdr.mod) &&
	    hdrs[j].binx==scis[i].hdr.binx &&
	    hdrs[j].biny==scis[i].hdr.biny) {
	  csrch[k].ind=j;
	  if (hdrs[j].mjd_e<scis[i].hdr.mjd)
	    csrch[k].dmjd=scis[i].hdr.mjd-hdrs[j].mjd_e;
	  else if (hdrs[j].mjd>scis[i].hdr.mjd_e)
	    csrch[k].dmjd=hdrs[j].mjd-scis[i].hdr.mjd_e;
	  else {
	    warnmsg("UVES_calsrch(): ORD frame\n\t%s,\n\
\twhich runs between MJD=%lf-%lf, appears to overlap with associated science frame\n\
\t%s\n\twhich runs between MJD=%lf-%lf.\n\
\tSetting time difference relative to middle of science frame.",hdrs[j].file,
		    hdrs[j].mjd,hdrs[j].mjd_e,scis[i].hdr.file,scis[i].hdr.mjd,
		    scis[i].hdr.mjd_e);
	    csrch[k].dmjd=fabs(0.5*(hdrs[j].mjd+hdrs[j].mjd_e)-
			  0.5*(scis[i].hdr.mjd+scis[i].hdr.mjd_e));
	  }
	  k++;
	  if (k==max_ncsrch)
	    errormsg("UVES_calsrch(): Maximum number of elements in cal. search\n\
\tarray exceeded. Increase NCALBLK in UVES_headsort.h");
	}
      }
    }
    if (!(ncsrch=k) && cprd->nord>0) {
      warnmsg("UVES_calsrch(): No ORDs found in cal. period for\n\t%s.\n\
\tIncrease calibration period using -c option",scis[i].hdr.file);
      scis[i].no=0; /* Indicates error for notes file writing */
    }
    else if (cprd->nord>0) {
      if (ncsrch<cprd->nord)
	warnmsg("UVES_calsrch(): %d ORDs requested but only %d found for\n\t%s",
		cprd->nord,ncsrch,scis[i].hdr.file);
      /* Sort the cal. search array in order of increasing DMJD */
      qsort(csrch,ncsrch,sizeof(calsrch),qsort_calsrch);
      /* Fill the order definition index array with relevant file numbers */
      scis[i].no=MIN(ncsrch,cprd->nord);
      for (j=0; j<scis[i].no; j++) scis[i].oind[j]=csrch[j].ind;
    }
    else if (!cprd->nord) scis[i].no=0;

    /** FMT **/
    /* Go though header list and determine fmt calibration search array */
    for (j=0,k=0; j<nhdrs; j++) {
      if (!strcmp(hdrs[j].typ,"fmt")) {
	if (hdrs[j].mjd>scis[i].hdr.mjd-cprd->ndscal_b &&
	    hdrs[j].mjd<scis[i].hdr.mjd+cprd->ndscal_f &&
	    !strcmp(hdrs[j].cwl,scis[i].hdr.cwl) &&
	    !strcmp(hdrs[j].mod,scis[i].hdr.mod) &&
	    hdrs[j].binx==scis[i].hdr.binx &&
	    hdrs[j].biny==scis[i].hdr.biny) {
	  csrch[k].ind=j;
	  if (hdrs[j].mjd_e<scis[i].hdr.mjd)
	    csrch[k].dmjd=scis[i].hdr.mjd-hdrs[j].mjd_e;
	  else if (hdrs[j].mjd>scis[i].hdr.mjd_e)
	    csrch[k].dmjd=hdrs[j].mjd-scis[i].hdr.mjd_e;
	  else {
	    warnmsg("UVES_calsrch(): FMT frame\n\t%s,\n\
\twhich runs between MJD=%lf-%lf, appears to overlap with associated science frame\n\
\t%s\n\twhich runs between MJD=%lf-%lf.\n\
\tSetting time difference relative to middle of science frame.",hdrs[j].file,
		    hdrs[j].mjd,hdrs[j].mjd_e,scis[i].hdr.file,scis[i].hdr.mjd,
		    scis[i].hdr.mjd_e);
	    csrch[k].dmjd=fabs(0.5*(hdrs[j].mjd+hdrs[j].mjd_e)-
			  0.5*(scis[i].hdr.mjd+scis[i].hdr.mjd_e));
	  }
	  k++;
	  if (k==max_ncsrch)
	    errormsg("UVES_calsrch(): Maximum number of elements in cal. search\n\
\tarray exceeded. Increase NCALBLK in UVES_headsort.h");
	}
      }
    }
    if (!(ncsrch=k) && cprd->nfmt>0) {
      warnmsg("UVES_calsrch(): No FMTs found in cal. period for\n\t%s.\n\
\tIncrease calibration period using -c option",scis[i].hdr.file);
      scis[i].nfm=0; /* Indicates error for notes file writing */
    }
    else if (cprd->nfmt>0) {
      if (ncsrch<cprd->nfmt)
	warnmsg("UVES_calsrch(): %d FMTs requested but only %d found for\n\t%s",
		cprd->nfmt,ncsrch,scis[i].hdr.file);
      /* Sort the cal. search array in order of increasing DMJD */
      qsort(csrch,ncsrch,sizeof(calsrch),qsort_calsrch);
      /* Fill the format check index array with relevant file numbers */
      scis[i].nfm=MIN(ncsrch,cprd->nfmt);
      for (j=0; j<scis[i].nfm; j++) scis[i].fmind[j]=csrch[j].ind;
    }
    else if (!cprd->nfmt) scis[i].nfm=0;

    /** STD **/
    /* Go though header list and determine std calibration search array */
    for (j=0,k=0; j<nhdrs; j++) {
      if (!strcmp(hdrs[j].typ,"std")) {
	if (hdrs[j].mjd>scis[i].hdr.mjd-cprd->ndscal_b &&
	    hdrs[j].mjd<scis[i].hdr.mjd+cprd->ndscal_f &&
	    !strcmp(hdrs[j].cwl,scis[i].hdr.cwl) &&
	    !strcmp(hdrs[j].mod,scis[i].hdr.mod) &&
	    hdrs[j].binx==scis[i].hdr.binx &&
	    hdrs[j].biny==scis[i].hdr.biny) {
	  csrch[k].ind=j;
	  csrch[k++].dmjd=fabs(hdrs[j].mjd-scis[i].hdr.mjd);
	  if (k==max_ncsrch)
	    errormsg("UVES_calsrch(): Maximum number of elements in cal. search\n\
\tarray exceeded. Increase NCALBLK in UVES_headsort.h");
	}
      }
    }
    if (!(ncsrch=k) && cprd->nstd>0) {
      warnmsg("UVES_calsrch(): No STDs found in cal. period for\n\t%s.\n\
\tIncrease calibration period using -c option",scis[i].hdr.file);
      scis[i].ns=0; /* Indicates error for notes file writing */
    }
    else if (cprd->nstd>0) {
      if (ncsrch<cprd->nstd)
	warnmsg("UVES_calsrch(): %d STDs requested but only %d found for\n\t%s",
		cprd->nstd,ncsrch,scis[i].hdr.file);
      /* Sort the cal. search array in order of increasing DMJD */
      qsort(csrch,ncsrch,sizeof(calsrch),qsort_calsrch);
      /* Fill the standard index array with relevant file numbers */
      scis[i].ns=MIN(ncsrch,cprd->nstd);
      for (j=0; j<scis[i].ns; j++) scis[i].sind[j]=csrch[j].ind;
      /* BUG: following code implies that only 1 standard per science
	 exposure is allowed */
      if (scis[i].ns) strcpy(scis[i].std,hdrs[scis[i].sind[0]].obj);
    }
    else if (!cprd->nstd) scis[i].ns=0;
  }

  /* Clean up */
  free(csrch);  

  return 1;

}
