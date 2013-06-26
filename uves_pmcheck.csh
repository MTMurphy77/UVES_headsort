#!/bin/tcsh

# Script to display the plots of the physical model constraints and
# print the box sizes and shifts to the screen. The user may enter a
# single argument which specifies the nominal central wavelength of
# the exposures which they want to check.

# Check whether there's an argument given
if ($1 != "") then
  set WAVSET = $1'*'
else
  set WAVSET = '*'
endif

# See if there's any phmod_*.ps files to display
@ WAVEXIST = 1
if ( `( /bin/ls phmod_$WAVSET.ps > /dev/null ) |& grep "No match"` != "") then
  @ WAVEXIST = 0
endif

# Loop over phmod_*.ps files
echo "Exposure   Box     Shift"
if ( $WAVEXIST == 1 ) then
  foreach i (`/bin/ls phmod_$WAVSET.ps`)
    set SETNAME = `echo "$i" | awk '{a=index($1,"phmod_"); b=a+6; c=index($1,".ps"); n=c-b; print substr($1,b,n)}'`
    set ARM = `echo "$SETNAME" | awk '{print substr($1,8,1)}'`
    set REDNAME = `echo "$SETNAME" | awk '{print "reduce_"substr($1,1,6)".cpl"}'`
    if ($ARM == "b") then
      set PARSET = `grep "esorex uves_cal_predict" $REDNAME | awk '{a=1+index($6,"="); b=1+index($7,"="); c=1+index($8,"="); d=1+index($9,"="); printf "%2d %2d %4.1lf %4.1lf",substr($6,a),substr($7,b),substr($8,c),substr($9,d)}'`
    else if ($ARM == "l" || $ARM == "u") then
      set PARSET = `grep "esorex uves_cal_predict --process_chip=red$ARM" $REDNAME | awk '{a=1+index($7,"="); b=1+index($8,"="); c=1+index($9,"="); d=1+index($10,"="); printf "%2d %2d %.1lf %.1lf",substr($7,a),substr($8,b),substr($9,c),substr($10,d)}'`
    endif
    echo "$SETNAME $PARSET" | awk '{printf "%-8s  %2d %2d  %4.1lf %4.1lf\n",$1,$2,$3,$4,$5}'
    gv --noantialias --orientation=landscape $i
  end
else
  echo "No phmod_$WAVSET.ps files present"
endif
