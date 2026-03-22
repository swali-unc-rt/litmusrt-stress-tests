CC := g++
CSTD := c++14
CFLAGS := -Ofast -std=$(CSTD)

SRC_DIR = src
INCLUDE_DIRS = include

SRC_FILES = main.cpp task_gen.cpp

all:
	$(CC) $(CFLAGS) $(foreach dir,$(INCLUDE_DIRS),-I$(dir)) $(foreach srcfile,$(SRC_FILES),$(SRC_DIR)/$(srcfile)) -o taskgen

.PHONY: all clean

clean:
	rm taskgen
