# This is a makefile that will build a command-line application for Linux
# (and hopefully similar operating systems).
# Requires libflac-dev (or equivalent) to be installed.

CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -fno-aggressive-loop-optimizations
LDFLAGS := -lFLAC
AUDIO_MIXER_CMDLINE_APP := audioMixer

SRC_DIR := ./common
SOURCES := \
 	$(SRC_DIR)/AudioFile.cpp \
 	$(SRC_DIR)/AudioFileInfo.cpp \
 	$(SRC_DIR)/AudioFileResultType.cpp \
 	$(SRC_DIR)/AudioFileTools.cpp \
 	$(SRC_DIR)/EOUtils.cpp \
 	$(SRC_DIR)/FLACFile.cpp \
 	$(SRC_DIR)/FLACFileInfo.cpp \
 	$(SRC_DIR)/utilFunctions.cpp \
 	$(SRC_DIR)/WAVFile.cpp \
 	$(SRC_DIR)/WAVFileInfo.cpp

OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(AUDIO_MIXER_CMDLINE_APP)

$(AUDIO_MIXER_CMDLINE_APP): $(OBJECTS) $(SRC_DIR)/cmdLineApp.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(SRC_DIR)/cmdLineApp.cpp $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(AUDIO_MIXER_CMDLINE_APP) $(OBJECTS)
