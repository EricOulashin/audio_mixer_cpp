 CXX := g++
 CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
 TARGET := audiomixer_cli
 
 SRC_DIR := .
 SOURCES := \
 	$(SRC_DIR)/AudioFile.cpp \
 	$(SRC_DIR)/AudioFileInfo.cpp \
 	$(SRC_DIR)/AudioFileResultType.cpp \
 	$(SRC_DIR)/AudioFileTools.cpp \
 	$(SRC_DIR)/EOUtils.cpp \
 	$(SRC_DIR)/StringUtils.cpp \
 	$(SRC_DIR)/utilFunctions.cpp \
 	$(SRC_DIR)/WAVFile.cpp \
 	$(SRC_DIR)/WAVFileInfo.cpp \
 	$(SRC_DIR)/cmdLineApp.cpp
 
 OBJECTS := $(SOURCES:.cpp=.o)
 
 .PHONY: all clean
 
 all: $(TARGET)
 
 $(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)
 
 %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
 
 clean:
	rm -f $(TARGET) $(OBJECTS)
