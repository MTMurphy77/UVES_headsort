#!/bin/tcsh

# Script to read important information from existing files containing
# the results from the wavelength calibration step.

# Check the argument, if it exists
if ( $1 != "" ) then
  @ CWL = `echo $1 | awk '{printf "%3.3d",substr($1,1,3)}'`
endif

# Check whether any wpol fits files exist
@ WAVEXIST = 1
if ( `( /bin/ls wpol_*fits > /dev/null ) |& grep "No match"` != "") then
  @ WAVEXIST = 0
endif

# Check whether any temporary wpol files exist
@ DISEXIST = 1
if ( `( /bin/ls linetable_*.fits > /dev/null ) |& grep "No match"` != "") then
  @ DISEXIST = 0
endif

# Check the reduction scripts for errors at the wavelength calibration step
echo -n "Checking for errors reported in reduction scripts ..."
@ ERRFLAG = 0
@ i = 0
foreach REDFILE (`/bin/ls reduce_[345678]*.cpl`)
  @ i = $i + 1
  set GREPOUT = `grep "ERROR Determining best tolerance" $REDFILE`
  if (`echo "$GREPOUT"` != "") then
    @ ERRFLAG = 1
    if ($i == 1) then
      echo ""
    endif
    echo "$REDFILE"": $GREPOUT"
  endif
end
if ($ERRFLAG == 1) then
  echo "$0"": FATAL ERROR: Errors reported in above reduction scripts"
  exit 0
else
  echo " none."
endif

# Loop over existing wpol files first
if ( $WAVEXIST == 1 ) then
  foreach i (`/bin/ls wpol_*.fits`)
    set WAVRES = `UVES_wavres $i -verb 0`
    set SETNAME = `echo "$i" | awk '{a=index($1,"sci_"); b=a+4; c=index($1,".fits"); n=c-b; print substr($1,b,n)}'`
    echo "$SETNAME $WAVRES" | awk '{printf "%-8s  %1dx%1d  TOL = %6.3lf  n_av = %6.2lf  rms = %6.2lf m/s\n",$1,$3,$4,$5,$7,$9}'
  end
else
  echo "No wpol_*.fits files present"
endif

# Loop over existing temporary wpol files
if ( $DISEXIST ) then
  foreach i (`/bin/ls linetable_*.fits`)
    set WAVRES = `UVES_wavres $i -verb 0`
    set SETNAME = `echo "$i" | awk '{a=index($1,"linetable_"); b=a+10; c=index($1,".fits"); n=c-b; print substr($1,b,n)}'`
    echo "$SETNAME $WAVRES" | awk '{printf "%-8s  %1dx%1d  TOL = %6.3lf  n_av = %6.2lf  rms = %6.2lf m/s\n",$1,$3,$4,$5,$7,$9}'
  end
else
  echo "No linetable_*.fits files present"
endif
