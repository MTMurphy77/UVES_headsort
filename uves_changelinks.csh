#!/bin/tcsh

if ($1 == "") then
  echo "$0"": FATAL ERROR: You must specifiy a target"
  echo "  directory path"
  exit 0
endif

if ( ! -d $1 ) then
  echo "$0"": FATAL ERROR: Argument specified,"
  echo "  $1,"
  echo "  is not a directory"
  exit 0
endif

if (`echo "$1" | awk '{if (substr($1,1,1) != "/") print 1; else print 0}'`) then
  echo "$0"": FATAL ERROR: You must use an absolute path"
  echo "  for the target directory"
  exit 0
endif

if ($2 != "") then
  if ($2 < 0) then
    echo "$0"": FATAL ERROR: Number of directory path segments to keep"
    echo "      must be >= 0"
    exit 0
  endif
  @ NDIR = `echo $2 | awk '{printf "%d",$1}'`
else
  @ NDIR = 0
endif

set LINKNAMES = `/bin/ls -l *.fits | awk '{print $9}'`
set OLDTARGETS = `/bin/ls -l *.fits | awk '{print $11}'`

@ i = 0
foreach FILE ($LINKNAMES)
  @ i = $i + 1
  if ( -l $FILE ) then
    set OLDTARGET = `/bin/ls -l $FILE | awk '{print $11}'`
    if ( `echo "$FILE"` == "thargood.fits") then
      set NEWTARGET = $UVES_HEADSORT_THARFILE
      if ( ! -r $NEWTARGET ) then
        echo "$0"": FATAL ERROR: Cannot read file $NEWTARGET"
        exit 0
      endif
    else if ( `echo "$FILE"` == "atmoexan.fits") then
      set NEWTARGET = $UVES_HEADSORT_ATMOFILE
      if ( ! -r $NEWTARGET ) then
        echo "$0"": FATAL ERROR: Cannot read file $NEWTARGET"
        exit 0
      endif
    else if ( `echo "$FILE"` == "flxstd.fits") then
      set NEWTARGET = $UVES_HEADSORT_FLSTFILE
      if ( ! -r $NEWTARGET ) then
        echo "$0"": FATAL ERROR: Cannot read file $NEWTARGET"
        exit 0
      endif
    else
      set NEWTARGET = `echo "$OLDTARGET" | awk '{ n=split($1,p,"/"); for (i=n-'$NDIR'; i<=n; i++) printf "'$1'/%s",p[i]; }'`
    endif
    /bin/rm -f $FILE
    ln -s $NEWTARGET $FILE
  endif
end

exit
