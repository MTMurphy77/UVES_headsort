#!/bin/tcsh

# Script to copy relevant headers from reduction products to files to
# be used as input to UVES_popler.

# Make sure we have an argument to work with
if ($1 == "") then
  echo "$0"": FATAL ERROR: You must specify an exposure,"
  echo "  e.g. 390_02, 564_01 etc."
  exit 0
endif

# Create a temporary file to hold names of files to copy header to
set TMPFILE = `echo "reduce_$1_tmp.dat"`
touch $TMPFILE
if ( ! -w $TMPFILE ) then
  echo "$0"": FATAL ERROR: Cannot write temporary file $TMPFILE"
  exit 0
endif

# Determine whether we need to operate on blue or red files
@ CWL = `echo $1 | awk '{printf "%3.3d",substr($1,1,3)}'`
if ( $CWL < 500) then
  ## First copy header from extracted ThAr polynomial files to
  ## extracted ThAr spectra.

  # Set name of wavelength polynomial file to copy info from
  set WPOLFILE = 'linetable_blue.fits'
  if ( ! -r $WPOLFILE ) then
    echo "$0"": FATAL ERROR: Cannot find/read file $WPOLFILE"
    exit 0
  endif
  # List names of extracted ThAr spectra to copy header info to
  /bin/ls spectrum_blue_0_?.fits > $TMPFILE
  if ( `wc $TMPFILE | awk '{print $1}'` < 3 ) then
    echo "$0"": FATAL ERROR: Cannot find all the target files,"
    echo "  spectrum_blue_0_?.fits. There should be three."
    exit 0
  endif
  # Copy the header information from the primary HDU
  UVES_copyhead $WPOLFILE $TMPFILE

  ## Now copy over the science header from the final pipeline product
  ## to the products which have not been redispersed.

  # Set name of final science product to copy header info from
  set SCIFILE = 'resampled_science_blue.fits'
  if ( ! -r $SCIFILE ) then
    echo "$0"": FATAL ERROR: Cannot find/read file $SCIFILE"
    exit 0
  endif
  # List names of flux files to copy header info to
  /bin/ls fxb_blue.fits wfxb_blue.fits > $TMPFILE
  if ( `wc $TMPFILE | awk '{print $1}'` < 2 ) then
    echo "$0"": FATAL ERROR: Cannot find target files"
    echo "  fxb_blue.fits and/or wfxb_blue.fits"
    exit 0
  endif 
  # Copy the header information from the primary HDU
  UVES_copyhead $SCIFILE $TMPFILE
else
  ### Deal first with REDL files

  ## First copy header from extracted ThAr polynomial files to
  ## extracted ThAr spectra.

  # Set name of wavelength polynomial file to copy info from
  set WPOLFILE = 'linetable_redl.fits'
  if ( ! -r $WPOLFILE ) then
    echo "$0"": FATAL ERROR: Cannot find/read file $WPOLFILE"
    exit 0
  endif
  # List names of extracted ThAr spectra to copy header info to
  /bin/ls spectrum_redl_0_?.fits > $TMPFILE
  if ( `wc $TMPFILE | awk '{print $1}'` < 3 ) then
    echo "$0"": FATAL ERROR: Cannot find all the target files,"
    echo "  spectrum_redl_0_?.fits. There should be three."
    exit 0
  endif
  # Copy the header information from the primary HDU
  UVES_copyhead $WPOLFILE $TMPFILE

  ## Now copy over the science header from the final pipeline product
  ## to the products which have not been redispersed.

  # Set name of final science product to copy header info from
  set SCIFILE = 'resampled_science_redl.fits'
  if ( ! -r $SCIFILE ) then
    echo "$0"": FATAL ERROR: Cannot find/read file $SCIFILE"
    exit 0
  endif
  # List names of flux files to copy header info to
  /bin/ls fxb_redl.fits wfxb_redl.fits > $TMPFILE
  if ( `wc $TMPFILE | awk '{print $1}'` < 2 ) then
    echo "$0"": FATAL ERROR: Cannot find target files"
    echo "  fxb_redl.fits and/or wfxb_redl.fits"
    exit 0
  endif 
  # Copy the header information from the primary HDU
  UVES_copyhead $SCIFILE $TMPFILE

  ### Deal now with REDU files

  ## First copy header from extracted ThAr polynomial files to
  ## extracted ThAr spectra.

  # Set name of wavelength polynomial file to copy info from
  set WPOLFILE = 'linetable_redu.fits'
  if ( ! -r $WPOLFILE ) then
    echo "$0"": FATAL ERROR: Cannot find/read file $WPOLFILE"
    exit 0
  endif
  # List names of extracted ThAr spectra to copy header info to
  /bin/ls spectrum_redu_0_?.fits > $TMPFILE
  if ( `wc $TMPFILE | awk '{print $1}'` < 3 ) then
    echo "$0"": FATAL ERROR: Cannot find all the target files,"
    echo "  spectrum_redu_0_?.fits. There should be three."
    exit 0
  endif
  # Copy the header information from the primary HDU
  UVES_copyhead $WPOLFILE $TMPFILE

  ## Now copy over the science header from the final pipeline product
  ## to the products which have not been redispersed.

  # Set name of final science product to copy header info from
  set SCIFILE = 'resampled_science_redu.fits'
  if ( ! -r $SCIFILE ) then
    echo "$0"": FATAL ERROR: Cannot find/read file $SCIFILE"
    exit 0
  endif
  # List names of flux files to copy header info to
  /bin/ls fxb_redu.fits wfxb_redu.fits > $TMPFILE
  if ( `wc $TMPFILE | awk '{print $1}'` < 2 ) then
    echo "$0"": FATAL ERROR: Cannot find target files"
    echo "  fxb_redu.fits and/or wfxb_redu.fits"
    exit 0
  endif 
  # Copy the header information from the primary HDU
  UVES_copyhead $SCIFILE $TMPFILE

endif

/bin/rm -f $TMPFILE

exit
