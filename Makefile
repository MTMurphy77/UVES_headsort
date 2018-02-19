# Makefile for UVES_HEADSORT
HS_NAME = UVES_headsort
CH_NAME = UVES_copyhead
IP_NAME = UVES_itphmod
WR_NAME = UVES_wavres

# Linux
# NOTE: Change compilation command for "UVES_popler" below to use
# $(FC) for compilation and linking as well.
#SHELL = tcsh
#CC = gcc
#FC = gfortran
#CFLAGS = -I${CFITSIO_DIR}/include -O2 -Wall -I./
#LIBS = -L${CFITSIO_DIR}/lib -lm -lcfitsio
#TARGET = ${HOME}/bin

# Mac OS X - assumes CFITSIO was installed through MacPorts.
SHELL = tcsh
CC = gcc
CFLAGS = -O2 -Wall -I./ -I/opt/local/include
LIBS = -lm /opt/local/lib/libcfitsio.a
TARGET = ${HOME}/bin

HS_OBJECTS = UVES_headsort.o errormsg.o faskropen.o faskwopen.o fcompl.o get_input.o getscbc.o iarray.o isdir.o nferrormsg.o qsort_calsrch.o qsort_mjd.o strlower.o UVES_calsrch.o UVES_link.o UVES_list.o UVES_Macmap.o UVES_params_init.o UVES_params_set.o UVES_rfitshead.o UVES_wheadinfo.o UVES_wredscr.o warnmsg.o

CH_OBJECTS = UVES_copyhead.o errormsg.o faskropen.o fcompl.o get_input.o getscbc.o isdir.o nferrormsg.o

IP_OBJECTS = UVES_itphmod.o errormsg.o darray.o faskropen.o fcompl.o get_input.o getscbc.o isdir.o isodd.o median.o nferrormsg.o qsort_darray.o

WR_OBJECTS = UVES_wavres.o errormsg.o darray.o iarray.o isodd.o median.o nferrormsg.o qsort_darray.o stats.o strlower.o warnmsg.o

UTILS = uves_changelinks.csh uves_filtplot.py uves_makesof.csh uves_copyhead.csh uves_itphmod.csh uves_itwavres.csh uves_wavcheck.csh uves_modcpl.csh uves_pmcheck.csh

all: $(HS_NAME) $(CH_NAME) $(IP_NAME) $(WR_NAME)

$(HS_NAME): $(HS_OBJECTS)
	$(CC) -o $(HS_NAME) $(HS_OBJECTS) $(LIBS)
#	$(FC) -o $(HS_NAME) $(HS_OBJECTS) $(LIBS)

$(CH_NAME): $(CH_OBJECTS)
	$(CC) -o $(CH_NAME) $(CH_OBJECTS) $(LIBS)
#	$(FC) -o $(CH_NAME) $(CH_OBJECTS) $(LIBS)

$(IP_NAME): $(IP_OBJECTS)
	$(CC) -o $(IP_NAME) $(IP_OBJECTS) $(LIBS)
#	$(FC) -o $(IP_NAME) $(IP_OBJECTS) $(LIBS)

$(WR_NAME): $(WR_OBJECTS)
	$(CC) -o $(WR_NAME) $(WR_OBJECTS) $(LIBS)
#	$(FC) -o $(WR_NAME) $(WR_OBJECTS) $(LIBS)

install:
	/bin/cp -f $(HS_NAME) $(CH_NAME) $(IP_NAME) $(WR_NAME) $(UTILS) $(TARGET)

depend:
	makedepend -f Makefile -Y -- $(CFLAGS) -- -s "# Dependencies" \
	$(HS_OBJECTS:.o=.c) $(CH_OBJECTS:.o=.c) $(IP_OBJECTS:.o=.c) \
	$(WR_OBJECTS:.o=.c) >& /dev/null

clean: 
	/bin/rm -f *~ *.o

# Dependencies

UVES_headsort.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_headsort.o: /opt/local/include/longnam.h charstr.h file.h memory.h
UVES_headsort.o: error.h
faskropen.o: file.h input.h error.h
faskwopen.o: file.h input.h error.h
fcompl.o: charstr.h file.h error.h
get_input.o: charstr.h error.h input.h
getscbc.o: charstr.h input.h error.h
iarray.o: error.h
qsort_calsrch.o: UVES_headsort.h /opt/local/include/fitsio.h
qsort_calsrch.o: /opt/local/include/longnam.h charstr.h
qsort_mjd.o: UVES_headsort.h /opt/local/include/fitsio.h
qsort_mjd.o: /opt/local/include/longnam.h charstr.h
UVES_calsrch.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_calsrch.o: /opt/local/include/longnam.h charstr.h error.h
UVES_link.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_link.o: /opt/local/include/longnam.h charstr.h file.h error.h
UVES_list.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_list.o: /opt/local/include/longnam.h charstr.h memory.h file.h error.h
UVES_Macmap.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_Macmap.o: /opt/local/include/longnam.h charstr.h file.h error.h
UVES_params_init.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_params_init.o: /opt/local/include/longnam.h charstr.h
UVES_params_set.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_params_set.o: /opt/local/include/longnam.h charstr.h
UVES_rfitshead.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_rfitshead.o: /opt/local/include/longnam.h charstr.h const.h error.h
UVES_wheadinfo.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_wheadinfo.o: /opt/local/include/longnam.h charstr.h file.h error.h
UVES_wredscr.o: UVES_headsort.h /opt/local/include/fitsio.h
UVES_wredscr.o: /opt/local/include/longnam.h charstr.h file.h error.h
UVES_copyhead.o: /opt/local/include/fitsio.h /opt/local/include/longnam.h
UVES_copyhead.o: charstr.h file.h error.h
faskropen.o: file.h input.h error.h
fcompl.o: charstr.h file.h error.h
get_input.o: charstr.h error.h input.h
getscbc.o: charstr.h input.h error.h
UVES_itphmod.o: stats.h charstr.h file.h memory.h error.h
darray.o: error.h
faskropen.o: file.h input.h error.h
fcompl.o: charstr.h file.h error.h
get_input.o: charstr.h error.h input.h
getscbc.o: charstr.h input.h error.h
median.o: sort.h stats.h memory.h error.h
UVES_wavres.o: /opt/local/include/fitsio.h /opt/local/include/longnam.h
UVES_wavres.o: stats.h charstr.h file.h memory.h const.h error.h
darray.o: error.h
iarray.o: error.h
median.o: sort.h stats.h memory.h error.h
stats.o: stats.h error.h
