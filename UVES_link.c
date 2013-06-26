/****************************************************************************
* Create relevant object directories and symbolic links within them
* which point to the relevant science and calibration frames. Create
* an info file for each science exposure which documents the links
* made and the relevant calibration files to be used. Also create a
* master Set Of Frames file for each science exposure with enough
* information included to allow a user to easily construct SOF files
* for invidivual reduction steps.
****************************************************************************/

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "UVES_headsort.h"
#include "file.h"
#include "error.h"

int UVES_link(header *hdrs, int nhdrs, scihdr *scis, int nscis) {

  double temp=0.0;
  int    first=0;
  int    i=0,j=0;
  char   sciname[LNGSTRLEN]="\0",calname[LNGSTRLEN]="\0";
  char   filedesc[NAMELEN]="\0",reddesc[LNGSTRLEN]="\0";
  char   infofile[NAMELEN]="\0",soffile[NAMELEN]="\0";
  char   callnkpth[LNGSTRLEN]="\0",callnktrg[LNGSTRLEN]="\0";
  FILE   *info_file,*sof_file;

  for (i=0; i<nscis; i++) {

    /* See if this is the first time this object has been encountered */
    first=1;
    j=0; while (first && j<i)
      if (!strcmp(scis[j++].hdr.obj,scis[i].hdr.obj)) first=0;

    /* Create (or check for) object directory */
    if (!isdir(scis[i].hdr.obj) && first) {
      if (mkdir(scis[i].hdr.obj,DIR_PERM))
	errormsg("UVES_link(): Cannot create directory %s.\n\
\tCheck permission settings?",scis[i].hdr.obj);
    }
    else if (first)
      errormsg("UVES_link(): Object directory %s\n\
\talready exists!",scis[i].hdr.obj);

    /* Define info file name and open it for writing */
    sprintf(infofile,"%s/info_%s_%2.2d.dat",scis[i].hdr.obj,scis[i].hdr.cwl,
	    scis[i].sciind);
    if ((info_file=faskwopen("Science exposure information file?",infofile,4))
	==NULL) errormsg("UVES_link(): Cannot open science exposure\n\
\tinformation file\n\t%s for writing",infofile);

    /* Define SOF file name and open it for writing */
    sprintf(soffile,"%s/reduce_%s_%2.2d.sof",scis[i].hdr.obj,scis[i].hdr.cwl,
	    scis[i].sciind);
    if ((sof_file=faskwopen("Science exposure SOF file?",soffile,4))==NULL)
      errormsg("UVES_link(): Cannot open science exposure\n\
\tSOF file\n\t%s for writing",soffile);

    /* Enter details of science exposure into info file */
    sprintf(sciname,"%s_%s_%s_%2.2d.fits",scis[i].hdr.obj,scis[i].hdr.typ,
	    scis[i].hdr.cwl,scis[i].sciind);
    temp=(!strcmp(scis[i].arm,"blue")) ? scis[i].hdr.tb : scis[i].hdr.tr;
    fprintf(info_file,"SCI  %-50s %33s %4.1lf %8.2lf %.8lf %4.1lf %5.1lf %d\n",
	    sciname,scis[i].hdr.abfile,scis[i].hdr.sw,scis[i].hdr.et,scis[i].hdr.mjd,
	    temp,scis[i].hdr.p,scis[i].hdr.enc);

    /* Enter details of science exposure into SOF file */
    if (!strcmp(scis[i].arm,"blue")) sprintf(filedesc,"SCIENCE_BLUE");
    else  sprintf(filedesc,"SCIENCE_RED");
    sprintf(reddesc,"sci");
    fprintf(sof_file,"%s %s %s\n",sciname,filedesc,reddesc);

    /* Determine science frame index string and make appropriate symlink */
    sprintf(scis[i].hdr.lnkpth,"%s/%s",scis[i].hdr.obj,sciname);
    if (access(scis[i].hdr.lnkpth,F_OK)) {
	sprintf(scis[i].hdr.lnktrg,"%s",scis[i].hdr.file);
	if (symlink(scis[i].hdr.lnktrg,scis[i].hdr.lnkpth))
	  errormsg("UVES_link(): Cannot create symlink %s\n\
\tto file %s\n\
\tCheck permission settings?",scis[i].hdr.lnkpth,scis[i].hdr.lnktrg);
    }
    else errormsg("UVES_link(): Symlink %s\n\
\tin directory %s already exists. Solution unknown!",sciname,scis[i].hdr.obj);

    /* Generate links to stds */
    for (j=0; j<scis[i].ns; j++) {
      sprintf(calname,"%s_%s_%s_%2.2d_%2.2d.fits",hdrs[scis[i].sind[j]].obj,
	      hdrs[scis[i].sind[j]].typ,scis[i].hdr.cwl,scis[i].sciind,j+1);
      sprintf(callnkpth,"%s/%s",scis[i].hdr.obj,calname);
      sprintf(callnktrg,"%s",hdrs[scis[i].sind[j]].file);
      if (symlink(callnktrg,callnkpth))
	errormsg("Cannot create symlink %s\n\tto file %s.\n\
\tCheck permission settings?",callnkpth,callnktrg);
      temp=(!strcmp(scis[i].arm,"blue")) ? hdrs[scis[i].sind[j]].tb :
	hdrs[scis[i].sind[j]].tr;
      fprintf(info_file,
	      "STD  %-50s %33s %4.1lf %8.2lf %.8lf %4.1lf %5.1lf %d\n",calname,
	      hdrs[scis[i].sind[j]].abfile,hdrs[scis[i].sind[j]].sw,
	      hdrs[scis[i].sind[j]].et,hdrs[scis[i].sind[j]].mjd,temp,
	      hdrs[scis[i].sind[j]].p,hdrs[scis[i].sind[j]].enc);
      if (!strcmp(scis[i].arm,"blue")) sprintf(filedesc,"STANDARD_BLUE");
      else  sprintf(filedesc,"STANDARD_RED");
      sprintf(reddesc,"std");
      fprintf(sof_file,"%s %s %s\n",calname,filedesc,reddesc);
    }

    /* Generate links to wavs */
    for (j=0; j<scis[i].nw; j++) {
      sprintf(calname,"%s_%s_%s_%2.2d_%2.2d.fits",hdrs[scis[i].wind[j]].obj,
	      hdrs[scis[i].wind[j]].typ,scis[i].hdr.cwl,scis[i].sciind,j+1);
      sprintf(callnkpth,"%s/%s",scis[i].hdr.obj,calname);
      sprintf(callnktrg,"%s",hdrs[scis[i].wind[j]].file);
      if (symlink(callnktrg,callnkpth))
	errormsg("Cannot create symlink %s\n\tto file %s.\n\
\tCheck permission settings?",callnkpth,callnktrg);
      temp=(!strcmp(scis[i].arm,"blue")) ? hdrs[scis[i].wind[j]].tb :
	hdrs[scis[i].wind[j]].tr;
      fprintf(info_file,
	      "WAV  %-50s %33s %4.1lf %8.2lf %.8lf %4.1lf %5.1lf %d\n",calname,
	      hdrs[scis[i].wind[j]].abfile,hdrs[scis[i].wind[j]].sw,
	      hdrs[scis[i].wind[j]].et,hdrs[scis[i].wind[j]].mjd,temp,
	      hdrs[scis[i].wind[j]].p,hdrs[scis[i].wind[j]].enc);
      if (!strcmp(scis[i].arm,"blue")) sprintf(filedesc,"ARC_LAMP_BLUE");
      else  sprintf(filedesc,"ARC_LAMP_RED");
      sprintf(reddesc,"wav1,wav2");
      fprintf(sof_file,"%s %s %s\n",calname,filedesc,reddesc);
    }

    /* Generate links to ords */
    for (j=0; j<scis[i].no; j++) {
      sprintf(calname,"%s_%s_%s_%2.2d_%2.2d.fits",hdrs[scis[i].oind[j]].obj,
	      hdrs[scis[i].oind[j]].typ,scis[i].hdr.cwl,scis[i].sciind,j+1);
      sprintf(callnkpth,"%s/%s",scis[i].hdr.obj,calname);
      sprintf(callnktrg,"%s",hdrs[scis[i].oind[j]].file);
      if (symlink(callnktrg,callnkpth))
	errormsg("Cannot create symlink %s\n\tto file %s.\n\
\tCheck permission settings?",callnkpth,callnktrg);
      temp=(!strcmp(scis[i].arm,"blue")) ? hdrs[scis[i].oind[j]].tb :
	hdrs[scis[i].oind[j]].tr;
      fprintf(info_file,
	      "ORD  %-50s %33s %4.1lf %8.2lf %.8lf %4.1lf %5.1lf %d\n",calname,
	      hdrs[scis[i].oind[j]].abfile,hdrs[scis[i].oind[j]].sw,
	      hdrs[scis[i].oind[j]].et,hdrs[scis[i].oind[j]].mjd,temp,
	      hdrs[scis[i].oind[j]].p,hdrs[scis[i].oind[j]].enc);
      if (!strcmp(scis[i].arm,"blue")) sprintf(filedesc,"ORDER_FLAT_BLUE");
      else  sprintf(filedesc,"ORDER_FLAT_RED");
      sprintf(reddesc,"ord");
      fprintf(sof_file,"%s %s %s\n",calname,filedesc,reddesc);
    }

    /* Generate links to fmts */
    for (j=0; j<scis[i].nfm; j++) {
      sprintf(calname,"%s_%s_%s_%2.2d_%2.2d.fits",hdrs[scis[i].fmind[j]].obj,
	      hdrs[scis[i].fmind[j]].typ,scis[i].hdr.cwl,scis[i].sciind,j+1);
      sprintf(callnkpth,"%s/%s",scis[i].hdr.obj,calname);
      sprintf(callnktrg,"%s",hdrs[scis[i].fmind[j]].file);
      if (symlink(callnktrg,callnkpth))
	errormsg("Cannot create symlink %s\n\tto file %s.\n\
\tCheck permission settings?",callnkpth,callnktrg);
      temp=(!strcmp(scis[i].arm,"blue")) ? hdrs[scis[i].fmind[j]].tb :
	hdrs[scis[i].fmind[j]].tr;
      fprintf(info_file,
	      "FMT  %-50s %33s %4.1lf %8.2lf %.8lf %4.1lf %5.1lf %d\n",calname,
	      hdrs[scis[i].fmind[j]].abfile,hdrs[scis[i].fmind[j]].sw,
	      hdrs[scis[i].fmind[j]].et,hdrs[scis[i].fmind[j]].mjd,temp,
	      hdrs[scis[i].fmind[j]].p,hdrs[scis[i].fmind[j]].enc);
      if (!strcmp(scis[i].arm,"blue")) sprintf(filedesc,"ARC_LAMP_FORM_BLUE");
      else  sprintf(filedesc,"ARC_LAMP_FORM_RED");
      sprintf(reddesc,"pred");
      fprintf(sof_file,"%s %s %s\n",calname,filedesc,reddesc);
    }

    /* Generate links to flats */
    for (j=0; j<scis[i].nfl; j++) {
      sprintf(calname,"%s_%s_%s_%2.2d_%2.2d.fits",hdrs[scis[i].flind[j]].obj,
	      hdrs[scis[i].flind[j]].typ,scis[i].hdr.cwl,scis[i].sciind,j+1);
      sprintf(callnkpth,"%s/%s",scis[i].hdr.obj,calname);
      sprintf(callnktrg,"%s",hdrs[scis[i].flind[j]].file);
      if (symlink(callnktrg,callnkpth))
	errormsg("Cannot create symlink %s\n\
\tto file %s.\n\
\tCheck permission settings?",callnkpth,callnktrg);
      temp=(!strcmp(scis[i].arm,"blue")) ? hdrs[scis[i].flind[j]].tb :
	hdrs[scis[i].flind[j]].tr;
      fprintf(info_file,
	      "FLAT %-50s %33s %4.1lf %8.2lf %.8lf %4.1lf %5.1lf %d\n",calname,
	      hdrs[scis[i].flind[j]].abfile,hdrs[scis[i].flind[j]].sw,
	      hdrs[scis[i].flind[j]].et,hdrs[scis[i].flind[j]].mjd,temp,
	      hdrs[scis[i].flind[j]].p,hdrs[scis[i].flind[j]].enc);
      if (!strcmp(scis[i].arm,"blue")) sprintf(filedesc,"FLAT_BLUE");
      else  sprintf(filedesc,"FLAT_RED");
      sprintf(reddesc,"flat");
      fprintf(sof_file,"%s %s %s\n",calname,filedesc,reddesc);
    }

    /* Generate links to biases */
    for (j=0; j<scis[i].nb; j++) {
      sprintf(calname,"%s_%s_%s_%2.2d_%2.2d.fits",hdrs[scis[i].bind[j]].obj,
	      hdrs[scis[i].bind[j]].typ,scis[i].hdr.cwl,scis[i].sciind,j+1);
      sprintf(callnkpth,"%s/%s",scis[i].hdr.obj,calname);
      sprintf(callnktrg,"%s",hdrs[scis[i].bind[j]].file);
      if (symlink(callnktrg,callnkpth))
	errormsg("Cannot create symlink %s\n\tto file %s.\n\
\tCheck permission settings?",callnkpth,callnktrg);
      temp=(!strcmp(scis[i].arm,"blue")) ? hdrs[scis[i].bind[j]].tb :
	hdrs[scis[i].bind[j]].tr;
      fprintf(info_file,
	      "BIAS %-50s %33s %4.1lf %8.2lf %.8lf %4.1lf %5.1lf %d\n",calname,
	      hdrs[scis[i].bind[j]].abfile,hdrs[scis[i].bind[j]].sw,
	      hdrs[scis[i].bind[j]].et,hdrs[scis[i].bind[j]].mjd,temp,
	      hdrs[scis[i].bind[j]].p,hdrs[scis[i].bind[j]].enc);
      if (!strcmp(scis[i].arm,"blue")) sprintf(filedesc,"BIAS_BLUE");
      else  sprintf(filedesc,"BIAS_RED");
      sprintf(reddesc,"bias");
      fprintf(sof_file,"%s %s %s\n",calname,filedesc,reddesc);
    }

    /* Complete SOF file with files expected to be produced in
       different reduction steps */
    if (!strcmp(scis[i].arm,"blue")) fprintf(sof_file,"\
masterbias_blue.fits MASTER_BIAS_BLUE flat,wav1,wav2,std,sci\n\
masterflat_blue.fits MASTER_FLAT_BLUE wav1,wav2,std,sci\n\
orderguesstable_blue.fits ORDER_GUESS_TAB_BLUE ord\n\
ordertable_blue.fits ORDER_TABLE_BLUE flat,wav1,wav2,std,sci\n\
lineguesstable_blue.fits LINE_GUESS_TAB_BLUE wav1,wav2\n\
weights_blue.fits WEIGHTS_BLUE wav2\n\
linetable_blue.fits LINE_TABLE_BLUE std,sci\n");
    else fprintf(sof_file,"\
masterbias_redl.fits MASTER_BIAS_REDL flat,wav1,wav2,std,sci\n\
masterbias_redu.fits MASTER_BIAS_REDU flat,wav1,wav2,std,sci\n\
masterflat_redl.fits MASTER_FLAT_REDL wav1,wav2,std,sci\n\
masterflat_redu.fits MASTER_FLAT_REDU wav1,wav2,std,sci\n\
orderguesstable_redl.fits ORDER_GUESS_TAB_REDL ord\n\
orderguesstable_redu.fits ORDER_GUESS_TAB_REDU ord\n\
ordertable_redl.fits ORDER_TABLE_REDL flat,wav1,wav2,std,sci\n\
ordertable_redu.fits ORDER_TABLE_REDU flat,wav1,wav2,std,sci\n\
lineguesstable_redl.fits LINE_GUESS_TAB_REDL wav1,wav2\n\
lineguesstable_redu.fits LINE_GUESS_TAB_REDU wav1,wav2\n\
weights_redl.fits WEIGHTS_REDL wav2\n\
weights_redu.fits WEIGHTS_REDU wav2\n\
linetable_redl.fits LINE_TABLE_REDL std,sci\n\
linetable_redu.fits LINE_TABLE_REDU std,sci\n");
    fprintf(sof_file,"\
thargood.fits LINE_REFER_TABLE pred,wav1,wav2\n\
atmoexan.fits EXTCOEFF_TABLE std\n\
flxstd.fits FLUX_STD_TABLE std\n");

    /* Close files */
    fclose(info_file); fclose(sof_file);

  }

  return 1;

}
