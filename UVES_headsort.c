/****************************************************************************

UVES_headsort: Sort ESO/VLT UVES exposures based on header-card values

****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "UVES_headsort.h"
#include "file.h"
#include "memory.h"
#include "error.h"

/* Global declarations */
char      *progname;

/****************************************************************************
* Print the usage message
****************************************************************************/

void usage(void) {

  fprintf(stderr,"\n%s: Sort ESO/VLT UVES exposures based on\n\
\theader-card values\n",progname);

  fprintf(stderr,"\nBy Michael Murphy (http://www.ast.cam.ac.uk/~mim)\n\
\nVersion: %4.2lf (03rd June 2013)\n",VERSION);

  fprintf(stderr,"\nUsage: %s [OPTIONS] [FITS file or list]\n", progname);

  fprintf(stderr, "\nOptions:\n\
  -c    = %4.1lf %4.1lf : Calibration period: Number of hours before and after\n\
                       science exposures to search for calibration exposures.\n\
  -bias = %1d         : Number of biases: Collect N biases per science exposure.\n\
  -flat = %1d         : Number of flats: Collect N flats per science exposure\n\
  -wav  = %1d         : Number of wavelength cal.s: Collect N wavs per science\n\
                       exposure.\n\
  -ord  = %1d         : Number of order definitions: Collect N ords per\n\
                       science exposure.\n\
  -fmt  = %1d         : Number of format checks: Collect N fmts per science\n\
                       exposure.\n\
  -std  = %1d         : Number of standards: Collect N stds per\n\
                       science exposure.\n\
  -redscr           : Turn off reduction script writing.\n\
  -redstd           : Include standards in reduction scripts.\n\
  -tharfile = %s\n\
                    : Full absolute pathname of reference laboratory ThAr\n\
                       frame. Only needed if environment variable\n\
                       UVES_HEADSORT_THARFILE not set.\n\
  -atmofile = %s\n\
                    : Full absolute pathname of reference atmospheric line\n\
                       frame. Only needed if environment variable\n\
                       UVES_HEADSORT_ATMOFILE not set.\n\
  -flstfile = %s\n\
                    : Full absolute pathname of flux standard reference\n\
                       frame. Only needed if environment variable\n\
                       UVES_HEADSORT_FLSTFILE not set.\n\
  -info [opt. FILE] : Write a file containing header info. for FITS file list.\n\
  -list             : Write lists of relevant files for each science exposure.\n\
  -macmap [opt. FILE] : Write a file specifying the mapping of the science\n\
                        object directories and file indices between\n\
                        case-sensitive and case-insensitive operating systems,\n\
                        e.g. Mac and linux, that would have been used before\n\
                        upper-case object names were enforced in Version 0.40.\n\
  -d                : Debug mode: search for errors associated with given\n\
                       files; don't create any direcories, links or files.\n\
  -h, -help         : Print this message.\n\n",
	  NHRSCAL_B,NHRSCAL_F,NBIAS,NFLAT,NWAV,NORD,NFMT,NSTD,THARFILE,ATMOFILE,
	  FLSTFILE);
  exit(3);
}

/****************************************************************************
* The main program

  Things to do:

****************************************************************************/

int main(int argc, char *argv[]) {

  int      debug=0,redscr=1,redstd=0,info=0,list=0,macmap=0;
  int      nhdrs=0;  /* Number of headers = Number of FITS files */
  int      ncal=0;   /* Maximum # calibrations selected of any type */
  int      nscis=0;  /* Number of science frames found in list */
  int      i=0;
  char     infile[NAMELEN]="\0",infofile[NAMELEN]="\0",macmapfile[NAMELEN]="\0";
  char     buffer[LNGSTRLEN]="\0";
  char     *tharfile=NULL,*atmofile=NULL,*flstfile=NULL;
  char     *cptr=NULL;
  FILE     *data_file=NULL;
  calprd   cprd;    /* Structure holding calibration period info. */
  header   *hdrs;   /* Array to contain all header info */
  scihdr   *scis;   /* Array of sci. hdrs with info about associated cals. */

  /* Define the program name from the command line input */
  progname=((progname=strrchr(argv[0],'/'))==NULL) ? argv[0] : progname+1;
  /* Must be at least one argument */
  if (argc==1) usage();
  /* Set reference file path names */
  tharfile=((cptr=getenv("UVES_HEADSORT_THARFILE"))==NULL) ? THARFILE : cptr; 
  atmofile=((cptr=getenv("UVES_HEADSORT_ATMOFILE"))==NULL) ? ATMOFILE : cptr; 
  flstfile=((cptr=getenv("UVES_HEADSORT_FLSTFILE"))==NULL) ? FLSTFILE : cptr; 
  strcpy(infofile,INFOFILE); strcpy(macmapfile,MACMAPFILE);
  /* Initialize parameters */
  if (!UVES_params_init(&cprd)) errormsg("Error returned from UVES_params_init()");
  /* Scan command line for options */
  while (++i<argc) {
    if (!strcmp(argv[i],"-d")) debug=1; /* Enter debug mode */
    else if (!strcmp(argv[i],"-c")) {
      if (sscanf(argv[++i],"%lf",&(cprd.nhrscal_b))!=1) usage();
      if (sscanf(argv[++i],"%lf",&(cprd.nhrscal_f))!=1) usage();
    }
    else if (!strcmp(argv[i],"-bias")) {
      if (sscanf(argv[++i],"%d",&(cprd.nbias))!=1) usage();
    }
    else if (!strcmp(argv[i],"-flat")) {
      if (sscanf(argv[++i],"%d",&(cprd.nflat))!=1) usage();
    }
    else if (!strcmp(argv[i],"-wav")) {
      if (sscanf(argv[++i],"%d",&(cprd.nwav))!=1) usage();
    }
    else if (!strcmp(argv[i],"-ord")) {
      if (sscanf(argv[++i],"%d",&(cprd.nord))!=1) usage();
    }
    else if (!strcmp(argv[i],"-fmt")) {
      if (sscanf(argv[++i],"%d",&(cprd.nfmt))!=1) usage();
    }
    else if (!strcmp(argv[i],"-std")) {
      if (sscanf(argv[++i],"%d",&(cprd.nstd))!=1) usage();
      if (!cprd.nstd) redstd=0;
    }
    else if (!strcmp(argv[i],"-info")) {
      info=1; if (i+1<argc && strncmp(argv[i+1],"-",1)) {
	if (sscanf(argv[++i],"%s",infofile)!=1) usage();
      }
    }
    else if (!strcmp(argv[i],"-macmap")) {
      macmap=1; if (i+1<argc && strncmp(argv[i+1],"-",1)) {
	if (sscanf(argv[++i],"%s",macmapfile)!=1) usage();
      }
    }
    else if (!strcmp(argv[i],"-list")) list=1;
    else if (!strcmp(argv[i],"-redscr")) redscr=0;
    else if (!strcmp(argv[i],"-redstd")) redstd=1;
    else if (!strcmp(argv[i],"-tharfile")) {
      if (++i>=argc || (strrchr((tharfile=argv[i]),'/'))==NULL)
	errormsg("Must specify full pathname of lab. ThAr frame");
    }
    else if (!strcmp(argv[i],"-atmofile")) {
      if (++i>=argc || (strrchr((atmofile=argv[i]),'/'))==NULL)
	errormsg("Must specify full pathname of reference Atmo. frame");
    }
    else if (!strcmp(argv[i],"-flstfile")) {
      if (++i>=argc || (strrchr((flstfile=argv[i]),'/'))==NULL)
	errormsg("Must specify full pathname of reference Flx. std. frame");
    }
    else if (!strcmp(argv[i],"-help") || !strcmp(argv[i],"-h")) usage();
    else if (!access(argv[i],R_OK)) {
      if (strlen(argv[i])<=NAMELEN) strcpy(infile,argv[i]);
      else errormsg("Input file name too long: %s",argv[i]);
    }
    else errormsg("File %s does not exist",argv[i]);
  }
  /* Make sure an input file was specified */
  if (!strncmp(infile,"\0",1)) usage();
  /* Set any unset parameters */
  if (!UVES_params_set(&cprd)) errormsg("Error returned from UVES_params_set()");
 
  /* Temporary warning messaage if number of standards requested is > 1 */
  if (cprd.nstd>1) {
    warnmsg("At present, there is no provision for selecting more\n\
\tthan 1 standard exposure per science exposure. Continuing to run with\n\
\tN=1 for the standard calibration files");
    cprd.nstd=1;
  }

  /* Check the ThAr, Atmo and FlSt files for existence if required */
  if (redscr) {
    if (access(tharfile,R_OK))
      warnmsg("ThAr laboratory frame\n\
\t%s\n\tdoes not exist. Will write reduction preparation scripts regardless.",
	      tharfile);
    if (access(atmofile,R_OK))
      warnmsg("Atmospheric line reference frame\n\
\t%s\n\tdoes not exist. Will write reduction preparation scripts regardless.",
	      atmofile);
    if (access(flstfile,R_OK))
      warnmsg("Flux standard reference frame\n\
\t%s\n\tdoes not exist. Will write reduction preparation scripts regardless.",
	      flstfile);
  }

  /* Check to make sure maximum number of calibrations requested is OK */
  ncal=MAX(cprd.nbias,cprd.nflat); ncal=MAX(ncal,(MAX(cprd.nwav,cprd.nfmt)));
  ncal=MAX(ncal,(MAX(cprd.nord,cprd.nstd)));
  if (ncal>NCALMAX) errormsg("To many calibrations requested, %d.\n\
\tIncrease NCALMAX in UVES_headsort.h");

  /* Convert calibration period to days and find _total_ cal. preiod*/
  cprd.ndscal_f=cprd.nhrscal_f/24.0; cprd.ndscal_b=cprd.nhrscal_b/24.0;
  
  /* Open input file, see if it's a list of FITS files or a FITS file iself */
  if ((data_file=faskropen("Valid input FITS file or list?",infile,5))
      ==NULL) errormsg("Can not open file %s",infile);
  
  /* Read list of FITS file names from input file */
  if ((cptr=fgets(buffer,LNGSTRLEN,data_file))==NULL) {
    fclose(data_file); errormsg("Problem reading file %s on line %d",infile,1);
  }

  if (!strncmp(buffer,"SIMPLE  =",8)) {
    /* Single file specified that looks suspiciously like a FITS file */
    fclose(data_file); nhdrs=1;
    /* Allocate memory for single header */
    if (!(hdrs=(header *)malloc((size_t)(nhdrs*sizeof(header)))))
      errormsg("Could not allocate memory for header array of size %d",nhdrs);
    cptr=((cptr=strrchr(strcpy(hdrs[0].file,infile),'/'))==NULL) ?
      hdrs[0].file : cptr+1; strcpy(hdrs[0].abfile,cptr);
  }
  else {
    /* Check for absolute path names ... a weak check anyway */
    if (strncmp(buffer,"/",1)) {
      fclose(data_file);
      errormsg("FITS file path invalid on line %d in file\n\
\t%s.\n\tYou must use absolute path names - FITS file names must begin with '/'.",
	       1,infile);
    }
    /* If not FITS file see if it is a list of valid FITS files instead */
    i=1; while ((cptr=fgets(buffer,LNGSTRLEN,data_file))!=NULL) {
      /* Check for absolute path names ... a weak check anyway */
      if (strncmp(buffer,"/",1)) {
	fclose(data_file);
	errormsg("FITS file path invalid on line %d in file\n\
\t%s.\n\tYou must use absolute path names - FITS file names must begin with '/'.",
		 i+1,infile);
      }
      i++;
    }
    if (!feof(data_file)) {
      fclose(data_file);
      errormsg("Problem reading file %s on line %d",infile,i+1);
    }
    else {
      rewind(data_file); nhdrs=i;
    }

    /* Allocate enough memory for headers */
    if (!(hdrs=(header *)malloc((size_t)(nhdrs*sizeof(header)))))
      errormsg("Could not allocate memory for header array of size %d",nhdrs);

    /* Read in list names of FITS files */
    for (i=0; i<nhdrs; i++) {
      cptr=fgets(buffer,LNGSTRLEN,data_file);
      if (sscanf(buffer,"%s",hdrs[i].file)!=1) {
	fclose(data_file);
	errormsg("Incorrect format in line %d of file %s",i,infile);
      }
      cptr=((cptr=strrchr(hdrs[i].file,'/'))==NULL) ? hdrs[i].file : cptr+1;
      strcpy(hdrs[i].abfile,cptr);
    }
    fclose(data_file);
    if (debug)
      fprintf(stdout,"INFO: Input file %s read successfully ...\n",infile);
  }

  /* Read in headers from FITS files */
  for (i=0; i<nhdrs; i++) {
    if (!UVES_rfitshead(hdrs[i].file,&(hdrs[i])))
      errormsg("Unknown error returned from UVES_rfitshead()");
  }
  if (debug) fprintf(stdout,"INFO: All FITS files read successfully ...\n");

  /* Sort headers in order of increasing MJD */
  qsort(hdrs,nhdrs,sizeof(header),qsort_mjd);

  /* Go through list of headers and identify the science exposures and
     allocate memory enough to hold info about them */
  for (i=0; i<nhdrs; i++)
    if (!strcmp(hdrs[i].typ,"sci")) nscis++;
  if (!(scis=(scihdr *)malloc((size_t)(nscis*sizeof(scihdr)))))
    errormsg("Could not allocate memory for science header array\n\
\tof size %d.",nscis);

  /* Write out header information output file if requested */
  if (info) {
    if (!UVES_wheadinfo(hdrs,nhdrs,infofile))
      errormsg("Uknown error returned from UVES_wheadinfo()");
  }

  /* Identify calibration files most appropriate for science frames */
  if (!UVES_calsrch(hdrs,nhdrs,scis,nscis,&cprd,ncal))
    errormsg("Unknown error returned from UVES_calsrch()");
  if (debug) fprintf(stdout,"INFO: Search for relevant calibration frames \
conducted successfully ...\n");

  /* Create a list of relevant files for each science object */
  if (list) {
    if (!UVES_list(hdrs,nhdrs,scis,nscis))
      errormsg("Unknown error returned from UVES_list()");
    if (debug) fprintf(stdout,"INFO: Created lists of relevant files for each \
object successfully ...\n");
  }

  /* Create a map of case-sensitive and case-insensitive object names
     and file indices */
  if (macmap) {
    if (!UVES_Macmap(hdrs,nhdrs,scis,nscis))
      errormsg("Unknown error returned from UVES_Macmap()");
    if (debug)
      fprintf(stdout,"INFO: Created map of case-sensitive vs. insensitive object\n\
\tnames and file indices successfully ...\n");
  }

  /* Create object subdirectories and symbolic links to FITS file,
     appropriately named */
  if (!debug) {
    if (!UVES_link(hdrs,nhdrs,scis,nscis))
      errormsg("Unknown error returned from UVES_link()");
  }

  /* Write out MIDAS and CPL reduction scripts if required */
  if (!debug && redscr) {
    if (!UVES_wredscr(scis,nscis,redstd,tharfile,atmofile,flstfile))
      errormsg("Unknown error returned from UVES_wredscr()");
  }

  /* Clean up */
  free(hdrs); free(scis);

  return 1;

}
