# This is a makefile that will build a command-line application and library
# files for Linux, macOS, BSD, and similar operating systems.
# Requires libflac-dev, libsndfile1-dev (or equivalent) for MP3/Ogg Vorbis/AIFF support.

# Detect the operating system
OS := $(shell uname)

CXX := g++

# Platform-specific settings
# macOS (Darwin): g++ is actually clang, so avoid GCC-specific flags.
# Homebrew installs libraries to a prefix that the compiler does not search
# by default (/opt/homebrew on Apple Silicon, /usr/local on Intel).
# Dynamic library is .dylib on macOS, .so on Linux/BSD/etc.
ifeq ($(OS),Darwin)
	BREW_PREFIX := $(shell brew --prefix)
	PKG_CF := PKG_CONFIG_PATH=$(BREW_PREFIX)/lib/pkg-config pkg-config
	SNDFILE_CFLAGS := $(shell $(PKG_CF) --silence-errors --cflags sndfile 2>/dev/null)
	SNDFILE_LIBS := $(shell $(PKG_CF) --silence-errors --libs sndfile 2>/dev/null)
ifeq ($(strip $(SNDFILE_LIBS)),)
	SNDFILE_LIBS := -L$(BREW_PREFIX)/lib -lsndfile
endif
	CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -fPIC -I$(BREW_PREFIX)/include $(SNDFILE_CFLAGS)
	LDFLAGS := -L$(BREW_PREFIX)/lib -lFLAC $(SNDFILE_LIBS)
	DYNAMIC_LIB := libaudiomixer.dylib
	DYNAMIC_LDFLAGS := -dynamiclib -install_name $(DYNAMIC_LIB)
else
	SNDFILE_CFLAGS := $(shell pkg-config --silence-errors --cflags sndfile 2>/dev/null)
	SNDFILE_LIBS := $(shell pkg-config --silence-errors --libs sndfile 2>/dev/null)
ifeq ($(strip $(SNDFILE_LIBS)),)
	SNDFILE_LIBS := -lsndfile
endif
	CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -fPIC -fno-aggressive-loop-optimizations $(SNDFILE_CFLAGS)
	LDFLAGS := -lFLAC $(SNDFILE_LIBS)
	DYNAMIC_LIB := libaudiomixer.so
	DYNAMIC_LDFLAGS := -shared -Wl,-soname,$(DYNAMIC_LIB)
endif

STATIC_LIB := libaudiomixer.a
AUDIO_MIXER_CMDLINE_APP := audioMixer

SRC_DIR := ./common
SOURCES := \
 	$(SRC_DIR)/AudioFile.cpp \
 	$(SRC_DIR)/AudioFileInfo.cpp \
 	$(SRC_DIR)/AudioFileResultType.cpp \
 	$(SRC_DIR)/AudioFileTools.cpp \
 	$(SRC_DIR)/AiffFile.cpp \
 	$(SRC_DIR)/AiffFileInfo.cpp \
 	$(SRC_DIR)/EOUtils.cpp \
 	$(SRC_DIR)/FLACFile.cpp \
 	$(SRC_DIR)/FLACFileInfo.cpp \
 	$(SRC_DIR)/MetadataAudioFile.cpp \
 	$(SRC_DIR)/MP3File.cpp \
 	$(SRC_DIR)/MP3FileInfo.cpp \
 	$(SRC_DIR)/OggFile.cpp \
 	$(SRC_DIR)/OggFileInfo.cpp \
 	$(SRC_DIR)/SndFileBackedMetadataAudioFile.cpp \
 	$(SRC_DIR)/SndFileHelpers.cpp \
 	$(SRC_DIR)/utilFunctions.cpp \
 	$(SRC_DIR)/WAVFile.cpp \
 	$(SRC_DIR)/WAVFileInfo.cpp

OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all libs clean

all: $(AUDIO_MIXER_CMDLINE_APP) libs

libs: $(STATIC_LIB) $(DYNAMIC_LIB)

$(AUDIO_MIXER_CMDLINE_APP): $(OBJECTS) $(SRC_DIR)/cmdLineApp.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(SRC_DIR)/cmdLineApp.cpp $(LDFLAGS)

# Static library
$(STATIC_LIB): $(OBJECTS)
	ar rvu $(STATIC_LIB) $(OBJECTS)
	ranlib $(STATIC_LIB)

# Dynamic/shared library
$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(DYNAMIC_LDFLAGS) -o $(DYNAMIC_LIB) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(AUDIO_MIXER_CMDLINE_APP) $(OBJECTS) $(STATIC_LIB) $(DYNAMIC_LIB)
