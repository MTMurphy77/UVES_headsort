/****************************************************************************
* Write a reduction script for MIDAS pipeline for a single science
* exposure. Also write an information file for each science exposure.
****************************************************************************/

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "UVES_headsort.h"
#include "file.h"
#include "error.h"

int UVES_wredscr(scihdr *scis, int nscis, int redstd, char *tharfile,
		 char *atmofile, char *flstfile) {

  double   dcwl=0.0,tol=0.0;
  int      first=1,minlines=0,maxlines=0,degree_b=0,degree_l=0,degree_u=0;
  int      i=0,j=0;
  int      nord[2];
  char     prepfile[NAMELEN]="\0",mastfile[NAMELEN]="\0",makefile[NAMELEN]="\0";
  char     redmfile[NAMELEN]="\0",redcfile[NAMELEN]="\0";
  char     obj[NAMELEN]="\0",std[NAMELEN]="\0",cwl[FLEN_KEYWORD]="\0";
  char     bin[NAMELEN]="\0",ind[NAMELEN]="\0",ci[NAMELEN]="\0",cia[NAMELEN]="\0";
  char     arm1[NAMELEN]="\0",arm2[NAMELEN]="\0";
  char     Arm1[NAMELEN]="\0",Arm2[NAMELEN]="\0";
  char     swid[NAMELEN]="\0";
  char     *abrvthar=NULL;
  FILE     *prep_file,*mast_file,*make_file,*redm_file,*redc_file;
  extern   char *progname;

  /* Find name of laboratory ThAr file without full path */
  abrvthar=((abrvthar=strrchr(tharfile,'/'))==NULL) ? tharfile : abrvthar+1;

  for (i=0; i<nscis; i++) {

    /* Switch to local variables for convenience of coding only */
    strcpy(obj,scis[i].hdr.obj); strcpy(cwl,scis[i].hdr.cwl);
    /* BUG: Only one standard per science object exposure allowed by
       following line */
    if (scis[i].ns) strcpy(std,scis[i].std);
    sprintf(ind,"%2.2d",scis[i].sciind); sprintf(ci,"_%s_%s_",cwl,ind);
    sprintf(cia,"%s_%s",cwl,ind); strcpy(swid,scis[i].swid);
    sprintf(bin,"%dx%d",scis[i].hdr.binx,scis[i].hdr.biny);

    /* See if this is the first time this object directory has been
       encountered and, if so, write a reduction preparation script, a
       master reduction script and a Makefile containing several
       script-like commands */
    first=1;
    j=0; while (first && j<i) if (!strcmp(scis[j++].hdr.obj,obj)) first=0;

    if (first) {

      /* Open and write a reduction preparation script for MIDAS reductions */
      sprintf(prepfile,"%s/reduce_prep.prg",obj);
      if ((prep_file=faskwopen("MIDAS reduction preparation script file?",
			       prepfile,4))==NULL)
	errormsg("UVES_wredscr(): Cannot open reduction preparation\n\
\tscript file %s for writing",prepfile);
      fprintf(prep_file,"!!! %s: MIDAS Reduction Preparation by %s\n",prepfile,
	      progname);
      fprintf(prep_file,"!!! Block 00\n");
      fprintf(prep_file,"$ln -s %s thargood.fits\n",tharfile);
      fprintf(prep_file,"$ln -s %s atmoexan.fits\n",atmofile);
      fprintf(prep_file,"$ln -s %s flxstd.fits\n",flstfile);
      fprintf(prep_file,"indisk/fits thargood.fits thar.tbl\n");
      fprintf(prep_file,"!!! Block 01\n");
      fprintf(prep_file,"create/icat images.cat *.fits\n");
      fprintf(prep_file,"split/uves images.cat\n");
      fclose(prep_file);
      /* Open and write a reduction preparation script for CPL reductions */
      sprintf(prepfile,"%s/reduce_prep.cpl",obj);
      if ((prep_file=faskwopen("CPL reduction preparation script file?",
			       prepfile,4))==NULL)
	errormsg("UVES_wredscr(): Cannot open reduction preparation\n\
\tscript file %s for writing",prepfile);
      fprintf(prep_file,"# %s: CPL Reduction Preparation by %s\n",prepfile,
	      progname);
      fprintf(prep_file,"ln -s %s thargood.fits\n",tharfile);
      fprintf(prep_file,"ln -s %s atmoexan.fits\n",atmofile);
      fprintf(prep_file,"ln -s %s flxstd.fits\n",flstfile);
      fclose(prep_file);

      /* Open and write a MIDAS reduction master script */
      sprintf(mastfile,"%s/reduce_master.prg",obj);
      if ((mast_file=faskwopen("MIDAS reduction master script file?",mastfile,4))
	  ==NULL) errormsg("UVES_wredscr(): Cannot open reduction master\n\
\tscript file %s for writing",mastfile);
      fprintf(mast_file,"!!! %s: MIDAS Reduction Master by %s\n",mastfile,
	      progname);
      fprintf(mast_file,"@@ reduce_prep.prg\n");
      fprintf(mast_file,"@@ reduce_%s_%s.prg\n",cwl,ind);
      for (j=i+1; j<nscis; j++) {
	if (!strcmp(scis[j].hdr.obj,obj))
	  fprintf(mast_file,"@@ reduce_%s_%2.2d.prg\n",scis[j].hdr.cwl,
		  scis[j].sciind);
      }
      fclose(mast_file);
      
      /* Open and write a CPL reduction master script */
      sprintf(mastfile,"%s/reduce_master.cpl",obj);
      if ((mast_file=faskwopen("CPL Reduction master script file?",mastfile,4))
	  ==NULL) errormsg("UVES_wredscr(): Cannot open reduction master\n\
\tscript file %s for writing",mastfile);
      fprintf(mast_file,"# %s: MIDAS Reduction Master by %s\n",mastfile,progname);
      fprintf(mast_file,"source reduce_prep.cpl\n");
      fprintf(mast_file,"source reduce_%s_%s.cpl\n",cwl,ind);
      for (j=i+1; j<nscis; j++) {
	if (!strcmp(scis[j].hdr.obj,obj))
	  fprintf(mast_file,"source reduce_%s_%2.2d.cpl\n",scis[j].hdr.cwl,
		  scis[j].sciind);
      }
      fclose(mast_file);

      /* Open and write a Makefile containing some script-like commands */
      sprintf(makefile,"%s/Makefile",obj);
      if ((make_file=faskwopen("Makefile name?",makefile,4))==NULL)
	errormsg("UVES_wredscr(): Cannot open Makefile for writing");
      fprintf(make_file,"redun:\n\
\t/bin/rm -f gnuplot* reduce_*_*_*.sof *_blue* *_red[lu]*\n\
\t/bin/rm -f *~ *.bdf *.tbl *.tfits *.cat *.ascii *.fmt *.KEY *.plt *.lst \
disp_res*.dat *free*.dat resolution*.dat middummclear.prg dat.dat\n\
\n");
      fprintf(make_file,"clean:\n\
\t/bin/rm -f gnuplot* reduce_*_*_*.sof *_blue* *_red[lu]*\n\
\t/bin/rm -f *~ *.bdf *.tbl *.tfits *.cat *.ascii *.fmt *.KEY *.plt *.lst \
disp_res*.dat *free*.dat resolution*.dat middummclear.prg dat.dat\n\
\t/bin/rm -f atmoexan.fits thargood.fits flxstd.fits\n\
\t/bin/rm -f *.ps *fxb*.fits err*.fits thar*sci*.fits thar*sky*.fits wpol*.fits \
`ls *.dat | grep -v \"info_\"`\n\
\n");
      fprintf(make_file,"tardir:\n\tgtar -zcf ../%s.tar.gz ../%s/*.fits \
../%s/wavres_*.dat ../%s/info_*.dat ../%s/phmod_*.ps ../%s/resol_*.ps \
../%s/reduce_*.prg ../%s/reduce_*.cpl ../%s/reduce_*.sof ../%s/esorex_*.log \
../%s/Makefile\n",obj,obj,obj,obj,obj,obj,obj,obj,obj,obj,obj);
      fclose(make_file);

    }
    
    /* Define output file names and open them for writing */
    sprintf(redmfile,"%s/reduce_%s_%s.prg",obj,cwl,ind);
    if ((redm_file=faskwopen("MIDAS reduction script file?",redmfile,4))==NULL)
      errormsg("UVES_wredscr(): Cannot open reduction script file\n\
\t%s for writing",redmfile);
    sprintf(redcfile,"%s/reduce_%s_%s.cpl",obj,cwl,ind);
    if ((redc_file=faskwopen("CPL reduction script file?",redcfile,4))==NULL)
      errormsg("UVES_wredscr(): Cannot open reduction script file\n\
\t%s for writing",redcfile);

    /* Now write the script depending on whether a blue or red frame */
    /* Common stuff at top */
    fprintf(redm_file,"!!! %s: MIDAS Reduction Script by %s\n",redmfile,progname);
    fprintf(redc_file,"# %s: CPL Reduction Script by %s\n",redcfile,progname);
    if (!scis[i].nb) {
      warnmsg("UVES_wredscr(): No BIAS frames found for\n\
\t%s_sci_%s_%s.fits\n\
\tWriting empty reduction script %s.",obj,cwl,ind,redmfile);
      fprintf(redm_file,"!!! Cannot write script: No BIAS frames found\n");
      fprintf(redc_file,"# Cannot write script: No BIAS frames found\n");
    }
    else if (!scis[i].nfl) {
      warnmsg("UVES_wredscr(): No FLAT frames found for\n\
\t%s_sci_%s_%s.fits\n\
\tWriting empty reduction script %s.",obj,cwl,ind,redmfile);
      fprintf(redm_file,"!!! Cannot write script: No FLAT frames found\n");
      fprintf(redc_file,"# Cannot write script: No FLAT frames found\n");
    }
    else if (!scis[i].nw) {
      warnmsg("UVES_wredscr(): No WAV frames found for\n\
\t%s_sci_%s_%s.fits\n\
\tWriting empty reduction script %s.",obj,cwl,ind,redmfile);
      fprintf(redm_file,"!!! Cannot write script: No WAV frames found\n");
      fprintf(redc_file,"# Cannot write script: No WAV frames found\n");
    }
    else if (!scis[i].no) {
      warnmsg("UVES_wredscr(): No ORD frames found for\n\
\t%s_sci_%s_%s.fits\n\
\tWriting empty reduction script %s.",obj,cwl,ind,redmfile);
      fprintf(redm_file,"!!! Cannot write script: No ORD frames found\n");
      fprintf(redc_file,"# Cannot write script: No ORD frames found\n");
    }
    else if (!scis[i].nfm) {
      warnmsg("UVES_wredscr(): No FMT frames found for\n\
\t%s_sci_%s_%s.fits\n\
\tWriting empty reduction script %s.",obj,cwl,ind,redmfile);
      fprintf(redm_file,"!!! Cannot write script: No FMT frames found\n");
      fprintf(redc_file,"# Cannot write script: No FMT frames found\n");
    }
    else {
      /* Write reduction script but check STDs situation first */
      if (!redstd) {
	fprintf(redm_file,"!!! Not including STANDARDS on request\n");
	fprintf(redc_file,"# Not including STANDARDS on request\n");
      }
      if (redstd && !scis[i].ns) {
	warnmsg("UVES_wredscr(): No STD frames found for\n\
\t%s_sci_%s_%s.fits\n\
\tNot including standards in reduction script %s.",obj,cwl,ind,redmfile);
	fprintf(redm_file,"!!! No STD frames found.\n");
	fprintf(redc_file,"# No STD frames found.\n");
      }

      /* Decide on wavelength calibration tolerance based on central wavelength */
      if (sscanf(scis[i].hdr.cwl,"%lf",&dcwl)!=1)
	errormsg("UVES_wredscr(): Cannot convert central wavelength\n\
\t(='%s') from a string to a double for %s_sci_%s_%s.fits",scis[i].hdr.cwl,
		 obj,cwl,ind);
      if (scis[i].hdr.binx<2) tol=(dcwl<425.0) ? 0.075 : 0.065;
      else tol=(dcwl<425.0) ? 0.120 : 0.100;

      /* Must redefine number of orders that pipeline is to find for the
	 346/390 and 437 settings */
      if (dcwl<400.0) nord[0]=39;
      else if (!strcmp(scis[i].hdr.cwl,"437")) nord[0]=36;
      /* Must redefine number of orders which pipeline is to find for the
	 lower chip in the 564, 520 & 750 settings */
      else if (!strcmp(scis[i].hdr.cwl,"564")) nord[0]=27;
      else if (!strcmp(scis[i].hdr.cwl,"520") || !strcmp(scis[i].hdr.cwl,"750"))
	nord[0]=30;
      else nord[0]=0;
      /* Must redefine number of orders that pipeline is to find for the
	 upper chip in the 564 & 860 settings */
      if (!strcmp(scis[i].hdr.cwl,"564")) nord[1]=19;
      else if (!strcmp(scis[i].hdr.cwl,"860")) nord[1]=10;
      else nord[1]=0;

      /* Write MIDAS reduction script for blue arm */
      if (!strcmp(scis[i].arm,"blue")) {
	strcpy(arm1,"b\0"); strcpy(Arm1,"_\0");
	fprintf(redm_file,"!!! Block 00\n");
	fprintf(redm_file,"create/icat bias%s%s.cat bias_cal%s*_%s.bdf\n",ci,
		arm1,ci,arm1);
	fprintf(redm_file,"create/icat flat%s%s.cat flat_cal%s*_%s.bdf\n",ci,
		arm1,ci,arm1);
	fprintf(redm_file,"create/icat thar%s%s.cat thar_wav%s01_%s.bdf\n",ci,
		arm1,ci,arm1);
	fprintf(redm_file,"create/icat sci%s%s.cat %s_sci%s%s.bdf\n",ci,
		arm1,obj,ci,arm1);
	if (redstd && scis[i].ns)
	  fprintf(redm_file,"add/icat sci%s%s.cat %s_std%s01_%s.bdf\n",ci,
		  arm1,std,ci,arm1);
	fprintf(redm_file,"!!! Block 01\n");
	fprintf(redm_file,"set/echelle NBORDI=%d\n",nord[0]);
	fprintf(redm_file,"orderp/uves thar_ord%s01_%s.bdf ord%s%s.cat\n",ci,
		arm1,ci,arm1);
	fprintf(redm_file,"pred/uves thar_fmt%s01_%s.bdf thar.tbl \
pred%s%s.cat 40,40 0,0 ?\n",ci,arm1,ci,arm1);
	fprintf(redm_file,"master/uves bias%s%s.cat mbias%s%s.cat\n",ci,arm1,ci,
		arm1);
	fprintf(redm_file,"set/echelle NBORDI=0\n");
	fprintf(redm_file,"!!! Block 02\n");
	fprintf(redm_file,"create/icat ref%s%s.cat d%s%s%s.tbl ESO.PRO.CATG\n",
		ci,arm1,cwl,Arm1,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat o%s%s%s.tbl\n",
		ci,arm1,cwl,Arm1,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat b%s%s%s.tbl\n",
		ci,arm1,cwl,Arm1,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat l%sBLUE.tbl\n",ci,arm1,cwl);
	fprintf(redm_file,"add/icat ref%s%s.cat thar.tbl\n",ci,arm1);
	fprintf(redm_file,"add/icat ref%s%s.cat mbias%s%s.cat\n",ci,arm1,ci,arm1);
	fprintf(redm_file,"!!! Block 03\n");
	fprintf(redm_file,"write/descr d%s%s%s.tbl TOL %5.3lf\n",cwl,Arm1,bin,tol);
	fprintf(redm_file,"wavec/uves thar%s%s.cat wav%s%s.cat ref%s%s.cat auto\n",
		ci,arm1,ci,arm1,ci,arm1);
	fprintf(redm_file,"add/icat ref%s%s.cat l%s%s%s_*.tbl\n",
		ci,arm1,cwl,Arm1,bin);
	fprintf(redm_file,"!!! Block 04\n");
	fprintf(redm_file,"master/uves flat%s%s.cat + ref%s%s.cat \
bmeasure=median\n",ci,arm1,ci,arm1);
	fprintf(redm_file,"add/icat ref%s%s.cat mf%s_%s_%s_%s.bdf\n",
		ci,arm1,cwl,bin,swid,arm1);
	fprintf(redm_file,"!!! Block 05\n");
	fprintf(redm_file,"reduce/uves sci%s%s.cat redu_sci%s%s.cat ref%s%s.cat \
e o median\n",ci,arm1,ci,arm1,ci,arm1);
	fprintf(redm_file,"!!! Block 06\n");
	fprintf(redm_file,"outdisk/fits wfxb_%s_sci%s%s.bdf wfxb_%s_sci%s%s.fits\n",
		obj,ci,arm1,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits errw%s_sci%s%s.bdf errw%s_sci%s%s.fits\n",
		obj,ci,arm1,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits fxb_%s_sci%s%s.bdf fxb_%s_sci%s%s.fits\n",
		obj,ci,arm1,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits err_%s_sci%s%s.bdf \
err_%s_sci%s%s.fits\n",obj,ci,arm1,obj,ci,arm1);
	fprintf(redm_file,
		"outdisk/fits x2_thar_wav%s01_%s.bdf thar_%s_sci%s%s.fits\n",ci,
		arm1,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits l%s%s%s_2.tbl wpol_%s_sci%s%s.fits\n",
		cwl,Arm1,bin,obj,ci,arm1);
	if (redstd && scis[i].ns) {
	  fprintf(redm_file,
		  "outdisk/fits wfxb_%s_std%s%s.bdf wfxb_%s_std%s%s.fits\n",
		  std,ci,arm1,std,ci,arm1);
	  fprintf(redm_file,
		  "outdisk/fits errw%s_std%s%s.bdf errw%s_std%s%s.fits\n",
		  std,ci,arm1,std,ci,arm1);
	  fprintf(redm_file,
		  "outdisk/fits fxb_%s_std%s%s.bdf fxb_%s_std%s%s.fits\n",
		  std,ci,arm1,std,ci,arm1);
	  fprintf(redm_file,
		  "outdisk/fits err_%s_std%s%s.bdf err_%s_std%s%s.fits\n",
		  std,ci,arm1,std,ci,arm1);
	}
	fprintf(redm_file,"$mv -f pm_%s_EEV.ps phmod%s%s.ps\n",cwl,ci,arm1);
	fprintf(redm_file,"$mv -f resol_l%s%s%s_2.tbl.ps resol%s%s.ps\n",cwl,Arm1,
		bin,ci,arm1);
	fprintf(redm_file,"$mv -f disp_res_l%s%s%s_2.tbl.dat wavres%s%s.dat\n",cwl,
		Arm1,bin,ci,arm1);
      } else {
	/* Write MIDAS reduction script for red arm */
	strcpy(arm1,"l\0"); strcpy(Arm1,"L\0");
	strcpy(arm2,"u\0"); strcpy(Arm2,"U\0");
	fprintf(redm_file,"!!! Block 00\n");
	fprintf(redm_file,"create/icat bias%s%s.cat bias_cal%s*_%s.bdf\n",ci,
		arm1,ci,arm1);
	fprintf(redm_file,"create/icat flat%s%s.cat flat_cal%s*_%s.bdf\n",ci,
		arm1,ci,arm1);
	fprintf(redm_file,"create/icat thar%s%s.cat thar_wav%s01_%s.bdf\n",ci,
		arm1,ci,arm1);
	fprintf(redm_file,"create/icat sci%s%s.cat %s_sci%s%s.bdf\n",ci,
		arm1,obj,ci,arm1);
	if (redstd && scis[i].ns)
	  fprintf(redm_file,"add/icat sci%s%s.cat %s_std%s01_%s.bdf\n",ci,
		  arm1,std,ci,arm1);
	fprintf(redm_file,"create/icat bias%s%s.cat bias_cal%s*_%s.bdf\n",ci,
		arm2,ci,arm2);
	fprintf(redm_file,"create/icat flat%s%s.cat flat_cal%s*_%s.bdf\n",ci,
		arm2,ci,arm2);
	fprintf(redm_file,"create/icat thar%s%s.cat thar_wav%s01_%s.bdf\n",ci,
		arm2,ci,arm2);
	fprintf(redm_file,"create/icat sci%s%s.cat %s_sci%s%s.bdf\n",ci,
		arm2,obj,ci,arm2);
	if (redstd && scis[i].ns)
	  fprintf(redm_file,"add/icat sci%s%s.cat %s_std%s01_%s.bdf\n",ci,
		  arm2,std,ci,arm2);
	fprintf(redm_file,"!!! Block 01\n");
	fprintf(redm_file,"set/echelle NBORDI=%d\n",nord[0]);
	fprintf(redm_file,"orderp/uves thar_ord%s01_%s.bdf ord%s%s.cat\n",ci,
		arm1,ci,arm1);
	fprintf(redm_file,"pred/uves thar_fmt%s01_%s.bdf thar.tbl \
pred%s%s.cat 40,40 0,0 ?\n",ci,arm1,ci,arm1);
	fprintf(redm_file,"master/uves bias%s%s.cat mbias%s%s.cat\n",ci,arm1,ci,
		arm1);
	fprintf(redm_file,"set/echelle NBORDI=%d\n",nord[1]);
	fprintf(redm_file,"orderp/uves thar_ord%s01_%s.bdf ord%s%s.cat\n",ci,
		arm2,ci,arm2);
	fprintf(redm_file,"pred/uves thar_fmt%s01_%s.bdf thar.tbl \
pred%s%s.cat 40,40 0,0 ?\n",ci,arm2,ci,arm2);
	fprintf(redm_file,"master/uves bias%s%s.cat mbias%s%s.cat\n",ci,arm2,ci,
		arm2);
	fprintf(redm_file,"set/echelle NBORDI=0\n");
	fprintf(redm_file,"!!! Block 02\n");
	fprintf(redm_file,"create/icat ref%s%s.cat d%s%s%s.tbl ESO.PRO.CATG\n",
		ci,arm1,cwl,Arm1,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat o%s%s%s.tbl\n",
		ci,arm1,cwl,Arm1,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat b%s%s%s.tbl\n",
		ci,arm1,cwl,Arm1,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat l%sRED%s.tbl\n",
		ci,arm1,cwl,Arm1);
	fprintf(redm_file,"add/icat ref%s%s.cat thar.tbl\n",ci,arm1);
	fprintf(redm_file,"add/icat ref%s%s.cat mbias%s%s.cat\n",ci,arm1,ci,arm1);
	fprintf(redm_file,"create/icat ref%s%s.cat d%s%s%s.tbl ESO.PRO.CATG\n",
		ci,arm2,cwl,Arm2,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat o%s%s%s.tbl\n",
		ci,arm2,cwl,Arm2,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat b%s%s%s.tbl\n",
		ci,arm2,cwl,Arm2,bin);
	fprintf(redm_file,"add/icat ref%s%s.cat l%sRED%s.tbl\n",
		ci,arm2,cwl,Arm2);
	fprintf(redm_file,"add/icat ref%s%s.cat thar.tbl\n",ci,arm2);
	fprintf(redm_file,"add/icat ref%s%s.cat mbias%s%s.cat\n",ci,arm2,ci,arm2);
	fprintf(redm_file,"!!! Block 03\n");
	fprintf(redm_file,"write/descr d%s%s%s.tbl TOL %5.3lf\n",cwl,Arm1,bin,tol);
	fprintf(redm_file,"wavec/uves thar%s%s.cat wav%s%s.cat ref%s%s.cat auto\n",
		ci,arm1,ci,arm1,ci,arm1);
	fprintf(redm_file,"add/icat ref%s%s.cat l%s%s%s_*.tbl\n",
		ci,arm1,cwl,Arm1,bin);
	fprintf(redm_file,"write/descr d%s%s%s.tbl TOL %5.3lf\n",cwl,Arm2,bin,tol);
	fprintf(redm_file,"wavec/uves thar%s%s.cat wav%s%s.cat ref%s%s.cat auto\n",
		ci,arm2,ci,arm2,ci,arm2);
	fprintf(redm_file,"add/icat ref%s%s.cat l%s%s%s_*.tbl\n",
		ci,arm2,cwl,Arm2,bin);
	fprintf(redm_file,"!!! Block 04\n");
	fprintf(redm_file,"master/uves flat%s%s.cat + ref%s%s.cat \
bmeasure=median\n",ci,arm1,ci,arm1);
	fprintf(redm_file,"add/icat ref%s%s.cat mf%s_%s_%s_%s.bdf\n",
		ci,arm1,cwl,bin,swid,arm1);
	fprintf(redm_file,"master/uves flat%s%s.cat + ref%s%s.cat \
bmeasure=median\n",ci,arm2,ci,arm2);
	fprintf(redm_file,"add/icat ref%s%s.cat mf%s_%s_%s_%s.bdf\n",
		ci,arm2,cwl,bin,swid,arm2);
	fprintf(redm_file,"!!! Block 05\n");
	fprintf(redm_file,"reduce/uves sci%s%s.cat redu%s%s.cat ref%s%s.cat \
e o median\n",ci,arm1,ci,arm1,ci,arm1);
	fprintf(redm_file,"reduce/uves sci%s%s.cat redu%s%s.cat ref%s%s.cat \
e o median\n",ci,arm2,ci,arm2,ci,arm2);
	fprintf(redm_file,"!!! Block 06\n");
	fprintf(redm_file,"outdisk/fits wfxb_%s_sci%s%s.bdf wfxb_%s_sci%s%s.fits\n",
		obj,ci,arm1,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits errw%s_sci%s%s.bdf errw%s_sci%s%s.fits\n",
		obj,ci,arm1,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits fxb_%s_sci%s%s.bdf fxb_%s_sci%s%s.fits\n",
		obj,ci,arm1,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits err_%s_sci%s%s.bdf \
err_%s_sci%s%s.fits\n",obj,ci,arm1,obj,ci,arm1);
	fprintf(redm_file,
		"outdisk/fits x2_thar_wav%s01_%s.bdf thar_%s_sci%s%s.fits\n",ci,
		arm1,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits l%s%s%s_2.tbl wpol_%s_sci%s%s.fits\n",
		cwl,Arm1,bin,obj,ci,arm1);
	fprintf(redm_file,"outdisk/fits wfxb_%s_sci%s%s.bdf wfxb_%s_sci%s%s.fits\n",
		obj,ci,arm2,obj,ci,arm2);
	fprintf(redm_file,"outdisk/fits errw%s_sci%s%s.bdf errw%s_sci%s%s.fits\n",
		obj,ci,arm2,obj,ci,arm2);
	fprintf(redm_file,"outdisk/fits fxb_%s_sci%s%s.bdf fxb_%s_sci%s%s.fits\n",
		obj,ci,arm2,obj,ci,arm2);
	fprintf(redm_file,"outdisk/fits err_%s_sci%s%s.bdf \
err_%s_sci%s%s.fits\n",obj,ci,arm2,obj,ci,arm2);
	fprintf(redm_file,
		"outdisk/fits x2_thar_wav%s01_%s.bdf thar_%s_sci%s%s.fits\n",ci,
		arm2,obj,ci,arm2);
	fprintf(redm_file,"outdisk/fits l%s%s%s_2.tbl wpol_%s_sci%s%s.fits\n",
		cwl,Arm2,bin,obj,ci,arm2);
	if (redstd && scis[i].ns) {
	  fprintf(redm_file,
		  "outdisk/fits wfxb_%s_std%s%s.bdf wfxb_%s_std%s%s.fits\n",
		  std,ci,arm1,std,ci,arm1);
	  fprintf(redm_file,
		  "outdisk/fits errw%s_std%s%s.bdf errw%s_std%s%s.fits\n",
		  std,ci,arm1,std,ci,arm1);
	  fprintf(redm_file,
		  "outdisk/fits fxb_%s_std%s%s.bdf fxb_%s_std%s%s.fits\n",
		  std,ci,arm1,std,ci,arm1);
	  fprintf(redm_file,
		  "outdisk/fits err_%s_std%s%s.bdf err_%s_std%s%s.fits\n",
		  std,ci,arm1,std,ci,arm1);
	  fprintf(redm_file,
		  "outdisk/fits wfxb_%s_std%s%s.bdf wfxb_%s_std%s%s.fits\n",
		  std,ci,arm2,std,ci,arm2);
	  fprintf(redm_file,
		  "outdisk/fits errw%s_std%s%s.bdf errw%s_std%s%s.fits\n",
		  std,ci,arm2,std,ci,arm2);
	  fprintf(redm_file,
		  "outdisk/fits fxb_%s_std%s%s.bdf fxb_%s_std%s%s.fits\n",
		  std,ci,arm2,std,ci,arm2);
	  fprintf(redm_file,
		  "outdisk/fits err_%s_std%s%s.bdf err_%s_std%s%s.fits\n",
		  std,ci,arm2,std,ci,arm2);
	}
	fprintf(redm_file,"$mv -f pm_%s_EEV.ps phmod%s%s.ps\n",cwl,ci,arm1);
	fprintf(redm_file,"$mv -f pm_%s_MIT.ps phmod%s%s.ps\n",cwl,ci,arm2);
	fprintf(redm_file,"$mv -f resol_l%s%s%s_2.tbl.ps resol%s%s.ps\n",
		cwl,Arm1,bin,ci,arm1);
	fprintf(redm_file,"$mv -f resol_l%s%s%s_2.tbl.ps resol%s%s.ps\n",
		cwl,Arm2,bin,ci,arm2);
	fprintf(redm_file,"$mv -f disp_res_l%s%s%s_2.tbl.dat wavres%s%s.dat\n",cwl,
		Arm1,bin,ci,arm1);
	fprintf(redm_file,"$mv -f disp_res_l%s%s%s_2.tbl.dat wavres%s%s.dat\n",cwl,
		Arm2,bin,ci,arm2);
      }

      /* Write CPL reduction script for blue arm */
      if (!strcmp(scis[i].arm,"blue")) {
	/* Decide on degree of wavelength polynomial for CPL reductions
	   based on central wavelength */
	degree_b=(dcwl>380.0) ? 6 : 5;
	/* Determine the maximum and minimum number of ThAr lines to
	   be found in initial line search */
	minlines=maxlines=0;
	if (dcwl<360.0) {
	  minlines=(scis[i].hdr.binx<2) ? 3250 : 2250;
	  maxlines=(scis[i].hdr.binx<2) ? 4500 : 2750;
	} else if (dcwl<420.0) {
	  minlines=(scis[i].hdr.binx<2) ? 3500 : 2500;
	  maxlines=(scis[i].hdr.binx<2) ? 5000 : 3000;
	} else if (dcwl<450.0) {
	  ;
	} else {
	  ;
	}
	/* Now write script */
	strcpy(arm1,"b\0");
	fprintf(redc_file,"uves_makesof.csh %s\n",cia);
	fprintf(redc_file,"uves_itphmod.csh %s\n",cia);
	fprintf(redc_file,"#esorex uves_cal_predict --plotter='cat > gnuplot%s$$.gp' \
--mbox_x=40 --mbox_y=40 --trans_x=0.0 --trans_y=0.0 reduce%spred.sof\n",ci,ci);
	fprintf(redc_file,"uves_filtplot.py %s -x -p phmod%s%s.ps XDIF YDIF XMOD \
YMOD\n",cia,ci,arm1);
	/*
	fprintf(redc_file,"esorex uves_cal_orderpos --use_guess_tab=1 --norders=%d \
reduce%sord.sof\n",nord[0],ci);
	*/
	fprintf(redc_file,"esorex uves_cal_orderpos reduce%sord.sof\n",ci);
	fprintf(redc_file,"esorex uves_cal_mbias reduce%sbias.sof\n",ci);
	fprintf(redc_file,"esorex uves_cal_mflat reduce%sflat.sof\n",ci);
	fprintf(redc_file,"esorex uves_cal_wavecal --degree=%d --tolerance=%5.3lf \
--minlines=%d --maxlines=%d reduce%swav1.sof\n",degree_b,
		3.0*tol/((double)(MIN(scis[i].hdr.binx,2))),minlines,maxlines,ci);
	if (redstd && scis[i].ns)
	  fprintf(redc_file,"esorex uves_cal_response reduce%sstd.sof\n",ci);
	fprintf(redc_file,"esorex uves_obs_scired --debug reduce%ssci.sof\n",ci);
	fprintf(redc_file,"uves_itwavres.csh %s %5.3lf %d nlines %d %d\n",cia,
		tol/((double)(MIN(scis[i].hdr.binx,2))),degree_b,minlines,maxlines);
	fprintf(redc_file,"#esorex uves_cal_wavecal --debug --extract.method=weighted \
--plotter='cat > gnuplot%s$$.gp' --degree=%d --tolerance=%5.3lf --minlines=%d --maxlines=%d \
reduce%swav2.sof\n",ci,degree_b,tol/((double)(MIN(scis[i].hdr.binx,2))),minlines,maxlines,ci);
	fprintf(redc_file,"UVES_wavres linetable_blue.fits > wavres%s%s.dat\n",ci,
		arm1);
	fprintf(redc_file,"uves_filtplot.py %s -x -p resol%s%s.ps WaveC Resol X \
Ynew\n",cia,ci,arm1);
	fprintf(redc_file,"esorex uves_obs_scired --debug reduce%ssci.sof\n",ci);
	fprintf(redc_file,"uves_copyhead.csh %s\n",cia);
	fprintf(redc_file,"/bin/mv -f wfxb_blue.fits wfxb_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f errwfxb_blue.fits errw%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f fxb_blue.fits fxb_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f errfxb_blue.fits err_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,
		"/bin/mv -f spectrum_blue_0_2.fits thar_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,
		"/bin/mv -f spectrum_noise_blue_0_2.fits errthar_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f linetable_blue.fits wpol_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f esorex.log esorex_%s.log\n",cia);
	if (redstd && scis[i].ns) {
	  fprintf(redc_file,
		  "# WARNING: %s has not yet been updated to copy the pipeline\n\
#   products for standard stars to files with standardized names. Please\n\
#   contact the author if you require a certain naming convention here.",progname);
	}
	fprintf(redc_file,"/bin/rm -f reduce%s*.sof *_blue*\n",ci);
      }
      /* Write CPL reduction script for red arm */
      else {
	/* Decide on degree of wavelength polynomial for CPL reductions
	   based on central wavelength */
	degree_l=(dcwl<700.0) ? 6 : 5;
	degree_u=(dcwl<780.0) ? degree_l : 4;
	/* Determine the maximum and minimum number of ThAr lines to
	   be found in initial line search */
	minlines=maxlines=0;
	if (dcwl<590.0) {
	  ;
	} else if (dcwl<650.0) {
	  ;
	} else if (dcwl<800.0) {
	  ;
	} else {
	  ;
	}
	/* Now write script */
	strcpy(arm1,"l\0"); strcpy(arm2,"u\0");
	fprintf(redc_file,"uves_makesof.csh %s\n",cia);
	fprintf(redc_file,"uves_itphmod.csh %s redl\n",cia);
	fprintf(redc_file,"#esorex uves_cal_predict --process_chip=redl \
--plotter='cat > gnuplot%s$$.gp' --mbox_x=40 --mbox_y=40 --trans_x=0.0 --trans_y=0.0 \
reduce%spred.sof\n",ci,ci);
	fprintf(redc_file,"uves_filtplot.py %s -x -p phmod%s%s.ps XDIF YDIF XMOD \
YMOD\n",cia,ci,arm1);
	fprintf(redc_file,"uves_itphmod.csh %s redu\n",cia);
	fprintf(redc_file,"#esorex uves_cal_predict --process_chip=redu \
--plotter='cat > gnuplot%s$$.gp' --mbox_x=40 --mbox_y=40 --trans_x=0.0 --trans_y=0.0 \
reduce%spred.sof\n",ci,ci);
	fprintf(redc_file,"uves_filtplot.py %s -x -p phmod%s%s.ps XDIF YDIF XMOD \
YMOD\n",cia,ci,arm2);
	/*
	fprintf(redc_file,"esorex uves_cal_orderpos --process_chip=redl \
--use_guess_tab=1 --norders=%d reduce%sord.sof\n",nord[0],ci);
	fprintf(redc_file,"esorex uves_cal_orderpos --process_chip=redu \
--use_guess_tab=1 --norders=%d reduce%sord.sof\n",nord[1],ci);
	*/
	fprintf(redc_file,"esorex uves_cal_orderpos --process_chip=redl \
reduce%sord.sof\n",ci);
	fprintf(redc_file,"esorex uves_cal_orderpos --process_chip=redu \
reduce%sord.sof\n",ci);
	fprintf(redc_file,"esorex uves_cal_mbias reduce%sbias.sof\n",ci);
	fprintf(redc_file,"esorex uves_cal_mflat reduce%sflat.sof\n",ci);
	fprintf(redc_file,"esorex uves_cal_wavecal --process_chip=redl \
--degree=%d --tolerance=%5.3lf --minlines=%d --maxlines=%d reduce%swav1.sof\n",
		degree_l,3.0*tol/((double)(MIN(scis[i].hdr.binx,2))),minlines,maxlines,ci);
	fprintf(redc_file,"esorex uves_cal_wavecal --process_chip=redu \
--degree=%d --tolerance=%5.3lf --minlines=%d --maxlines=%d reduce%swav1.sof\n",
		degree_u,3.0*tol/((double)(MIN(scis[i].hdr.binx,2))),minlines,maxlines,ci);
	if (redstd && scis[i].ns)
	  fprintf(redc_file,"esorex uves_cal_response reduce%sstd.sof\n",ci);
	fprintf(redc_file,"esorex uves_obs_scired --debug reduce%ssci.sof\n",ci);
	fprintf(redc_file,"uves_itwavres.csh %s %5.3lf %d redl nlines %d %d\n",cia,
		tol/((double)(MIN(scis[i].hdr.binx,2))),degree_l,minlines,maxlines);
	fprintf(redc_file,"#esorex uves_cal_wavecal --debug --process_chip=redl \
--extract.method=weighted --plotter='cat > gnuplot%s$$.gp' --degree=%d \
--tolerance=%5.3lf --minlines=%d --maxlines=%d reduce%swav2.sof\n",ci,degree_l,
		tol/((double)(MIN(scis[i].hdr.binx,2))),minlines,maxlines,ci);
	fprintf(redc_file,"UVES_wavres linetable_redl.fits > wavres%s%s.dat\n",ci,
		arm1);
	fprintf(redc_file,"uves_filtplot.py %s -x -p resol%s%s.ps WaveC Resol X \
Ynew\n",cia,ci,arm1);
	fprintf(redc_file,"uves_itwavres.csh %s %5.3lf %d redu nlines %d %d\n",cia,
		tol/((double)(MIN(scis[i].hdr.binx,2))),degree_u,minlines,maxlines);
	fprintf(redc_file,"#esorex uves_cal_wavecal --debug --process_chip=redu \
--extract.method=weighted --plotter='cat > gnuplot%s$$.gp' --degree=%d \
--tolerance=%5.3lf --minlines=%d --maxlines=%d reduce%swav2.sof\n",ci,degree_u,
		tol/((double)(MIN(scis[i].hdr.binx,2))),minlines,maxlines,ci);
	fprintf(redc_file,"UVES_wavres linetable_redu.fits > wavres%s%s.dat\n",ci,
		arm2);
	fprintf(redc_file,"uves_filtplot.py %s -x -p resol%s%s.ps WaveC Resol X \
Ynew\n",cia,ci,arm2);
	fprintf(redc_file,"esorex uves_obs_scired --debug reduce%ssci.sof\n",ci);
	fprintf(redc_file,"uves_copyhead.csh %s\n",cia);
	fprintf(redc_file,"/bin/mv -f wfxb_redl.fits wfxb_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f errwfxb_redl.fits errw%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f fxb_redl.fits fxb_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f errfxb_redl.fits err_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,
		"/bin/mv -f spectrum_redl_0_2.fits thar_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,
		"/bin/mv -f spectrum_noise_redl_0_2.fits errthar_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f linetable_redl.fits wpol_%s_sci%s%s.fits\n",
		obj,ci,arm1);
	fprintf(redc_file,"/bin/mv -f wfxb_redu.fits wfxb_%s_sci%s%s.fits\n",
		obj,ci,arm2);
	fprintf(redc_file,"/bin/mv -f errwfxb_redu.fits errw%s_sci%s%s.fits\n",
		obj,ci,arm2);
	fprintf(redc_file,"/bin/mv -f fxb_redu.fits fxb_%s_sci%s%s.fits\n",
		obj,ci,arm2);
	fprintf(redc_file,"/bin/mv -f errfxb_redu.fits err_%s_sci%s%s.fits\n",
		obj,ci,arm2);
	fprintf(redc_file,
		"/bin/mv -f spectrum_redu_0_2.fits thar_%s_sci%s%s.fits\n",
		obj,ci,arm2);
	fprintf(redc_file,
		"/bin/mv -f spectrum_noise_redu_0_2.fits errthar_%s_sci%s%s.fits\n",
		obj,ci,arm2);
	fprintf(redc_file,"/bin/mv -f linetable_redu.fits wpol_%s_sci%s%s.fits\n",
		obj,ci,arm2);
	fprintf(redc_file,"/bin/mv -f esorex.log esorex_%s.log\n",cia);
	if (redstd && scis[i].ns) {
	  fprintf(redc_file,
		  "# WARNING: %s has not yet been updated to copy the pipeline\n\
#   products for standard stars to files with standardized names. Please\n\
#   contact the author if you require a certain naming convention here.",progname);
	}
	fprintf(redc_file,"/bin/rm -f reduce%s*.sof *_red[lu]*\n",ci);
      }
    }

    /* Close files */
    fclose(redm_file); fclose(redc_file);

  }
  
  return 1;
}
