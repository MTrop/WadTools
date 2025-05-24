##############################################################################
## WadTools Makefile
##############################################################################

##############
### PATHS
##############

# Source directory.
SRC_DIR          := src
# Test Source directory.
TEST_SRC_DIR     := testsrc
# Build directory.
BUILD_DIR        := build
# Build directory.
TEST_BUILD_DIR   := build/test
# Distributable directory.
DIST_DIR         := dist
# Distributable directory.
TEST_DIR         := dist/test
# Modules to build.
MODULES          := io parser struct wad wadio wadtool
# Images to build.
EXECUTABLES      := wad
TEST_EXECUTABLES := test testlexer teststream
EXE_SUFFIX       := .exe


##############
### UTILITIES
##############

# C Compiler
CC               := gcc
# C Compiler Flags
CCFLAGS          := -Wall
# C Include Directories
INCLUDES         := "-I./$(SRC_DIR)"

# Make directory command.
MKDIR_CMD        := @mkdir
# Delete directory tree command.
DELDIR_CMD       := @rm -Rf


##############
### LISTS
##############

vpath %.c $(SRC_DIR)
MODULE_DEST           := $(BUILD_DIR) $(addprefix $(BUILD_DIR)/,$(MODULES)) 
MODULE_SRC_PATHS      := $(foreach sdir,$(MODULES),$(wildcard $(SRC_DIR)/$(sdir)/*.c))
MODULE_SRC_FILES      := $(patsubst $(SRC_DIR)/%,%,$(MODULE_SRC_PATHS))
MODULE_OBJ_FILES      := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(MODULE_SRC_FILES)))

LINKED_SRC_FILES      := $(addsuffix .c,$(EXECUTABLES))
LINKED_OBJ_FILES      := $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(EXECUTABLES)))
LINKED_FILES          := $(addprefix $(DIST_DIR)/,$(addsuffix $(EXE_SUFFIX),$(EXECUTABLES)))

## ---------------------------------------------------------------------------
## Targets
## ---------------------------------------------------------------------------

default: makedirs compile link
	
clean:
	@echo Cleaning....
	@rm -Rf $(TEST_DIR)
	@rm -Rf $(DIST_DIR)
	@rm -Rf $(TESTBUILD_DIR)
	@rm -Rf $(BUILD_DIR)
	@echo Done.

cleanexe:
	@rm $(LINKED_FILES)

compile: $(MODULE_OBJ_FILES)

testcompile: compile $(TEST_MODULE_OBJ_FILES)

link: compile $(LINKED_FILES)

makedirs: $(MODULE_DEST) $(DIST_DIR)

$(DIST_DIR):
	@mkdir $@

$(MODULE_DEST):
	@mkdir $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo ==== Compiling $< to $@ ....
	@$(CC) $(CCFLAGS) $(INCLUDES) -o $@ -c $<

$(TEST_BUILD_DIR)/%.o: $(TEST_SRC_DIR)/%.c
	@echo ==== Compiling $< to $@ ....
	@$(CC) $(CCFLAGS) $(INCLUDES) -o $@ -c $<

$(LINKED_FILES): $(LINKED_OBJ_FILES)
	@echo ==== Linking $@ ....
	@$(CC) $(CCFLAGS) $(INCLUDES) -o $@ $< $(MODULE_OBJ_FILES)
	@echo ==== Finished.
