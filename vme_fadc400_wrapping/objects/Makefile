# Makefile for DAQ Objects (ROOT 6 Compatible)
FILENAME = objects
LIBname  = libobjects.so

CXX      = g++
ROOTCFLAGS := $(shell root-config --cflags)
ROOTLIBS   := $(shell root-config --libs)

CXXFLAGS = -fPIC -O2 -Wall $(ROOTCFLAGS) -I./include
LDFLAGS  = -shared $(ROOTLIBS)

DIR_INC = include
DIR_SRC = src
DIR_TMP = tmp
DIR_LIB = lib

DICT      = $(FILENAME)Dict
DICTSRC   = $(DIR_TMP)/$(DICT).cxx
DICTOBJ   = $(DIR_TMP)/$(DICT).o
# PCM 파일 이름 정의 (ROOT 6)
PCMNAME   = $(DICT)_rdict.pcm

HEADERS  = $(wildcard $(DIR_INC)/*.hh)
SOURCES  = $(wildcard $(DIR_SRC)/*.cc)
OBJECTS  = $(patsubst $(DIR_SRC)/%.cc, $(DIR_TMP)/%.o, $(SOURCES))

all: dirs $(DIR_LIB)/$(LIBname)

dirs:
	mkdir -p $(DIR_TMP) $(DIR_LIB)

$(DIR_LIB)/$(LIBname): $(OBJECTS) $(DICTOBJ)
	@echo "Linking $@"
	$(CXX) $(LDFLAGS) -o $@ $^

# Dictionary Generation & PCM Move
$(DICTSRC): $(HEADERS) $(DIR_INC)/LinkDef.h
	@echo "Generating dictionary..."
	rootcling -f $@ -c -I$(DIR_INC) $^
	@echo "Moving PCM file to lib..."
	mv $(DIR_TMP)/$(PCMNAME) $(DIR_LIB)/

$(DIR_TMP)/%.o: $(DIR_SRC)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(DIR_TMP)/%.o: $(DIR_TMP)/%.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(DIR_TMP) $(DIR_LIB)