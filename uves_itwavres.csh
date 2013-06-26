#!/bin/tcsh

# Script to run iteratively determine the best value of the wavelength
# tolerance to use in the CPL uves_cal_wavecal routine
#
# NOTE: As per the above, this script will modify the reduction script
# from which it is called.

# Make sure we have an exposure to work with
if ($1 == "") then
  echo "$0"": FATAL ERROR: You must specify an exposure,"
  echo "  e.g. 390_02, 564_01 etc."
  exit 0
endif

# Make sure the initial default tolerance is specified
if ($2 == "") then
  echo "$0"": FATAL ERROR: You must specify the default tolerance as a"
  echo "   second argument"
  exit 0
endif
set DEFTOL = `echo "$2" | awk '{printf "%6.3lf",$1}'`

# Make sure the wavelength polynomial degree is specified
if ($3 == "") then
  echo "$0"": FATAL ERROR: You must specify the wavelength polynomial"
  echo "   degree as a third argument"
  exit 0
endif
set DEGREE = `echo "$3" | awk '{printf "%d",$1}'`

# Determine the nominal central wavelength of the exposure
@ CWL = `echo $1 | awk '{printf "%3.3d",substr($1,1,3)}'`

# If dealing with a red exposure, make sure the chip is defined
if ($CWL >= 500 && ($4 != "redl" && $4 != "redu")) then
  echo "$0"": FATAL ERROR: You must specify the chip as a third"
  echo "  argument when dealing with red exposures, i.e. redl or redu"
  exit 0
endif

# If the min/max number of ThAr lines is specified, collect that information
@ NTHARMIN = 0; @ NTHARMAX = 0;
if ($4 == "nlines") then
  @ NTHARMIN = `echo "$5" | awk '{printf "%d",$1}'`; @ NTHARMAX = `echo "$6" | awk '{printf "%d",$1}'`;
else if ($5 == "nlines") then
  @ NTHARMIN = `echo "$6" | awk '{printf "%d",$1}'`; @ NTHARMAX = `echo "$7" | awk '{printf "%d",$1}'`;
endif

# Make sure reduction script exists
set REDFILE = `echo "reduce_$1.cpl"`
if ( ! -w $REDFILE ) then
  echo "$0"": FATAL ERROR: Cannot write to file $REDFILE"
  exit 0
endif

# Make sure SOF file exists
set SOFFILE = `echo "reduce_$1_wav2.sof"`
if ( ! -w $SOFFILE ) then
  echo "$0"": FATAL ERROR: Cannot read file $SOFFILE"
  exit 0
endif

# Determine the name of the line table file to use
if ($CWL < 500) then
  set LINEFILE = 'linetable_blue.fits'
else
  set LINEFILE = `echo "linetable_$4.fits"`
endif
/bin/rm -f $LINEFILE

# Run the uves_cal_wavecal script with the tolerance set well above
# that nominally required
@ ERRFLAG = 0
set TOL1 = `echo "$2" | awk '{printf "%6.3lf",2.5*$1}'`
if ($CWL < 500) then
  esorex uves_cal_wavecal --debug --extract.method=weighted --degree=$DEGREE --tolerance=$TOL1 --minlines=$NTHARMIN --maxlines=$NTHARMAX $SOFFILE > /dev/null
else
  esorex uves_cal_wavecal --debug --process_chip=$4 --extract.method=weighted --degree=$DEGREE --tolerance=$TOL1 --minlines=$NTHARMIN --maxlines=$NTHARMAX $SOFFILE > /dev/null
  endif
endif
# Make sure linetable exists
if ( ! -r $LINEFILE ) then
  echo "$0"": FATAL ERROR: Cannot find or read file $LINEFILE"
  exit 0
endif
# Run the UVES_wavres program to find the best tolerance to use in next iteration
set RES = `UVES_wavres $LINEFILE -tolm 1`
# Begin output
echo "$0"": Iterative results:"
echo "CWL Bin Tol   N   N_ord Res_ord  Res_mean F NewTol"
echo "$RES"
# Filter output to detect errors
if ($RES[9] == "0" || $RES[10] == "0.000") then
  @ ERRFLAG = 1
  set TOL4 = $DEFTOL
endif
/bin/rm -f $LINEFILE

# Run the uves_cal_wavecal script again with the tolerance returned from the first run
if ($ERRFLAG == 0) then
  set TOL2 = $RES[10]
  if ($CWL < 500) then
    esorex uves_cal_wavecal --debug --extract.method=weighted --degree=$DEGREE --tolerance=$TOL2 --minlines=$NTHARMIN --maxlines=$NTHARMAX $SOFFILE > /dev/null
  else
    esorex uves_cal_wavecal --debug --process_chip=$4 --extract.method=weighted --degree=$DEGREE --tolerance=$TOL2 --minlines=$NTHARMIN --maxlines=$NTHARMAX $SOFFILE > /dev/null
  endif
  if ( ! -r $LINEFILE ) then
    echo "$0"": FATAL ERROR: Cannot find or read file $LINEFILE"
    exit 0
  endif
  set RES = `UVES_wavres $LINEFILE -tolm 2`
  echo "$RES"
  if ($RES[9] == "0" || $RES[10] == "0.000") then
    @ ERRFLAG = 1
    set TOL4 = $DEFTOL
  endif
endif
/bin/rm -f $LINEFILE

# Run the uves_cal_wavecal script with the tolerance returned from the second run
if ($ERRFLAG == 0) then
  set TOL3 = $RES[10]
  if ($CWL < 500) then
    esorex uves_cal_wavecal --debug --extract.method=weighted --degree=$DEGREE --tolerance=$TOL3 --minlines=$NTHARMIN --maxlines=$NTHARMAX $SOFFILE > /dev/null
  else
    esorex uves_cal_wavecal --debug --process_chip=$4 --extract.method=weighted --degree=$DEGREE --tolerance=$TOL3 --minlines=$NTHARMIN --maxlines=$NTHARMAX $SOFFILE > /dev/null
  endif
  if ( ! -r $LINEFILE ) then
    echo "$0"": FATAL ERROR: Cannot find or read file $LINEFILE"
    exit 0
  endif
  set RES = `UVES_wavres $LINEFILE -tolm 3`
  echo "$RES"
  if ($RES[9] == "0" || $RES[10] == "0.000") then
    @ ERRFLAG = 1
    set TOL4 = $DEFTOL
  else
    set TOL4 = $RES[10]
  endif
endif
/bin/rm -f $LINEFILE

# Output final results and error message if necessary
echo "$0"": Final results:"
echo "CWL Bin Tol   N   N_ord Res_ord  Res_mean F NewTol"
echo "$RES"
if ($ERRFLAG == 1) then
  echo "$0"": ERROR: Problem in estimating best tolerance to use."
  echo "  Using default tolerance and adding error flag to "
  echo "  reduction script $REDFILE"
endif
echo "$0"": Editing reduction script $REDFILE"
echo "  and running uves_cal_wavecal with final tolerance"

# Define a new temporary file and test writing to it
set TMPFILE = 'itwavres_temp_1.dat'
/bin/rm -f $TMPFILE; touch $TMPFILE
if ( ! -w $TMPFILE ) then
  echo "$0"": FATAL ERROR: Cannot write temporary file $TMPFILE"
  exit 0
endif

# Run the uves_cal_wavecal command one last time with the final "best" tolerance
if ($CWL < 500) then
  awk '{if ((a=index($1,"uves_itwavres.csh"))!=0) { print "#"substr($0,a); if ('$ERRFLAG') print "# ERROR Determining best tolerance" } else if (index($1,"esorex")!=0 && $2=="uves_cal_wavecal" && $3=="--debug" && $4="--extract.method=weighted") printf "esorex uves_cal_wavecal --debug --extract.method=weighted --plotter=\x027\x063\x061\x074 > gnuplot_'$1'_$$.gp\x027 --degree='$DEGREE' --tolerance=%.3lf --minlines='$NTHARMIN' --maxlines='$NTHARMAX' '$SOFFILE'\n",'$TOL4'; else print $0}' $REDFILE >> $TMPFILE
  esorex uves_cal_wavecal --debug --extract.method=weighted --plotter='cat > gnuplot_'$1'_$$.gp' --degree=$DEGREE --tolerance=$TOL4 --minlines=$NTHARMIN --maxlines=$NTHARMAX $SOFFILE
else
  awk '{if ((a=index($1,"uves_itwavres.csh"))!=0 && $5=="'$4'") { print "#"substr($0,a); if ('$ERRFLAG') print "# ERROR Determining best tolerance for '$4'" } else if (index($1,"esorex")!=0 && $2=="uves_cal_wavecal" && $3=="--debug" && $4=="--process_chip='$4'" && $5="--extract.method=weighted") printf "esorex uves_cal_wavecal --debug --process_chip='$4' --extract.method=weighted --plotter=\x027\x063\x061\x074 > gnuplot_'$1'_$$.gp\x027 --degree='$DEGREE' --tolerance=%.3lf --minlines='$NTHARMIN' --maxlines='$NTHARMAX' '$SOFFILE'\n",'$TOL4'; else print $0}' $REDFILE >> $TMPFILE
  esorex uves_cal_wavecal --debug --process_chip=$4 --extract.method=weighted --plotter='cat > gnuplot_'$1'_$$.gp' --degree=$DEGREE --tolerance=$TOL4 --minlines=$NTHARMIN --maxlines=$NTHARMAX $SOFFILE
endif

# Replace old reduction script with new, edited one containing "best fit" parameters
/bin/mv -f $TMPFILE $REDFILE

exit
