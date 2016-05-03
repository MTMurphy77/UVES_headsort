/****************************************************************************

UVES_wavres: Read in the wavelength calibration results from the CPL
uves_cal_wavecal command and display some relevant information

****************************************************************************/

/* Include files */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fitsio.h>
#include <longnam.h>
#include "stats.h"
#include "charstr.h"
#include "file.h"
#include "memory.h"
#include "const.h"
#include "error.h"

/* Definitions */
#define VERB 2
#define TOLM 0
#define INCLOSE status=0; fits_close_file(infits,&status);
#define TOLMOUT(ERR,TOL) fprintf(stdout,"%.0lf %1d %1d %6.3lf %4d %6.2lf %9.4lf \
%9.4lf %1d %6.3lf\n",ts.cwl,ts.binx,ts.biny,ts.tol,ts.np,(double)ts.np/(double)ts.no,\
				 ts.mrmso,ts.mrms,ERR,TOL);
#define FREETS free(ts.x); free(ts.h); free(ts.c); free(ts.s); free(ts.w); \
               free(ts.dis); free(ts.wlf); free(ts.wlc); free(ts.res); \
               free(ts.o_rms); free(ts.ora); free(ts.orr); free(ts.sts); \
               free(ts.stp); free(ts.o_id); free(ts.o_n); free(ts.o_np);
#define FREETOL free(tol); free(nav); free(rms);

/* Structures */
typedef struct ThArSet {
  double cwl;           /* Nominal central wavelength of setting */
  double mrms;          /* Mean RMS residual */
  double mrmso;         /* Mean RMS residual per order */
  double tol;           /* Tolerance parameter used in CPL reduction */
  double *x;            /* Fitted pixel position */
  double *h;            /* Fitted line height above continuum */
  double *c;            /* Fitted continuum height */
  double *s;            /* Fitted continuum slope */
  double *w;            /* Fitted FWHM width of line [A] */
  double *dis;          /* Dispersion [A/pix] */
  double *wlf;          /* Wavelength from fit [A] */
  double *wlc;          /* Wavelength from catalogue [A] */
  double *res;          /* Residuals: wlf-wlc/wlc [m/s] */
  double *o_rms;        /* RMS residuals in each order [m/s] */
  int    chip;          /* Which CCD chip was used?: 0=blue; 1=redl; 2=redu */
  int    deg;           /* Wavelength polynomial degree used in CPL reduction */
  int    binx;          /* CCD Binning in spectral direction */
  int    biny;          /* CCD Binning in spatial direction */
  int    n;             /* Number of lines in set */
  int    ns;            /* Number of lines selected */
  int    np;            /* Number of lines used in polynomial solution */
  int    no;            /* Number of orders processed */
  int    nop;           /* Number of orders with lines used in polynomial solution */
  int    o_ids;         /* Starting diffraction order */
  int    o_ide;         /* Ending diffraction order */
  int    o_slp;         /* Slope of diffraction order change with increasing index */
  int    *ora;          /* Absolute order number */
  int    *orr;          /* Relative order number */
  int    *sts;          /* Status as selected line */
  int    *stp;          /* Status as line used in polynomial fit */
  int    *o_id;         /* Diffraction order numbers */
  int    *o_n;          /* Number of lines in each order */
  int    *o_np;         /* Number of lines in each order used in polynomial solution */
} tharset;

/* Global declarations */
char      *progname;

/****************************************************************************
* Print the usage message
****************************************************************************/

void usage(void) {

  fprintf(stderr,"\n%s: Read in the wavelength calibration results from the CPL\n\
\tuves_cal_wavecal command and display some relevant information\n",progname);

  fprintf(stderr,"\nBy Michael Murphy");

  fprintf(stderr,"\nUsage: %s [OPTIONS] [INPUT FITS FILE]\n",progname);

  fprintf(stderr, "\nOptions:\n\
  -h, -help        : Print this message.\n\
  -tolm = %1d        : Tolerance-finding mode (0=none, 1=first iteration, 2=second\n\
                         3=check).\n\
  -verb = %1d        : Verbosity level (2=all, 1=some, 0=minimal).\n\n",
	  TOLM,VERB);

  exit(3);
}


/****************************************************************************

Calculate the average number of lines used in the polynomial
solution and the resulting RMS when the residuals are restricted to
lie within a given tolerance

****************************************************************************/

int UVES_tolstat(tharset *ts, double tol, double *nav, double *rms) {

  int     *sts=NULL;
  int     n=0;
  int     i=0;
  statset stat;

  /* Make sure input tolerance is sensible */
  if (tol<=0.0) {
    nferrormsg("UVES_tolstat(): Tolerance value entered (=%lf)\n\
is invalid",tol); return 0;
  }

  /* Allocate a new status array for those lines with the right residuals */
  if ((sts=iarray(ts->n))==NULL) {
    nferrormsg("UVES_tolstat(): Cannot allocate memory for sts\n\
\tarray of length %d",ts->n); return 0;
  }
  
  /* Fill new status array */
  for (i=0,n=0; i<ts->n; i++) {
    sts[i]=0;
    if (ts->stp[i] && fabs(ts->wlf[i]-ts->wlc[i])/ts->dis[i]<=tol) {
      sts[i]=1; n++;
    }
  }

  /* If no lines are found then exit */
  if (n==0) {
    nferrormsg("UVES_tolstat(): No lines can be found with residuals\n\
\tless than or equal to tolerance entered (=%lf)",tol); return 0;
  }

  /* Calculate statistics */
  if (!stats(ts->res,NULL,NULL,NULL,sts,ts->n,0,&stat)) {
    nferrormsg("UVES_tolstat(): Error returned from stats()"); return 0;
  }

  /* Set results values */
  *nav=(double)n/(double)ts->no;
  *rms=stat.rms;

  return 1;

}

/****************************************************************************
* The main program

  Things to do:

****************************************************************************/

int main(int argc, char *argv[]) {

  double   nulval=0.0;
  double   nmin=0.0,RMStarg=0.0,tolmin=0.010,tolstep=0.005,rmsdiff=0.0,navdiff=0.0;
  double   *tol=NULL,*nav=NULL,*rms=NULL;
  long     nrows=0;
  int      verb=-1,tolm=-1;
  int      ntol=0;
  int      hdunum=0,hdutype=0,status=0,naxis=0,anynul=0,col=0;
  int      i=0,j=0,k=0,l=0;
  char     infile[NAMELEN]="\0";
  char     key[FLEN_KEYWORD]="\0",dummy[FLEN_KEYWORD]="\0";
  char     card[FLEN_CARD]="\0",comment[FLEN_COMMENT]="\0";
  char     *inclist[1],search[FLEN_CARD]="HISTORY\0";
  fitsfile *infits;
  tharset  ts;
  statset  stat;

  /* Define the program name from the command line input */
  progname=((progname=strrchr(argv[0],'/'))==NULL) ? argv[0] : progname+1;
  /* Must be at least one argument */
  if (argc<1) usage();
  /* Scan command line for options */
  while (++i<argc) {
    if (!strcmp(argv[i],"-tolm")) {
      if (sscanf(argv[++i],"%d",&(tolm))!=1) usage();
    }
    if (!strcmp(argv[i],"-verb")) {
      if (sscanf(argv[++i],"%d",&(verb))!=1) usage();
    }
    else if (!strcmp(argv[i],"-help") || !strcmp(argv[i],"-h")) usage();
    else if (!strncmp(infile,"\0",1)) {
      if (!access(argv[i],R_OK)) {
	if (strlen(argv[i])<=NAMELEN) strcpy(infile,argv[i]);
	else errormsg("Input file name too long: %s",argv[i]);
      }
      else errormsg("Input file %s does not exist",argv[i]);
    }
  }
  /* Make sure input file was specified */
  if (!strncmp(infile,"\0",1)) usage();
  /* Make sure tolerance-finding mode makes sense */
  if (tolm<0) tolm=TOLM;
  /* Make sure verbosity level makes sense */
  if (verb<0) verb=VERB;

  /* Open input file as FITS file */
  if (fits_open_file(&infits,infile,READONLY,&status))
      errormsg("Cannot open input FITS file %s",infile);
  /* Check number of HDUs */
  if (fits_get_num_hdus(infits,&hdunum,&status)) {
    INCLOSE; errormsg("Cannot find number of HDUs in file %s",infile);
  }
  if (hdunum!=10) {
    INCLOSE;
    errormsg("Number of HDUs is %d instead of %d in file\n\t%s",hdunum,10,infile);
  }
  /* Determine nominal central wavelength */
  if (fits_read_key(infits,TSTRING,"HIERARCH ESO INS PATH",card,comment,&status)) {
    INCLOSE; errormsg("Cannot read value of header card %s from file\n\t%s",
		      "HIERARCH ESO INS PATH",infile);
  }
  if (strstr(card,"RED")!=NULL || strstr(card,"red")!=NULL) {
    if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS GRAT2 WLEN",&ts.cwl,comment,
		      &status)) {
      INCLOSE; errormsg("Cannot read value of header card %s from file\n\t%s.",
			"HIERARCH ESO INS GRAT2 WLEN",infile);      
    }
  } else {
    if (fits_read_key(infits,TDOUBLE,"HIERARCH ESO INS GRAT1 WLEN",&ts.cwl,comment,
		      &status)) {
      INCLOSE; errormsg("Cannot read value of header card %s from file\n\t%s.",
			"HIERARCH ESO INS GRAT1 WLEN",infile);      
    }
  }
  /* Read which chip we're using directly from the OBJECT descriptor */
  if (fits_read_key(infits,TSTRING,"OBJECT",card,comment,&status)) {
    INCLOSE; errormsg("Cannot read value of header card %s from file\n\t%s",
		      "OBJECT",infile);
  }
  if (strstr(card,"BLUE") || strstr(card,"blue")) ts.chip=0;
  else if (strstr(card,"REDL") || strstr(card,"redl")) ts.chip=1;
  else if (strstr(card,"REDU") || strstr(card,"redu")) ts.chip=2;
  else {
    /* v0.55: ESO changed the (silly) use of the OBJECT card and now
       use the following card to identify which chip is being analysed */
    if (fits_read_key(infits,TSTRING,"HIERARCH ESO PRO CATG",card,comment,&status)) {
      INCLOSE; errormsg("Cannot read value of header card %s from file\n\t%s",
			"HIERARCH ESO PRO CATG",infile);
    }
    if (strstr(card,"BLUE") || strstr(card,"blue")) ts.chip=0;
    else if (strstr(card,"REDL") || strstr(card,"redl")) ts.chip=1;
    else if (strstr(card,"REDU") || strstr(card,"redu")) ts.chip=2;
    else {
      INCLOSE; errormsg("Do not understand values of header cards\n\
\t%s or %s\n\tfrom file\n\t%s","OBJECT","HIERARCH ESO PRO CATG",infile);
    }
  }
  /* Read binning values */
  if (fits_read_key(infits,TINT,"HIERARCH ESO DET WIN1 BINX",&ts.biny,comment,
		    &status)) {
    status=0; INCLOSE;
    errormsg("Cannot read value of header card %s from FITS file\n\t%s.",
	     "HIERARCH ESO DET WIN1 BINX",infile);
  }
  if (fits_read_key(infits,TINT,"HIERARCH ESO DET WIN1 BINY",&ts.binx,comment,
		    &status)) {
    status=0; INCLOSE;
    errormsg("Cannot read value of header card %s from FITS file\n\t%s.",
	     "HIERARCH ESO DET WIN1 BINY",infile);
  }
  /* Read tolerance value used in CPL reduction */
  i=0; while (sprintf(key,"%s%d%s","HIERARCH ESO PRO REC1 PARAM",i+1," NAME")>0 &&
	      !fits_read_key(infits,TSTRING,key,card,comment,&status) &&
	      strlower(card) && strncmp(card,"tolerance",9)) i++;
  if (!strncmp(card,"tolerance",9)) {
    sprintf(key,"%s%d%s","HIERARCH ESO PRO REC1 PARAM",i+1," VALUE");
    if (fits_read_key(infits,TDOUBLE,key,&ts.tol,comment,&status)) {
      status=0; INCLOSE;
      errormsg("Cannot read value of header card HIERARCH ESO PRO REC1 PARAM%d\n\
\tfrom FITS file\n\t%s.",i+1,infile);
    }
  } else {
    status=0; INCLOSE;
    errormsg("Cannot find/read header card containing wavelength calibration\n\
\ttolerance in/from FITS file\n\t%s.",infile);
  }
  /* Read polynomial value used in CPL reduction */
  i=0; while (sprintf(key,"%s%d%s","HIERARCH ESO PRO REC1 PARAM",i+1," NAME")>0 &&
	      !fits_read_key(infits,TSTRING,key,card,comment,&status) &&
	      strlower(card) && strncmp(card,"degree",6)) i++;
  if (!strncmp(card,"degree",6)) {
    sprintf(key,"%s%d%s","HIERARCH ESO PRO REC1 PARAM",i+1," VALUE");
    if (fits_read_key(infits,TINT,key,&ts.deg,comment,&status)) {
      status=0; INCLOSE;
      errormsg("Cannot read value of header card HIERARCH ESO PRO REC1 PARAM%d\n\
\tfrom FITS file\n\t%s.",i+1,infile);
    }
  } else {
    status=0; INCLOSE;
    errormsg("Cannot find/read header card containing wavelength calibration\n\
\ttolerance in/from FITS file\n\t%s.",infile);
  }
  /* Move to HDU containing ThAr information */
  if (fits_movrel_hdu(infits,4,&hdutype,&status)) {
    INCLOSE;
    errormsg("Cannot move to extension %d (HDU %d) in FITS file\n\t%s.",4,5,infile);
  }
  /* Check HDU type */
  if (hdutype!=BINARY_TBL) {
    INCLOSE;
    errormsg("Extension %d (HDU %d) not a binary table in file\n\t%s",4,5,infile);
  }
  /* Find and read in the diffraction order numbers from HISTORY cards */
  *inclist=search;
  while (!fits_find_nextkey(infits,inclist,1,inclist,0,card,&status) &&
	 strncmp(card,"HISTORY FABSORD",15));
  if (strncmp(card,"HISTORY FABSORD",15))
    errormsg("Cannot find header record beginning\n\
\twith %s in file \n\t%s","HISTORY FABSORD",infile);
  if (sscanf(card,"%s %s %d",dummy,dummy,&ts.o_ids)!=3)
    errormsg("Cannot read starting diffraction\n\
\torder number from file \n\t%s",infile);
  while (!fits_find_nextkey(infits,inclist,1,inclist,0,card,&status) &&
	 strncmp(card,"HISTORY LABSORD",15));
  if (strncmp(card,"HISTORY LABSORD",15))
    errormsg("Cannot find header record beginning\n\
\twith %s in file \n\t%s","HISTORY LABSORD",infile);
  if (sscanf(card,"%s %s %d",dummy,dummy,&ts.o_ide)!=3)
    errormsg("Cannot read ending diffraction\n\
\torder number from file \n\t%s",infile);
  /* Calculate number of diffraction orders */
  ts.no=abs(ts.o_ids-ts.o_ide+1);
  /* Calculate diffraction order slope */
  ts.o_slp=1; if (ts.o_ids>ts.o_ide) ts.o_slp=-1;
  /* Allocate memory for order results */
  if ((ts.o_rms=darray(ts.no))==NULL)
    errormsg("Cannot allocate memory for ts.o_rms array of length %d",ts.no);
  if ((ts.o_id=iarray(ts.no))==NULL)
    errormsg("Cannot allocate memory for ts.o_id array of length %d",ts.no);
  if ((ts.o_n=iarray(ts.no))==NULL)
    errormsg("Cannot allocate memory for ts.o_n array of length %d",ts.no);
  if ((ts.o_np=iarray(ts.no))==NULL)
    errormsg("Cannot allocate memory for ts.o_np array of length %d",ts.no);
  /* Label order id's */
  for (i=0; i<ts.no; i++) ts.o_id[i]=ts.o_ids+i*ts.o_slp;
  /* Find number of axes to be read in */
  if (fits_get_num_cols(infits,&naxis,&status))
    errormsg("Cannot read number of columns %s in FITS file\n\t%s","TFIELDS",infile);
  if (naxis!=26) errormsg("The binary table in file\n\t%s\n\
\thas %d columns. It should have %d",infile,naxis,26);
  /* Find number of rows to be read in */
  if (fits_get_num_rows(infits,&nrows,&status))
    errormsg("Cannot read number of rows %s in FITS file\n\t%s","NAXIS2",infile);
  ts.n=nrows;
  /* Allocate memory for ThAr line set */
  if ((ts.x=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.x array of length %d",ts.n);
  if ((ts.h=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.h array of length %d",ts.n);
  if ((ts.c=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.c array of length %d",ts.n);
  if ((ts.s=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.s array of length %d",ts.n);
  if ((ts.w=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.w array of length %d",ts.n);
  if ((ts.dis=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.dis array of length %d",ts.n);
  if ((ts.wlf=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.wlf array of length %d",ts.n);
  if ((ts.wlc=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.wlc array of length %d",ts.n);
  if ((ts.res=darray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.res array of length %d",ts.n);
  if ((ts.ora=iarray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.ora array of length %d",ts.n);
  if ((ts.orr=iarray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.orr array of length %d",ts.n);
  if ((ts.sts=iarray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.sts array of length %d",ts.n);
  if ((ts.stp=iarray(ts.n))==NULL)
    errormsg("Cannot allocate memory for ts.stp array of length %d",ts.n);
  /* Find and read in relevant columns */
  if (fits_get_colnum(infits,CASEINSEN,"X",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","X",infile);
  if (fits_read_col(infits,TDOUBLE,col,1,1,nrows,&nulval,ts.x,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","X",infile);
  if (fits_get_colnum(infits,CASEINSEN,"Peak",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","Peak",infile);
  if (fits_read_col(infits,TDOUBLE,col,1,1,nrows,&nulval,ts.h,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","Peak",infile);
  if (fits_get_colnum(infits,CASEINSEN,"Background",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","Background",
	     infile);
  if (fits_read_col(infits,TDOUBLE,col,1,1,nrows,&nulval,ts.c,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","Background",
	     infile);
  if (fits_get_colnum(infits,CASEINSEN,"Slope",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","Slope",infile);
  if (fits_read_col(infits,TDOUBLE,col,1,1,nrows,&nulval,ts.s,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","Slope",infile);
  if (fits_get_colnum(infits,CASEINSEN,"Xwidth",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","Xwidth",infile);
  if (fits_read_col(infits,TDOUBLE,col,1,1,nrows,&nulval,ts.w,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","Xwidth",infile);
  if (fits_get_colnum(infits,CASEINSEN,"Pixel",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","Pixel",infile);
  if (fits_read_col(infits,TDOUBLE,col,1,1,nrows,&nulval,ts.dis,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","Pixel",infile);
  if (fits_get_colnum(infits,CASEINSEN,"Ident",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","Ident",infile);
  if (fits_read_col(infits,TDOUBLE,col,1,1,nrows,&nulval,ts.wlc,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","Ident",infile);
  if (fits_get_colnum(infits,CASEINSEN,"WaveC",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","WaveC",infile);
  if (fits_read_col(infits,TDOUBLE,col,1,1,nrows,&nulval,ts.wlf,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","WaveC",infile);
  if (fits_get_colnum(infits,CASEINSEN,"AbsOrder",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","AbsOrder",
	     infile);
  if (fits_read_col(infits,TINT,col,1,1,nrows,&nulval,ts.ora,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","AbsOrder",
	     infile);
  if (fits_get_colnum(infits,CASEINSEN,"Y",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","Y",infile);
  if (fits_read_col(infits,TINT,col,1,1,nrows,&nulval,ts.orr,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","Y",infile);
  if (fits_get_colnum(infits,CASEINSEN,"Select",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","Select",infile);
  if (fits_read_col(infits,TINT,col,1,1,nrows,&nulval,ts.sts,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","Select",infile);
  if (fits_get_colnum(infits,CASEINSEN,"NLinSol",&col,&status))
    errormsg("Cannot find column '%s' in binary table in file\n\t%s","NLinSol",infile);
  if (fits_read_col(infits,TINT,col,1,1,nrows,&nulval,ts.stp,&anynul,&status))
    errormsg("Cannot read column '%s' in binary table in file\n\t%s","NLinSol",infile);

  /* Close input file */
  INCLOSE;

  /* Calculate the velocity-space residuals for each line */
  for (i=0; i<ts.n; i++) if (ts.sts[i]) ts.res[i]=C_C*(ts.wlf[i]-ts.wlc[i])/ts.wlc[i];

  /* Calculate the total number of lines selected and/or used in the
     polynomial solution */
  for (i=0,ts.ns=ts.np=0; i<ts.n; i++) {
    if (ts.sts[i]) { ts.ns++; if (ts.stp[i]) ts.np++; }
  }

  /* Calculate the mean residual */
  if (ts.n && !stats(ts.res,NULL,NULL,NULL,ts.stp,ts.n,0,&stat)) {
    FREETS; errormsg("Error returned from stats()");
  }
  ts.mrms=0.0; if (ts.n) ts.mrms=stat.rms;

  /* Determine number of orders represented in line set and make sure
     it is less than the number considered in the wavelength
     calibration process */
  if (ts.orr[ts.n-1]>(i=ts.orr[0])) i=ts.orr[ts.n-1];
  if (i>ts.no) warnmsg("Number of orders represented in file\n\t%s\n\
\t(=%d) is larger than the number considered in the wavelength calibration\n\
\tprocess (=%d). This indicates some inconsistency in this file.",infile,i,ts.no);

  /** Determine results for each order **/
  for (i=0,ts.nop=0,ts.mrmso=0.0; i<ts.no; i++) {
    /* Find first and last lines in this order and determine number of
       lines in this order */
    l=i+1; j=0; while (j<ts.n && ts.orr[j]!=l) j++;
    if (j!=ts.n) {
      k=j+1; while (k<ts.n && ts.orr[k]==l) k++;
      ts.o_n[i]=k-j; ts.nop++;
    } else ts.o_n[i]=0;
    /* Determine the number of lines in this order used in the polynomial solution */
    for (k=0,l=j,ts.o_np[i]=0; k<ts.o_n[i]; k++,l++) if (ts.stp[l]) ts.o_np[i]++;
    /* Find the rms residuals for this order */
    stat.rms=0.0;
    if (ts.o_np[i] &&
	!stats(&(ts.res[j]),NULL,NULL,NULL,&(ts.stp[j]),ts.o_n[i],0,&stat)) {
      FREETS; errormsg("Error returned from stats()");
    }
    ts.o_rms[i]=stat.rms; ts.mrmso+=ts.o_rms[i];
  }
  if (ts.nop) ts.mrmso/=(double)ts.nop;

  /* Display results if not finding tolerances */
  if (!tolm) {
    if (verb==2 || verb==1) fprintf(stdout,"\
 CENTRAL WAVELENGTH: %.0lf\n\
 BINNING: %dx%d\n\
 TOLERANCE: %6.3lf\n\
 DEGREE: %d\n",ts.cwl,ts.binx,ts.biny,ts.tol,ts.deg);
    if (verb==2) {
      fprintf(stdout," SEQ.NO  SPECTRAL  NO.LINES     STD.DEV.\n\
          ORDER                   M/S\n\
 ------  --------  --------    ---------\n");
      for (i=0; i<ts.no; i++)
	fprintf(stdout,"     %2d       %3d       %3d    %9.4lf\n",i+1,ts.o_id[i],
		ts.o_np[i],ts.o_rms[i]);
      fprintf(stdout," ---------------------------------------\n");
    }
    if (verb==2 || verb==1) fprintf(stdout,"\
 TOTAL NUM. LINES:     %4d\n\
 MEAN RMS RESIDUAL PER ORD:    %9.4lf\n\
 MEAN LINES PER ORD: %6.2lf\n\
 MEAN RMS RESIDUAL:            %9.4lf\n",ts.np,ts.mrmso,(double)ts.np/(double)ts.no,
	      ts.mrms);
    if (verb==0)
      fprintf(stdout,"%.0lf %1d %1d %6.3lf %4d %6.2lf %9.4lf %9.4lf\n",ts.cwl,ts.binx,
	      ts.biny,ts.tol,ts.np,(double)ts.np/(double)ts.no,ts.mrmso,ts.mrms);
  } else {
    /** Enter tolerance-finding mode **/
    /* First define minimum number of lines and target RMS given
       information about the wavelength setting being considered, the
       CCD binning and which chip (if using the red arm) */
    switch (ts.binx) {
    case 1:
      if (!ts.chip && ts.cwl<425.0) { nmin=12.0; RMStarg=40.0; }
      else if (!ts.chip) { nmin=13.0; RMStarg=35.0; }
      else if (ts.chip==1 && ts.cwl<700.0) { nmin=17.0; RMStarg=30.0; }
      else if (ts.chip==1) { nmin=15.0; RMStarg=35.0; }
      else if (ts.chip==2 && ts.cwl<700.0) { nmin=15.0; RMStarg=35.0; }
      else if (ts.chip==2) { nmin=12.0; RMStarg=40.0; }
      else {
	INCLOSE; errormsg("Do not understand combination of chip name\n\
\tand central wavelength, %d and %lf, in file\n\t%s\n\
\tChip names: 0=blue; 1=redl; 2=redu",ts.chip,ts.cwl);
      }
      break;
    case 2:
      if (!ts.chip && ts.cwl<425.0) { nmin=10.0; RMStarg=75.0; }
      else if (!ts.chip) { nmin=11.0; RMStarg=70.0; }
      else if (ts.chip==1 && ts.cwl<700.0) { nmin=14.0; RMStarg=60.0; }
      else if (ts.chip==1) { nmin=12.0; RMStarg=65.0; }
      else if (ts.chip==2 && ts.cwl<700.0) { nmin=13.0; RMStarg=55.0; }
      else if (ts.chip==2) { nmin=10.0; RMStarg=70.0; }
      else {
	INCLOSE; errormsg("Do not understand combination of chip name\n\
\tand central wavelength, %d and %lf, in file\n\t%s\n\
\tChip names: 0=blue; 1=redl; 2=redu",ts.chip,ts.cwl);
      }
      break;
    default:
      if (!ts.chip && ts.cwl<425.0) { nmin=9.0; RMStarg=110.0; }
      else if (!ts.chip) { nmin=10.0; RMStarg=100.0; }
      else if (ts.chip==1 && ts.cwl<700.0) { nmin=11.0; RMStarg=95.0; }
      else if (ts.chip==1) { nmin=10.0; RMStarg=100.0; }
      else if (ts.chip==2 && ts.cwl<700.0) { nmin=12.0; RMStarg=90.0; }
      else if (ts.chip==2) { nmin=9.0; RMStarg=100.0; }
      else {
	INCLOSE; errormsg("Do not understand combination of chip name\n\
\tand central wavelength, %d and %lf, in file\n\t%s\n\
\tChip names: 0=blue; 1=redl; 2=redu",ts.chip,ts.cwl);
      }
      break;
    }
    /* Check consistency of tolerance used in polynomial solution and
       minimum tol for arrays */
    if (ts.tol<=tolmin+tolstep) {
      INCLOSE; FREETS;
      errormsg("Tolerance used in polynomial solution (=%lf) in file\n\t%s\n\
\tis less than or too similar to minimum allowed (=%lf)",ts.tol,infile,tolmin);
    }
    /* Determine number of steps to use in constructing tol, nav and rms arrays */
    ntol=(int)((ts.tol-tolmin)/tolstep)+1;
    if ((tol=darray(ntol))==NULL) {
      INCLOSE; FREETS;
      errormsg("Cannot allocate memory for tol array of length %d",ntol);
    }
    if ((nav=darray(ntol))==NULL) {
      INCLOSE; FREETS;
      errormsg("Cannot allocate memory for nav array of length %d",ntol);
    }
    if ((rms=darray(ntol))==NULL) {
      INCLOSE; FREETS;
      errormsg("Cannot allocate memory for rms array of length %d",ntol);
    }
    /* Fill tol, nav and rms arrays */
    for (i=0; i<ntol; i++) {
      tol[i]=tolmin+tolstep*(double)i;
      if (!UVES_tolstat(&ts,tol[i],&nav[i],&rms[i])) {
	INCLOSE; FREETS; FREETOL; errormsg("Error returned from UVES_tolstat() when\n\
\tcomputing for tolerance = %lf",tol[i]);
      }
    }
    /* Now find the tolerances as per tolm value/request */
    switch (tolm) {
    case 1:
      /* First tolerance finding iteration is to determine whether
	 average number of ThAr lines found at this tolerance, N(tol),
	 is greater than nmin. If so, then we also predict at which
	 tol the RMS is equal to RMStarg */
      if ((double)ts.np/(double)ts.no>=nmin) {
	/* Find last tol in array which provides an RMS less than target */
	i=0; while (i<ntol && rms[i]<RMStarg) i++;
	/* Does the tolerance for which the rms target is achieved
	   provide enough lines? */
	if (i<ntol && nav[i]>=nmin) { TOLMOUT(1,tol[i]); }
	else {
	  /* If not, or the target rms is never reached, find the
	     minimum tolerance which provides enough lines */
	  i=0; while (i<ntol && nav[i]<nmin) i++; if (i==ntol) i--;
	  TOLMOUT(1,tol[i]);
	}
      } else { TOLMOUT(0,0.0); }
      break;
    case 2:
      /* Second tolerance finding iteration is to zero-in on the
	 target RMS if possible */
      if ((double)ts.np/(double)ts.no>=nmin) {
	if (ts.mrms>RMStarg) {
	  /* If rms at this tolerance is worse than the target rms
	     then see if there's predicted to be a tolerance which
	     provide enough lines and an RMS slightly better than the
	     target */
	  i=0; while (i<ntol && rms[i]<RMStarg) i++;
	  if (i==0 || i==ntol) { TOLMOUT(1,ts.tol); }
	  else {
	    if (nav[i-1]>=nmin) { TOLMOUT(1,tol[i-1]); }
	    else {
	      while (i<ntol && nav[i]<nmin) i++; if (i==ntol) i--;
	      TOLMOUT(1,tol[i]);
	    }
	  }
	} else {
	  /* If the RMS is too low, see if stepping to next tolerance
	     value would still leave us below the target. Do this by
	     assuming that the rms changes linearly with tol. Step no
	     more than once. */
	  i=0; if ((rmsdiff=(rms[ntol-1]-rms[0])/(double)(ntol-1))>0.0) {
	    if (ts.mrms+rmsdiff>RMStarg) i=0;
	    else i=1;
	  }
	  TOLMOUT(1,(ts.tol+tolstep*(double)i));
	}
      } else {
	/* If not enough lines, see if stepping to next tolerance
	   value would still leave us below the target. Do this by
	   assuming that nav changes linearly with tol. Step no more
	   than three times. */
	i=0; if ((navdiff=(nav[ntol-1]-nav[0])/(double)(ntol-1))>0.0) {
	  if ((double)ts.np/(double)ts.no+navdiff>=nmin) i=1;
	  else if ((double)ts.np/(double)ts.no+2.0*navdiff>=nmin) i=2;
	  else i=3;
	}
	TOLMOUT(1,(ts.tol+tolstep*(double)i));
      }
      break;
    case 3:
      /* Third tolerance finding iteration is to determine whether
	 enough lines have been used in the polynomial solution */
      if ((double)ts.np/(double)ts.no>=nmin) { TOLMOUT(1,ts.tol); }
      else {
	/* If not enough lines, see if stepping to next tolerance
	   value would still leave us below the target. Do this by
	   just looking at the difference between this nav value and
	   the previous one. Step no more than three times. */
	i=0; if ((navdiff=(nav[ntol-1]-nav[0])/(double)(ntol-1))>0.0) {
	  if ((double)ts.np/(double)ts.no+navdiff>=nmin) i=1;
	  else if ((double)ts.np/(double)ts.no+2.0*navdiff>=nmin) i=2;
	  else if ((double)ts.np/(double)ts.no+3.0*navdiff>=nmin) i=3;
	  else i=0;
	}
	if (i) { TOLMOUT(1,(ts.tol+tolstep*(double)i)); }
	else { TOLMOUT(0,ts.tol); }
      }
      break;
    }
  }

  /* Clean up */
  FREETS; FREETOL;

  return 1;

}
