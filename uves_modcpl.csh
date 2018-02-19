#!/bin/tcsh

# Script to modify the CPL reduction scripts to:
# -tol: Change the wavelength calibration tolerance
# -min: Change the minimum number of ThAr lines to search for
# -max: Change the maximum number of ThAr lines to search for
# -deg: Change the polynomial degree for the wavelength calibration
#
# Usage:
# uves_modcpl.csh [OPTIONS] [UVES setting & optional chip] [optional exposure numbers]
# e.g. uves_modcpl.csh -min 2500 -max 3000 -deg 6 390
# e.g. uves_modcpl.csh -tol 0.05 -deg 6 580L "0[2-5]"
#
# Notes:
# This script renames the existing reduce_*.cpl scripts to
# reduce_*.old.cpl, silently over-writing any existing
# reduce_*.old.cpl files.

# Set program name
set PROG = $0:t:r

# Need at least one argument, i.e. the reduction directory name
if ( ${#argv} == 0 ) then
  echo "${PROG}: ERROR: No arguments passed"
  echo "${PROG}: Usage:"
  echo "  ${0:t} [OPTIONS] [UVES setting & optional chip] [optional exposure numbers]"
  echo "  e.g. ${0:t} -min 2500 -max 3000 -deg 6 390"
  echo "  e.g. ${0:t} -tol 0.05 -deg 6 580L "\""0[2-5]"\"
  exit
endif

# Check options
@ SKIP = 0; @ MODTOL = 0; @ MODDEG = 0; @ MODMIN = 0; @ MODMAX = 0;
set SETANDCHIP = ""
foreach i ( `seq 1 ${#argv}` )
  if ( $SKIP == 1 ) then
     @ SKIP = 0
  else if ( X${argv[$i]} == X"-tol" ) then
    @ MODTOL = 1; @ j = $i + 1
    if ( $j > ${#argv} ) then
      echo "${PROG}: ERROR: -tol option passed with no value"
      exit
    else
      set TOL = ${argv[$j]}
      @ SKIP = 1
    endif
  else if ( X${argv[$i]} == X"-deg" ) then
    @ MODDEG = 1; @ j = $i + 1
    if ( $j > ${#argv} ) then
      echo "${PROG}: ERROR: -deg option passed with no value"
      exit
    else
      set DEG = ${argv[$j]}
      @ SKIP = 1
    endif
  else if ( X${argv[$i]} == X"-min" ) then
    @ MODMIN = 1; @ j = $i + 1
    if ( $j > ${#argv} ) then
      echo "${PROG}: ERROR: -min option passed with no value"
      exit
    else
      set MIN = ${argv[$j]}
      @ SKIP = 1
    endif
  else if ( X${argv[$i]} == X"-max" ) then
    @ MODMAX = 1; @ j = $i + 1
    if ( $j > ${#argv} ) then
      echo "${PROG}: ERROR: -max option passed with no value"
      exit
    else
      set MAX = ${argv[$j]}
      @ SKIP = 1
    endif
  else
    set SETANDCHIP = ${argv[$i]}
    @ j = $i + 1
    set EXPOSURES = "*"
    if ( $j == ${#argv} ) then
      set EXPOSURES = "${argv[$j]}"
    else if ( $j < ${#argv} ) then
      @ j = $j + 1
      echo "${PROG}: ERROR: Do not understand argument ${argv[$j]}"
      exit
    endif
    @ SKIP = 1
  endif
end
if ( ${SETANDCHIP} == "" ) then
  echo "${PROG}: ERROR: Setting not specified."
  echo "  Run with no arguments for usage"
  exit
endif

# Split up the SETANDCHIP argument into SETTING and CHIP
set SETTING = `echo ${SETANDCHIP} | cut -c 1-3`
@ MODCHIP = 0
set CHIP = `echo ${SETANDCHIP} | cut -c 4`
if ( ${CHIP} != "" ) then
  @ MODCHIP = 1
  if ( ${CHIP} == "L" || ${CHIP} == "l" ) then
    set CHIP = "redl"
  else if ( ${CHIP} == "U" || ${CHIP} == "u" ) then
    set CHIP = "redu"
  else if ( ${CHIP} == "B" || ${CHIP} == "b" ) then
    @ MODCHIP = 0
  else
    echo "${PROG}: ERROR: Do not recognise chip '${CHIP}'"
    exit
  endif
endif

# Loop over reduction scripts satisfying the specified argument
@ i = 0
echo "${PROG}: Modifying reduction scripts reduce_${SETTING}_${EXPOSURES}.cpl"
echo "${PROG}: Moving old reduction scripts to reduce_${SETTING}_${EXPOSURES}.old.cpl"
foreach REDFILE ( `\ls reduce_${SETTING}_${EXPOSURES}.cpl | grep -v ".old.cpl"` )
  @ i = $i + 1
  echo "${REDFILE}: "
  # Check that reduction file exists
  if ( -r ${REDFILE} ) then
    # Find relevant line to replace in file
    if ( $MODCHIP == 0 ) then
      # Check that there's only one line to edit
      @ NWAVCALLINE = `grep -n "^esorex uves_cal_wavecal --debug" ${REDFILE} | wc -l`
      if ( $NWAVCALLINE != 1 ) then
        echo "${PROG}: ERROR: When processing reduction script ${REDFILE} more"
        echo "  than 1 final wavelength calibration command was found. If"
        echo "  modifying red-arm scripts, specify which chip in setting, e.g. 580L"
	exit
      endif
      @ WAVCALLINE = `grep -n "^esorex uves_cal_wavecal --debug" ${REDFILE} | awk -F ':' '{print $1}'`
    else
      @ WAVCALLINE = `grep -n "^esorex uves_cal_wavecal --debug --process_chip=${CHIP}" ${REDFILE} | awk -F ':' '{print $1}'`
    endif
    # Determine current values of degree, tolerance, minlines and maxlines
    set ODEG = `awk -F '--degree=' 'NR == '$WAVCALLINE' {printf "%d\n",$2}' ${REDFILE}`
    set OTOL = `awk -F '--tolerance=' 'NR == '$WAVCALLINE' {printf "%.3lf\n",$2}' ${REDFILE}`
    set OMIN = `awk -F '--minlines=' 'NR == '$WAVCALLINE' {printf "%d\n",$2}' ${REDFILE}`
    set OMAX = `awk -F '--maxlines=' 'NR == '$WAVCALLINE' {printf "%d\n",$2}' ${REDFILE}`
    # Set values to write into new file and report old and new values, where relevant
    if ( $MODDEG == 0 ) then
      set DEG = ${ODEG}
    else
      echo -n "  deg: ${ODEG} -> ${DEG}"
    endif
    if ( $MODTOL == 0 ) then
      set TOL = ${OTOL}
    else
      echo -n "  tol: ${OTOL} -> ${TOL}"
    endif
    if ( $MODMIN == 0 ) then
      set MIN = ${OMIN}
    else
      echo -n "  min: ${OMIN} -> ${MIN}"
    endif
    if ( $MODMAX == 0 ) then
      set MAX = ${OMAX}
    else
      echo -n "  max: ${OMAX} -> ${MAX}"
    endif
    # Move old reduction script
    set OREDFILE = "${REDFILE:r}.old.cpl"
    \mv -f ${REDFILE} ${OREDFILE}
    # Construct new reduction script in temporary file
    @ j = $WAVCALLINE - 1
    # Copy old file above relevant line
    touch ${REDFILE}
    head -$j ${OREDFILE} >> ${REDFILE}
    # Copy first part of relevant line from old script
    awk -F '--degree=|--tolerance=|--minlines=|--maxlines=' 'NR == '$WAVCALLINE' {printf "%s",$1}' ${OREDFILE} >> ${REDFILE}
    # Write in values of variables
    echo -n "--degree=${DEG} --tolerance=${TOL} --minlines=${MIN} --maxlines=${MAX} " >> ${REDFILE}
    # Grab last part of relevant lines from old script
    awk 'NR == '$WAVCALLINE' {print $NF}' ${OREDFILE} >> ${REDFILE}
    # Copy old file below relevant line
    awk 'NR > '$WAVCALLINE' {print $0}' ${OREDFILE} >> ${REDFILE}
    echo " done"
  else
    echo "${PROG}: ERROR: Cannot read file ${REDFILE}"
  endif
end
echo "${PROG}: Modifications complete"

exit
