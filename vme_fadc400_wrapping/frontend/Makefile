# Makefile for DAQ & Production
EXE_DAQ  = frontend_nfadc400
EXE_PROD = production_nfadc400

CC       = g++
INCLUDES = -I../objects/include -I$(NKHOME)/include

# Link Objects & Notice Libs
LIBS_DAQ = -L../objects/lib -lobjects \
           -L$(NKHOME)/lib -lNoticeNFADC400_root -lNotice6UVME_root -llinux_interface_c

# Production tool needs Objects Lib only (for reading ROOT files)
LIBS_PROD = -L../objects/lib -lobjects

ROOTCFLAGS := $(shell root-config --cflags)
ROOTLIBS   := $(shell root-config --libs)

CXXFLAGS = -O2 -Wall -fPIC $(ROOTCFLAGS) $(INCLUDES)
LDFLAGS  = $(ROOTLIBS)

# Sources
OBJS_DAQ  = frontend.o ELog.o
OBJS_PROD = production.o

all: $(EXE_DAQ) $(EXE_PROD)

$(EXE_DAQ): $(OBJS_DAQ)
	$(CC) -o $@ $(OBJS_DAQ) $(LDFLAGS) $(LIBS_DAQ)
	@echo "Built $(EXE_DAQ)"

$(EXE_PROD): $(OBJS_PROD)
	$(CC) -o $@ $(OBJS_PROD) $(LDFLAGS) $(LIBS_PROD)
	@echo "Built $(EXE_PROD)"

%.o: %.cc
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXE_DAQ) $(EXE_PROD) *.o