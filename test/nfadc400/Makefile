CC      = g++
TARGET  = nfadc400_test

# ROOT Config
ROOTCFLAGS = $(shell root-config --cflags)
ROOTLIBS   = $(shell root-config --libs)

# Include & Link
# [핵심] 우리가 빌드한 _root.so 라이브러리 사용
CFLAGS  = -O2 -Wall -fPIC -DLINUX -DNKROOT $(ROOTCFLAGS) -I$(NKHOME)/include
LIBS    = -L$(NKHOME)/lib -lNoticeNFADC400_root -lNotice6UVME_root -llinux_interface_c $(ROOTLIBS) -lrt

SRCS    = nfadc400_test.cc
OBJS    = $(SRCS:.cc=.o)

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o : %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f $(TARGET) $(OBJS) legacy_test.root