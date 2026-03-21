#LIBUSB_INC = /usr/include/libusb-1.0
#LIBUSB_LIB = /usr/lib64

FILENAME = test_display

EXE	=	 $(FILENAME).exe

CC	=	gcc

COPTS   =       -fPIC -Wall

FLAGS   =       -shared -Wall,-soname -nostartfiles -s

LDFLAGS =        -O2 -Wall

OutPutOpt = -o

LIBS    =       -L$(LIBUSB_LIB) -lusb-1.0 -L$(NKHOME)/lib  -lNotice6UVME -lNoticeDISPLAY -lm

INCLUDEDIR =    -I$(NKHOME)/include -I$(LIBUSB_INC) -I.

INSTALLDIR     =        $(NKHOME)/bin
INSTALLINCLUDE =        $(NKHOME)/include

OBJS    =       $(FILENAME).o

INCLUDES =


#########################################################################

all     :       $(EXE)
clean   :
		/bin/rm -f $(OBJS) $(EXE)

install :       $(EXE)
		#/bin/rm -f $(INSTALLDIR)/$(EXE)
		install $(EXE) $(INSTALLDIR)

$(EXE):      $(OBJS)
		$(CC) $^ $(LIBS) $(OutPutOpt)$@ $(LDFLAGS)
		@echo "$@ done"

$(OBJS) :       $(INCLUDES)

%.o     :       %.c
		$(CC) $(COPTS) $(INCLUDEDIR) -c -o $@ $<

