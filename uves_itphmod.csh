#!/bin/tcsh

# Script to iteratively constrain the physical model of UVES using the
# uves_cal_predict command. The command is run several times, with
# decreasing box sizes and offsets in an attempt to (i) find the right
# offsets and (ii) to determine whether a box size od 20 or 25 is
# better to use. At each iteration, the offsets are simply set to the
# median model offsets. A final box size of 20 is used if the number
# of ThAr lines used in the model drops by less than 10% when moving
# from a box size of 25 to 20. The final parameters are used in
# updating the corresponding reduction script for the specified
# exposure. The reference to the current script within the reduction
# script is then commented out. The uves_cal_predict command is also
# run one last time with the same parameters as normally used in the
# reduction scripts.
#
# NOTE: As per the above, this script will modify the reduction script
# from which it is called

# Make sure we have an exposure to work with
if ($1 == "") then
  echo "$0"": FATAL ERROR: You must specify an exposure,"
  echo "  e.g. 390_02, 564_01 etc."
  exit 0
endif

# Determine the nominal central wavelength of the exposure
@ CWL = `echo $1 | awk '{printf "%3.3d",substr($1,1,3)}'`

# If dealing with a red exposure, make sure the chip is defined
if ($CWL >= 500 && ($2 != "redl" && $2 != "redu")) then
  echo "$0"": FATAL ERROR: You must specify the chip as a second"
  echo "  argument when dealing with red exposures, i.e. redl or redu"
  exit 0
endif

# Set some initial parameters
set REDFILE = `echo "reduce_$1.cpl"`
if ( ! -w $REDFILE ) then
  echo "$0"": FATAL ERROR: Cannot write to file $REDFILE"
  exit 0
endif
set SOFFILE = `echo "reduce_$1_pred.sof"`
if ( ! -r $SOFFILE ) then
  echo "$0"": FATAL ERROR: Cannot read file $SOFFILE"
  exit 0
endif
set TRANSX = '0.1'
set TRANSY = '0.0'

# Define a temporary file and test writing to it
set TMPFILE = 'gnuplot_temp_1.dat'
/bin/rm -f $TMPFILE; touch $TMPFILE
if ( ! -w $TMPFILE ) then
  echo "$0"": FATAL ERROR: Cannot write temporary file $TMPFILE"
  exit 0
endif

# Loop over different box sizes
@ i = 0
foreach BSIZE ( 80 40 25 20 )
  @ i = $i + 1
  # Save previous results
  if ( $i > 1 ) then
    set ONUMLINES = $NUMLINES
    set OTRANSX = $TRANSX
    set OTRANSY = $TRANSY
  endif
  # Run uves_cal_predict command
  /bin/rm -f $TMPFILE
  if ($CWL < 500) then
    esorex uves_cal_predict --plotter='cat >> '$TMPFILE'' --mbox_x=$BSIZE --mbox_y=$BSIZE --trans_x=$TRANSX --trans_y=$TRANSY $SOFFILE > /dev/null
  else
    esorex uves_cal_predict --process_chip=$2 --plotter='cat >> '$TMPFILE'' --mbox_x=$BSIZE --mbox_y=$BSIZE --trans_x=$TRANSX --trans_y=$TRANSY $SOFFILE > /dev/null
  endif
  # Use UVES_itphmod to calculate median offsets
  set MEDRES = `UVES_itphmod $TMPFILE`
  # Grab results and add offsets to previous results, being careful to
  # make sure the x-offset is never exactly zero
  set NUMLINES = `echo $MEDRES | awk '{print $1}'`
  if ( $i == 1 ) then
    set TRANSX = `echo $MEDRES | awk '{if ($2==0.0) print 0.1; else printf "%.1lf",$2}'`
    set TRANSY = `echo $MEDRES | awk '{printf "%.1lf",$3}'`
    echo "$0"": Iterative results:"
    echo "Box_size  TRANSX  TRANSY"
    echo "$BSIZE         $TRANSX    $TRANSY"
  else
    set TRANSX = `echo $MEDRES | awk '{x=$2+'$TRANSX'; if (x==0.0 || x==0.1) print 0.1; else if ('$TRANSX'==0.1) printf "%.1lf",x-0.1; else printf "%.1lf",x}'`
    set TRANSY = `echo $MEDRES | awk '{printf "%.1lf",$3+'$TRANSY'}'`
    echo "$BSIZE         $TRANSX    $TRANSY"
  endif
end
/bin/rm -f $TMPFILE

# Set the final results, depending on decision about whether a box
# size of 20 or 25 is better
if (`echo "$ONUMLINES $NUMLINES" | awk '{if (($1-$2)/$1 > 0.10) print 1; else print 0}'`) then
  set FBSIZE = '25'
  set FNUMLINES = $ONUMLINES
  set FTRANSX = $OTRANSX
  set FTRANSY = $OTRANSY
else
  set FBSIZE = '20'
  set FNUMLINES = $NUMLINES
  set FTRANSX = $TRANSX
  set FTRANSY = $TRANSY
endif
echo "$0"": Final results:"
echo "Box_size  TRANSX  TRANSY"
echo "$BSIZE         $TRANSX    $TRANSY"
echo "$0"": Editing reduction script $REDFILE"
echo "  and running uves_cal_predict with final parameters"

# Define a new temporary file and test writing to it
set TMPFILE = 'itphmod_temp_1.dat'
/bin/rm -f $TMPFILE; touch $TMPFILE
if ( ! -w $TMPFILE ) then
  echo "$0"": FATAL ERROR: Cannot write temporary file $TMPFILE"
  exit 0
endif

# Run the uves_cal_predict command one last time with the final "best fit" parameters
if ($CWL < 500) then
  awk '{if ((a=index($1,"uves_itphmod.csh"))!=0) print "#"substr($0,a); else if (index($1,"esorex")!=0 && $2=="uves_cal_predict") printf "esorex uves_cal_predict --plotter=\x027\x063\x061\x074 > gnuplot_'$1'_$$.gp\x027 --mbox_x=%2d --mbox_y=%2d --trans_x=%.1lf --trans_y=%.1lf '$SOFFILE'\n",'$FBSIZE','$FBSIZE','$FTRANSX','$FTRANSY'; else print $0}' $REDFILE >> $TMPFILE
  esorex uves_cal_predict --plotter='cat > gnuplot_'$1'_$$.gp' --mbox_x=$FBSIZE --mbox_y=$FBSIZE --trans_x=$FTRANSX --trans_y=$FTRANSY $SOFFILE
else
  awk '{if ((a=index($1,"uves_itphmod.csh"))!=0 && $3=="'$2'") print "#"substr($0,a); else if (index($1,"esorex")!=0 && $2=="uves_cal_predict" && $3=="--process_chip='$2'") printf "esorex uves_cal_predict --process_chip='$2' --plotter=\x027\x063\x061\x074 > gnuplot_'$1'_$$.gp\x027 --mbox_x=%2d --mbox_y=%2d --trans_x=%.1lf --trans_y=%.1lf '$SOFFILE'\n",'$FBSIZE','$FBSIZE','$FTRANSX','$FTRANSY'; else print $0}' $REDFILE >> $TMPFILE
  esorex uves_cal_predict --process_chip=$2 --plotter='cat > gnuplot_'$1'_$$.gp' --mbox_x=$FBSIZE --mbox_y=$FBSIZE --trans_x=$FTRANSX --trans_y=$FTRANSY $SOFFILE
endif

# Replace old reduction script with new, edited one containing "best fit" parameters
/bin/mv -f $TMPFILE $REDFILE

exit
