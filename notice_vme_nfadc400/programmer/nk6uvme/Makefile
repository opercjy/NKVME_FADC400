# Makefile for nk6uvme utility
TARGET = nk6uvme

CXX      = g++
# [FIX] Added -DNKC to disable ROOT dependency in header files
CXXFLAGS = -O2 -Wall -DNKC -I$(NKHOME)/include
LDFLAGS  = -L$(NKHOME)/lib -lNotice6UVME_c -llinux_interface_c

# Source objects
OBJS = Programmer.o Option.o Device.o Actions.o pvmex.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)
	@echo "Installing $(TARGET) to $(NKHOME)/bin"
	cp $(TARGET) $(NKHOME)/bin/

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
