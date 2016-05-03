/***************************************************************************
* Definitions, structures and function prototypes for UVES_HEADSORT
***************************************************************************/

/* INCLUDE FILES */
#include <fitsio.h>
#include <longnam.h>
#include "charstr.h"

/* DEFINITIONS */
/* Version number */
#define VERSION    0.55

/* Functions */
#define MAX(a,b)   a>b ? a : b
#define MIN(a,b)   a<b ? a : b

#define BORR     500.0  /* Cross-over wavelength (nm) for blue/red settings  */
#define NHRSCAL_B 12.0  /* Default backward # hours to search for cal. files */
#define NHRSCAL_F 12.0  /* Default forward # hours to search for cal. files  */
                  /* Following integer must be < 100      */
#define NCALMAX   20    /* Maximum # cal. frames of each type per sci. frame */
                  /* Following integers must be < NCALMAX */
#define NBIAS      5    /* Default # biases to find per science exp.         */
#define NFLAT      5    /* Default # flats to find ...                       */
#define NWAV       1    /* Default # wavlength cal.s to find ...             */
#define NORD       1    /* Default # order definitions to find ...           */
#define NFMT       1    /* Default # format checks to find ...               */
#define NSTD       0    /* Default # standards to find ...                   */
#define NCALBLK 1000    /* Max. number of calibration blocks expected in     */
                        /*    total number of calibration periods            */
#define DIR_PERM  00755 /* Permission code for creation of new directories   */
#define CSH_PERM  00777 /* Permission code for creation of executable scripts*/
#define INFOFILE  "UVES_headsort.info"
                        /* Default name for header info. output file */
#define MACMAPFILE "UVES_headsort.macmap"
                        /* Default name for Macmap output file */
#define THARFILE  "/usr/local/uves/calib/uves/ech/cal/thargood_3.tfits"
                        /* Default path for laboratory ThAr frame */
#define ATMOFILE  "/usr/local/uves/calib/uves/ech/cal/atmoexan.tfits"
                        /* Default path for reference atmospheric line frame */
#define FLSTFILE  "/usr/local/uves/calib/uves/ech/cal/flxstd.tfits"
                        /* Default path for reference flux standards frame */

/* STRUCTURES */
typedef struct Header {
  double   mjd;                   /* Modified Julian Date of start time      */
  double   sw;                    /* Slit width                              */
  double   et;                    /* Exposure time [s]                       */
  double   rt;                    /* CCD read-out time [s]                   */
  double   tt;                    /* CCD image transfer time [s]             */
  double   mjd_e;                 /* MJD of end of exposure+read-out/transfer*/
  double   tb;                    /* Temperature in blue arm of UVES         */
  double   tr;                    /* Temperature in red  arm of UVES         */
  double   p;                     /* Barometric pressure                     */
  int      arm;                   /* UVES arm: blue (0) or red (1)           */
  int      binx;                  /* Binning factor in X                     */
  int      biny;                  /* Binning factor in Y                     */
  int      enc;                   /* Grating encoder value                   */
  char     file[LNGSTRLEN];       /* Name of file                            */
  char     abfile[LNGSTRLEN];     /* Name of file without full path          */
  char     dat[FLEN_KEYWORD];     /* Date of archived UVES file              */
  char     obj[FLEN_KEYWORD];     /* Object name                             */
  char     obj_31[FLEN_KEYWORD];  /* Object name                             */
  char     typ[FLEN_KEYWORD];     /* Type of observation                     */
  char     cwl[FLEN_KEYWORD];     /* Central wavelength for observaiton      */
  char     mod[FLEN_KEYWORD];     /* Mode of observation (i.e. dichroic?)    */
  char     lnktrg[LNGSTRLEN];     /* Target for link                         */
  char     lnkpth[LNGSTRLEN];     /* Path for link to target                 */
} header;

typedef struct SciHdr {
  int      sciind;           /* Index of sci. frame for naming links         */
  int      sciind_31;        /* Index of sci. frame for naming links in      */
                             /*    Versions <=0.31                           */
  int      bind[NCALMAX];    /* Index array for associated biases            */
  int      nb;               /* Number of associated biases                  */
  int      flind[NCALMAX];   /* Index array for associated flats             */
  int      nfl;		     /* Number of associated flats                   */
  int      wind[NCALMAX];    /* Index array for associated wave. cals.       */
  int      nw;		     /* Number of associated wavs                    */
  int      oind[NCALMAX];    /* Index array for associated order-defs        */
  int      no;		     /* Number of associated order-defs              */
  int      fmind[NCALMAX];   /* Index array for associated format checks     */
  int      nfm;		     /* Number of associated formats                 */
  int      sind[NCALMAX];    /* Index array for associated standards         */
  int      ns;		     /* Number of associated standards               */
  char     std[FLEN_KEYWORD];/* Name of standard - BUG: implies only one
				std allowed per science object exposure */
  char     arm[NAMELEN];     /* Which science arm, blue or red?              */
  char     swid[NAMELEN];    /* Slit width string to form part of master
				flatfield name */
  header   hdr;              /* The science frame's header info              */
} scihdr;

typedef struct CalPrd {
  double   nhrscal_f;   /* Number of hours for future cal. period            */
  double   nhrscal_b;   /* Number of hours for backward cal. period          */
  double   ndscal_f;    /* Number of days for future cal. period             */
  double   ndscal_b;    /* Number of days for backward cal. period           */
  int      nbias;       /* Number of biases */
  int      nflat;       /* Number of flats */
  int      nwav;        /* Number of wavelength cal.s */
  int      nfmt;        /* Number of format checks */
  int      nord;        /* Number of order definitions */
  int      nstd;        /* Number of standards */
} calprd;

typedef struct CalSrch {
  double   dmjd;        /* MJD difference between science and cal. frame     */
  int      ind;         /* Index of cal. frame in array of headers           */
} calsrch;

/* FUNCTION PROTOTYPES */
int qsort_calsrch(const void *csrch1, const void *csrch2);
int qsort_mjd(const void *hdr1, const void *hdr2);
int UVES_calsrch(header *hdrs, int nhdrs, scihdr *scis, int nscis,
		 calprd *cprd, int ncal);
int UVES_link(header *hdrs, int nhdrs, scihdr *scis, int nscis);
int UVES_list(header *hdrs, int nhdrs, scihdr *scis, int nscis);
int UVES_Macmap(header *hdrs, int nhdrs, scihdr *scis, int nscis);
int UVES_params_init(calprd *cprd);
int UVES_params_set(calprd *cprd);
int UVES_rfitshead(char *infile, header *hdr);
int UVES_wheadinfo(header *hdrs, int ndrs, char *outfile);
int UVES_wredscr(scihdr *scis, int nscis, int redstd, char *tharfile,
		 char *atmofile, char *flstfile);
