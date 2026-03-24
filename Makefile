# Specify the targets here.
# Note each target has to have a cpp file in the "main" folder.
# TARGETS = rgtest smlpstresstest smlpcorrectnesstest
TARGETS = $(patsubst $(MAIN_DIR)/%.cpp,%,$(wildcard $(MAIN_DIR)/*.cpp))

# C++ compiler
CXX = g++

RT_NUM_CPUS=8

# project has an include directory
PROJECT_INCLUDE_DIR ?= ./include

# liblitmus directories
LIBLITMUS_INCLUDE_DIR ?= ../liblitmus/include
LIBLITMUS_ARCH_INCLUDE_DIR ?= ../liblitmus/arch/x86/include
LIBLITMUS_LIB_DIR ?= ../liblitmus

FEATHERTRACE_DIR ?= ../feather-trace-tools

# objects to compile
SRC_DIR = ./src
OBJ_DIR = ./obj
MAIN_DIR = ./main
BIN_DIR = ./bin
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/*.cu)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Compiler flags
CXXFLAGS = -std=c++20 -I/usr/local/include -I$(PROJECT_INCLUDE_DIR) -I$(LIBLITMUS_INCLUDE_DIR) -I$(LIBLITMUS_ARCH_INCLUDE_DIR)
CXXFLAGS += -DCONFIG_LITMUS_LOCKING_SMLP -DCONFIG_LITMUS_LOCKING_WITHARGS -DCONFIG_LITMUS_LOCKING_OMLP -DCONFIG_LITMUS_ENABLE_RELEASEGROUPS
CXXFLAGS += -DLIBLITMUS_LIB_DIR=\"$(LIBLITMUS_LIB_DIR)\" -DRT_NUM_CPUS=$(RT_NUM_CPUS) -g

# Linker flags
LDFLAGS =  -L$(LIBLITMUS_LIB_DIR) -llitmus

all: $(TARGETS)

# Executable rule, place in bin folder
$(TARGETS): %: $(OBJ_FILES) $(OBJ_DIR)/%.o
	$(CXX) $(OBJ_DIR)/$@.o $(OBJ_FILES) -o $(BIN_DIR)/$@ $(LDFLAGS)
	@echo "Executable: $(BIN_DIR)/$@"

# Rule to compile C++ source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "Compiled: $<"

$(OBJ_DIR)/%.o: $(MAIN_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "Compiled: $<"

# Clean up build artifacts
clean:
	rm -f $(wildcard $(OBJ_DIR)/*.o) $(wildcard $(BIN_DIR)/*)

# Phony targets
.PHONY: all run clean