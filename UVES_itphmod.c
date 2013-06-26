/****************************************************************************

UVES_itphmod: Read the output from the CPL uves_cal_predict command to
determine the offsets of the physical model.

****************************************************************************/

/* Include files */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stats.h"
#include "charstr.h"
#include "file.h"
#include "memory.h"
#include "error.h"

/* Definitions */
#define NINT(a) (a-(int)(a)<0.5) ? (int)(a) : (int)(a+1.0)

/* Global declarations */
char      *progname;

/****************************************************************************
* Print the usage message
****************************************************************************/

void usage(void) {

  fprintf(stderr,"\n%s: Read the output from the CPL uves_cal_predict\n\
\tcommand to determine the offsets of the physical model.\n",progname);

  fprintf(stderr,"\nBy Michael Murphy");

  fprintf(stderr,"\nUsage: %s [OPTIONS] [INPUT DATA FILE]\n",progname);

  fprintf(stderr, "\nOptions:\n\
  -h, -help        : Print this message.\n\n");
  exit(3);
}

/****************************************************************************
* The main program

  Things to do:

****************************************************************************/

int main(int argc, char *argv[]) {

  double   xdif_med=0.0,ydif_med=0.0;
  double   ddum=0.0;
  double   *xmod=NULL,*ymod=NULL,*xdif=NULL,*ydif=NULL;
  int      n=0;
  int      i=0,j=0,k=0;
  char     infile[NAMELEN]="\0";
  char     buffer[VVLNGSTRLEN]="\0";
  FILE     *data_file=NULL;
  statset  stat;

  /* Define the program name from the command line input */
  progname=((progname=strrchr(argv[0],'/'))==NULL) ? argv[0] : progname+1;
  /* Must be at least 1 argument */
  if (argc<1) usage();
  /* Scan command line for options */
  while (++i<argc) {
    if (!strcmp(argv[i],"-help") || !strcmp(argv[i],"-h")) usage();
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

  /* Open the input file */
  if ((data_file=faskropen("Valid input data file?",infile,5))==NULL)
    errormsg("Can not open input data file %s",infile);

  /** Read the input data file **/
  /* First find the XMOD/XDIF data and determine the number of entries */
  i=0; while (fgets(buffer,VVLNGSTRLEN,data_file)!=NULL) {
    if (strstr(buffer,"XMOD")!=NULL && strstr(buffer,"XDIF")!=NULL) break;
    i++;
  }
  if (feof(data_file)) {
    fclose(data_file); errormsg("Reached end of file at line %d in file %s.\n\
\tWas expecting to find a line containing both the strings\n\
\t'XMOD' and 'XDIF'",i+1,infile);
  }
  k=++i; j=0; while (fgets(buffer,VVLNGSTRLEN,data_file)!=NULL) {
    if (sscanf(buffer,"%lf %lf",&ddum,&ddum)!=2) {
      if (!strncmp(buffer,"e",1)) { rewind(data_file); break; }
      else {
	fclose(data_file);
	errormsg("Incorrect format in line %d of file %s",i+1,infile);
      }
    }
    i++; j++;
  }
  n=j;

  /* Allocate memory for data arrays */
  if ((xmod=darray(n))==NULL) {
    fclose(data_file); errormsg("Cannot allocate memory for xmod array\n\
\tof length %d",n);
  }
  if ((xdif=darray(n))==NULL) {
    fclose(data_file); errormsg("Cannot allocate memory for xdif array\n\
\tof length %d",n);
  }
  if ((ymod=darray(n))==NULL) {
    fclose(data_file); errormsg("Cannot allocate memory for ymod array\n\
\tof length %d",n);
  }
  if ((ydif=darray(n))==NULL) {
    fclose(data_file); errormsg("Cannot allocate memory for ydif array\n\
\tof length %d",n);
  }

  /* Find the XMOD/XDIF data again and read it in */
  for (i=0; i<k; i++) if (fgets(buffer,VVLNGSTRLEN,data_file)==NULL)
    errormsg("Problem reading line %d in file %s",i+1,infile);
  for (i=0; i<n; i++) {
    if (fgets(buffer,VVLNGSTRLEN,data_file)==NULL)
      errormsg("Problem reading line %d in file %s",i+k,infile);
    if (sscanf(buffer,"%lf %lf",&(xmod[i]),&(xdif[i]))!=2) {
      fclose(data_file);
      errormsg("Incorrect format in line %d of file %s",i+k,infile);
    }
  }
  rewind(data_file);

  /* Now find the YMOD/YDIF data and read it in */
  i=0; while (fgets(buffer,VVLNGSTRLEN,data_file)!=NULL) {
    if (strstr(buffer,"YMOD")!=NULL && strstr(buffer,"YDIF")!=NULL) break;
    i++;
  }
  if (feof(data_file)) {
    fclose(data_file); errormsg("Reached end of file at line %d in file %s.\n\
\tWas expecting to find a line containing both the strings\n\
\t'YMOD' and 'YDIF'",i+1,infile);
  }
  i++; j=0; while (fgets(buffer,VVLNGSTRLEN,data_file)!=NULL) {
    if (j>n) {
      fclose(data_file);
      errormsg("There appears to be more YMOD/YDIF data entries\n\
\tthan XMOD/XDIF entires (=%d)",n);
    }
    if (sscanf(buffer,"%lf %lf",&(ymod[j]),&(ydif[j]))!=2) {
      if (!strncmp(buffer,"e",1)) break;
      else {
	fclose(data_file);
	errormsg("Incorrect format in line %d of file %s",i+1,infile);
      }
    }
    i++; j++;
  }
  if (j<n) {
    fclose(data_file);
    errormsg("There appears to be fewer YMOD/YDIF data entries\n\
\t(=%d) than XMOD/XDIF entires (=%d)",j,n);
  }

  /* Close the data file */
  fclose(data_file);

  /* Determine the median values of XDIF and YDIF */
  if (!median(xdif,NULL,n,&stat,0))
    errormsg("Error determining median value of xdif");
  xdif_med=stat.med;
  if (!median(ydif,NULL,n,&stat,0))
    errormsg("Error determining median value of ydif");
  ydif_med=stat.med;

  /* Output the number of points and the median offsets rounded to the
     nearest half integer */
  fprintf(stdout,"%d %4.1lf %4.1lf\n",n,0.5*(NINT(2.0*xdif_med)),
	  0.5*(NINT(2.0*ydif_med)));

  /* Clean up */
  free(xmod); free(xdif); free(ymod); free(ydif);

  return 1;

}
