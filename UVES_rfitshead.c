/****************************************************************************
* Read in relevant UVES FITS file header information only
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "UVES_headsort.h"
#include "const.h"
#include "error.h"

int UVES_rfitshead(char *infile, header *hdr) {

  double   cwl=0.0;
  int      hdutype=0,hdunum=0,status=0;
  char     comment[FLEN_COMMENT]="\0";
  char     *cptr=NULL;
  fitsfile *infits;

  /* Open input file as FITS file */
  if (fits_open_file(&infits,infile,READONLY,&status))
      errormsg("UVES_rfitshead(): Cannot open FITS file %s",infile);

  /* Check HDU type */
  fits_get_hdu_type(infits,&hdutype,&status);
  if (hdutype!=IMAGE_HDU) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): File not a FITS image: %s",infile);
  }

  /* Check number of HDUs */
  if (fits_get_num_hdus(infits,&hdunum,&status))
    errormsg("UVES_rfitshead(): Cannot find number of HDUs in file\n\
\t%s",infile);

  /* Check type of exposure */
  if (fits_read_key(infits,TSTRING,"HIERARCH ESO DPR TYPE",hdr->obj,
		    comment,&status)) {
    status=0;
    if (fits_read_key(infits,TSTRING,"OBJECT",hdr->obj,comment,&status)) {
      status=0;
      warnmsg("UVES_rfitshead(): Cannot read value of header cards\n\
\t%s or %s from FITS file\n\t%s\n\tAssuming that this is an OBJECT file.",
	      "HIERARCH ESO DPR TYPE","OBJECT",infile);
    }
  }
  if (strlen(hdr->obj)==0 ||
      (strstr(hdr->obj,"OBJECT")==NULL && strstr(hdr->obj,"SLIT")==NULL &&
       strstr(hdr->obj,"STD")==NULL && strstr(hdr->obj,"BIAS")==NULL &&
       strstr(hdr->obj,"FLAT")==NULL && strstr(hdr->obj,"LAMP")==NULL))
    sprintf(hdr->obj,"%s","OBJECT");
  if (fits_read_key(infits,TSTRING,"HIERARCH ESO DPR CATG",hdr->typ,
		    comment,&status)) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO DPR CATG",infile);
  }

  if (fits_read_key(infits,TSTRING,"ARCFILE",hdr->dat,comment,&status)) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","ARCFILE",infile);
  }

  if (fits_read_key(infits,TDOUBLE,"EXPTIME",&(hdr->et),comment,&status)) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","EXPTIME",infile);
  }

  if (fits_read_key(infits,TDOUBLE,"MJD-OBS",&(hdr->mjd),comment,&status)) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","MJD-OBS",infile);
  }

  if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO DET EXP RDTTIME",&(hdr->rt),comment,
		    &status)) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO DET EXP RDTTIME",infile);
  }

  if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO DET EXP XFERTIM",&(hdr->tt),comment,
		    &status)) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO DET EXP XFERTIM",infile);
  }

  /* Define time of end of exposure+read-out/transfer */
  hdr->mjd_e=hdr->mjd+(hdr->et+(MAX(hdr->rt,hdr->tt)))/C_SECDAY;

  if (fits_read_key(infits,TINT,"HIERARCH ESO DET WIN1 BINX",&(hdr->biny),
		    comment,&status)) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO DET WIN1 BINX",infile);
  }

  if (fits_read_key(infits,TINT,"HIERARCH ESO DET WIN1 BINY",&(hdr->binx),
		    comment,&status)) {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO DET WIN1 BINY",infile);
  }

  if (strstr(hdr->obj,"OBJECT")!=NULL || strstr(hdr->obj,"SLIT")!=NULL ||
      strstr(hdr->obj,"STD")!=NULL) {
    if (fits_read_key(infits,TSTRING,"HIERARCH ESO OBS TARG NAME",hdr->obj,
		      comment,&status)) {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO OBS TARG NAME",infile);
    }
    if (!strcmp(hdr->typ,"SCIENCE")) strcpy(hdr->typ,"sci\0");
    else if (!strcmp(hdr->typ,"CALIB") || !strcmp(hdr->typ,"STD"))
      strcpy(hdr->typ,"std\0");
    else if (!strcmp(hdr->typ,"TEST")) strcpy(hdr->typ,"tst\0");
    else if (!strcmp(hdr->typ,"ACQUISITION")) strcpy(hdr->typ,"aqu\0");
    else {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Do not understand header card %s in file\n\t%s",
	       "HIERARCH ESO DPR CATG",infile);
    }
    /* Alter object name to remove some special characters */
    while ((cptr=strchr(hdr->obj,'+'))!=NULL) *cptr='p';
    while ((cptr=strchr(hdr->obj,'-'))!=NULL) *cptr='m';
    while ((cptr=strchr(hdr->obj,'_'))!=NULL) *cptr='u';
    while ((cptr=strchr(hdr->obj,'$'))!=NULL) *cptr='d';
    while ((cptr=strchr(hdr->obj,' '))!=NULL) *cptr='s';
    /* Version 0.40: To be compatible with (stupid) Macs, all object
       names are now converted to lower case only. Variables with
       suffix "_31" are those which would have been allocated in
       versions <=0.31 */
    sprintf(hdr->obj_31,"%s",hdr->obj); strlower(hdr->obj);
  }
  else if (strstr(hdr->obj,"BIAS")!=NULL) {
    strcpy(hdr->obj,"bias\0"); strcpy(hdr->typ,"cal\0");
  }
  else if (strstr(hdr->obj,"FLAT")!=NULL) {
    strcpy(hdr->obj,"flat\0"); strcpy(hdr->typ,"cal\0");
  }
  else if (strstr(hdr->obj,"LAMP")!=NULL) {
    if (strstr(hdr->obj,"WAV")!=NULL) strcpy(hdr->typ,"wav\0");
    else if (strstr(hdr->obj,"ORD")!=NULL) strcpy(hdr->typ,"ord\0");
    else if (strstr(hdr->obj,"FMT")!=NULL) strcpy(hdr->typ,"fmt\0");
    else {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Do not understand LAMP-type\n\
\theader card %s in file\n\t%s","HIERARCH ESO DPR TYPE",infile);
    }
    strcpy(hdr->obj,"thar\0");
  }
  else {
    status=0; fits_close_file(infits,&status);
    errormsg("UVES_rfitshead(): Do not understand header card %s in file\n\t%s",
	     "HIERARCH ESO DPR TYPE",infile);
  }

  if (!strcmp(hdr->obj,"bias")) {
    /* Currently there are only rather dodgy ways of figuring out which
       science arm we are using in a bias frame. God knows what will happen
       if they ever add another CCD to the blue end ...
    */
    /* Current method: Check the names of CHIP1 in the header. If this
       contains the string "MIT" then we're dealing with the red
       arm */
    if (fits_read_key(infits,TSTRING,"HIERARCH ESO DET CHIP1 NAME",hdr->cwl,
		      comment,&status)) {
      status=0;
      if (fits_read_key(infits,TSTRING,"ORIGFILE",hdr->cwl,comment,&status)) {
	status=0; fits_close_file(infits,&status);
	errormsg("UVES_rfitshead(): Cannot read value of header cards\n\
\t%s or %s from FITS file\n\t%s.","HIERARCH ESO DET CHIP1 NAME","ORIGFILE",infile);
      }
      if (strstr(hdr->cwl,"RED")!=NULL || strstr(hdr->cwl,"red")!=NULL ||
	  strstr(hdr->cwl,"Red")!=NULL) {
	strcpy(hdr->cwl,"red\0"); hdr->arm=1;
      } else if (strstr(hdr->cwl,"BLUE")!=NULL || strstr(hdr->cwl,"blue")!=NULL ||
		 strstr(hdr->cwl,"Blue")!=NULL) {
	strcpy(hdr->cwl,"blue\0"); hdr->arm=0;
      } else {
	/* If nothing is in the first HDU, move to the next HDU if it exists */
	if (hdunum>1) {
	  /* Move to next HDU */
	  if (fits_movrel_hdu(infits,1,&hdutype,&status))
	    errormsg("UVES_rfitshead(): Could not move to second HDU\n\
\tin file\n\t%s",infile);
	  /* Check HDU type */
	  if (hdutype!=IMAGE_HDU)
	    errormsg("UVES_rfitshead(): Second extension %d not a FITS image\n\
\tin file\n\t%s",infile);
	  if (fits_read_key(infits,TSTRING,"HIERARCH ESO DET CHIP1 NAME",hdr->cwl,
			    comment,&status))
	    errormsg("UVES_rfitshead(): Cannot read value of header cards\n\
\t%s from FITS file\n\t%s.","HIERARCH ESO DET CHIP1 NAME",infile);
	  if (strstr(hdr->cwl,"MIT")!=NULL) strcpy(hdr->cwl,"red\0");
	  else strcpy(hdr->cwl,"blue\0");
	} else
	  errormsg("UVES_rfitshead(): Cannot read value of header cards\n\
\t%s or %s from FITS file\n\t%s\n\tand there is only one HDU present.\n\
\tI therefore cannot determine whether exposure is in blue or red arm",
		   "HIERARCH ESO DET CHIP1 NAME","ORIGFILE",infile);
      }
    } else {
      if (strstr(hdr->cwl,"MIT")!=NULL) { strcpy(hdr->cwl,"red\0"); hdr->arm=1; }
      else { strcpy(hdr->cwl,"blue\0"); hdr->arm=0; }
    }
    /* Set several parameters not relevant to biases to zero */
     hdr->sw=0.0;
  } else {
    /* Find the temperature in each arm and the atmospheric pressure */
    if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS TEMP1 MEAN",&(hdr->tb),
		      comment,&status)) {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS TEMP1 MEAN",infile);
    }
    if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS TEMP2 MEAN",&(hdr->tr),
		      comment,&status)) {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS TEMP2 MEAN",infile);
    }
    if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS SENS26 MEAN",&(hdr->p),
		      comment,&status)) {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS SENS26 MEAN",infile);
    }

    /* Decide which arm we're using */
    if (fits_read_key(infits,TSTRING,"HIERARCH ESO INS PATH",hdr->cwl,
		      comment,&status)) {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS PATH",infile);
    }
    if (strstr(hdr->cwl,"RED")!=NULL || strstr(hdr->cwl,"red")!=NULL) {
      hdr->arm=1;
      if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS GRAT2 WLEN",&cwl,
			comment,&status)) {
	status=0; fits_close_file(infits,&status);
	errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS GRAT2 WLEN",infile);      
      }
    }
    else {
      hdr->arm=0;
      if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS GRAT1 WLEN",&cwl,
			comment,&status)) {
	status=0; fits_close_file(infits,&status);
	errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS GRAT1 WLEN",infile);      
      }
    }
    /* To cope with pathalogical cases where very very red settings in
       the blue arm were used, make sure al blue arm exposures are
       labelled as having central wavelength BORR-1 or less and all
       red arm exposures are labelled with BORR or greater. */
    if (!hdr->arm) cwl=(MIN((BORR-1.0),cwl));
    else cwl=(MAX(BORR,cwl));
    /* Imprint central wavelength label */
    sprintf(hdr->cwl,"%.0lf",cwl);

    /* Find the slit width used and encoder value for the relevant grating */
    if (!hdr->arm) {
      if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS SLIT2 WID",&(hdr->sw),
			comment,&status)) {
	status=0; fits_close_file(infits,&status);
	errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS SLIT2 WID",infile);
      }
      if (fits_read_key(infits,TINT,"HIERARCH ESO INS GRAT1 ENC",&(hdr->enc),
			comment,&status)) {
	status=0; fits_close_file(infits,&status);
	errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS GRAT1 ENC",infile);
      }
    }
    else {
      if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS SLIT3 WID",&(hdr->sw),
			comment,&status)) {
	status=0; fits_close_file(infits,&status);
	errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS SLIT3 WID",infile);
      }
      if (fits_read_key(infits,TINT,"HIERARCH ESO INS GRAT2 ENC",&(hdr->enc),
			comment,&status)) {
	status=0; fits_close_file(infits,&status);
	errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS GRAT2 ENC",infile);
      }
    }
  }

  /* Make sure we deal with dichroic and non-dichroic settings */
  if (!strcmp(hdr->obj,"bias")) strcpy(hdr->mod,"NA\0");
  else {
    if (fits_read_key(infits,TSTRING,"HIERARCH ESO INS MODE",hdr->mod,
		      comment,&status)) {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Cannot read value of header card %s\n\
\tfrom FITS file %s.","HIERARCH ESO INS MODE",infile);
    }
    if (strstr(hdr->mod,"DIC")!=NULL) strcpy(hdr->mod,"dic\0");
    else if (strstr(hdr->mod,"BLUE")!=NULL) strcpy(hdr->mod,"blue\0");
    else if (strstr(hdr->mod,"RED")!=NULL) strcpy(hdr->mod,"red\0");
    else {
      status=0; fits_close_file(infits,&status);
      errormsg("UVES_rfitshead(): Do not understand header card %s in file\n\t%s",
	       "HIERARCH ESO INS MODE",infile);
    }
  }

  /* Close input FITS file */
  fits_close_file(infits,&status);
  
  return 1;
}
