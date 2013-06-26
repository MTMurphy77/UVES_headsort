/****************************************************************************

UVES_copyhead: Copy the primary header from one FITS file to a new, empty
HDU in an existing, non-empty FITS file or list of non-empty FITS files.

****************************************************************************/

/* Include files */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fitsio.h>
#include <longnam.h>
#include "charstr.h"
#include "file.h"
#include "error.h"

/* Definitions */
#define EXT 0
#define INCLOSE  fits_close_file(infits,&status);
#define OUTCLOSE  fits_close_file(outfits,&status);
#define DATCLOSE  fclose(data_file);

/* Global declarations */
char      *progname;

/****************************************************************************
* Print the usage message
****************************************************************************/

void usage(void) {

  fprintf(stderr,"\n%s: Copy a header from one FITS file\n\
\tto a new, empty HDU in an existing, non-empty FITS file.\n",progname);

  fprintf(stderr,"\nBy Michael Murphy");

  fprintf(stderr,"\nUsage: %s [OPTIONS] [INPUT FITS FILE] [OUTPUT FITS file or list]\n\
",progname);

  fprintf(stderr, "\nOptions:\n\
  -h, -help        : Print this message.\n\
  -ext  = %2d       : Copy header in extension number x from input file.\n\n",
	  EXT);
  exit(3);
}

/****************************************************************************
* The main program

  Things to do:

****************************************************************************/

int main(int argc, char *argv[]) {

  int      ext=-1;
  int      hdunum=0,hdutype=0,status=0;
  int      i=0;
  char     infile[NAMELEN]="\0",outfile[NAMELEN]="\0",datfile[NAMELEN]="\0";
  char     buffer[LNGSTRLEN]="\0";
  FILE     *data_file=NULL;
  fitsfile *infits,*outfits;

  /* Define the program name from the command line input */
  progname=((progname=strrchr(argv[0],'/'))==NULL) ? argv[0] : progname+1;
  /* Must be at least two arguments */
  if (argc<=1) usage();
  /* Scan command line for options */
  while (++i<argc) {
    if (!strcmp(argv[i],"-ext")) {
      if (sscanf(argv[++i],"%d",&(ext))!=1) usage();
    }
    else if (!strcmp(argv[i],"-help") || !strcmp(argv[i],"-h")) usage();
    else if (!strncmp(infile,"\0",1)) {
      if (!access(argv[i],R_OK)) {
	if (strlen(argv[i])<=NAMELEN) strcpy(infile,argv[i]);
	else errormsg("Input file name too long: %s",argv[i]);
      }
      else errormsg("Input file %s does not exist",argv[i]);
    }
    else if (!strncmp(outfile,"\0",1)) {
      if (!access(argv[i],W_OK)) {
	if (strlen(argv[i])<=NAMELEN) strcpy(outfile,argv[i]);
	else errormsg("Input file name too long: %s",argv[i]);
      }
      else errormsg("Input file %s either does not exist or\n\
\tcannot be written to",argv[i]);
    }
  }
  /* Make sure input and output files were specified */
  if (!strncmp(infile,"\0",1) || !strncmp(outfile,"\0",1)) usage();
  /* Make sure extension number makes sense, initialize it if not */
  if (ext<0) ext=EXT;

  /* Open input file as FITS file */
  if (fits_open_file(&infits,infile,READONLY,&status))
      errormsg("Cannot open input FITS file %s",infile);
  /* Check number of HDUs */
  if (fits_get_num_hdus(infits,&hdunum,&status))
    errormsg("Cannot find number of HDUs in file %s",infile);
  if (ext>hdunum-1)
    errormsg("Cannot copy extension number %d from file\n\
\t%s since only %d HDUs exist",ext,infile,hdunum);
  /* Move to target HDU */
  if (fits_movrel_hdu(infits,ext,&hdutype,&status))
    errormsg("Cannot move to extension %d (HDU %d)\n\
\tin FITS file %s.",ext,ext+1,infile);

  /* First attempt to open output file as a FITS file. If this fails,
     assume it's a list of FITS files */
  if (!fits_open_file(&outfits,outfile,READWRITE,&status)) {
    /* This must be a single output FITS file */
    /* Attempt to copy header */
    if (fits_copy_header(infits,outfits,&status)) {
      INCLOSE; OUTCLOSE; errormsg("Failed to copy header from input file %s\n\
\tto output file %s",infile,outfile);
    }
    /* Close the output FITS file */
    OUTCLOSE;
  } else {
    /* This might be a list of output FITS files */
    status=0; sprintf(datfile,"%s",outfile);
    /* Open the output file */
    if ((data_file=faskropen("Valid list of output FITS files?",datfile,5))==NULL)
      { INCLOSE; errormsg("Can not open out file %s",datfile); }
    /* Read the list of putative output FITS files */
    i=0; while (fgets(buffer,LNGSTRLEN,data_file)!=NULL) {
      /* Attempt to open this output FITS file */
      if (sscanf(buffer,"%s",outfile)!=1) {
	INCLOSE; DATCLOSE;
	errormsg("Incorrect format in line %d of file %s",i+1,infile);
      }
      if (strlen(outfile)>=NAMELEN-1) {
	INCLOSE; DATCLOSE;
	errormsg("Output file name\n\t%s\n\tspecified on line %d of output list\n\
\tfile %s is too long",outfile,i+1,datfile);
      }
      if (fits_open_file(&outfits,outfile,READWRITE,&status)) {
	INCLOSE; DATCLOSE; errormsg("Cannot open output FITS file %s\n\
\tspecified on line %d of output file list %s",outfile,i+1,datfile); 
      }
      /* Attempt to copy header */
      if (fits_copy_header(infits,outfits,&status)) {
	INCLOSE; OUTCLOSE; DATCLOSE;
	errormsg("Failed to copy header from input file %s\n\
\tto output file %s specified on line %d\n\
\tof output file list %s",infile,outfile,i+1,datfile);
      }
      /* Close this output FITS file */
      OUTCLOSE;
      i++;
    }
    if (!feof(data_file)) {
      INCLOSE; DATCLOSE;
      errormsg("Problem reading file %s on line %d",outfile,i+1);
    }
    DATCLOSE;
  }
  /* Close the input FITS file */
  INCLOSE;

  return 1;

}
