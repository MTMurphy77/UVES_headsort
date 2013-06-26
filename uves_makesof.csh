#!/bin/tcsh

# Script to produce SOF files for the reduction of a particular
# exposure, specified by the single argument, from a master SOF
# file. The master SOF file should have been produced by UVES_headsort

# Make sure we have an argument to work with
if ($1 == "") then
  echo "$0"": FATAL ERROR: You must specify an exposure,"
  echo "  e.g. 390_02, 564_01 etc."
  exit 0
endif

# Make sure master SOF files exists
set BASEFILE = `echo "reduce_$1.sof"`
if ( ! -r $BASEFILE ) then
  echo "$0"": FATAL ERROR: Cannot find/read file $BASEFILE"
  exit 0
endif

# Loop through SOF files to create and issue warnings if we have to
# overwrite existing oones
foreach SOFTYPE ( pred ord bias flat wav1 wav2 std sci )
  set SOFFILE = `echo "reduce_$1_$SOFTYPE.sof"`
  if ( -e $SOFFILE ) then
    if ( -w $SOFFILE ) then
      echo "$0"": Warning: Will overwrite existing file $SOFFILE"
    else
      echo "$0"": FATAL ERROR: Cannot overwrite existing file $SOFFILE"
      exit 0
    endif
  endif
end

# Loop through SOF files to create and create them
foreach SOFTYPE ( pred ord bias flat wav1 wav2 std sci )
  set SOFFILE = `echo "reduce_$1_$SOFTYPE.sof"`
  awk '{if (index($3,"'$SOFTYPE'")) print $1,$2}' $BASEFILE > $SOFFILE
end

exit
