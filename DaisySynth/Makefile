# Project Name
TARGET = TheSynthDaisy

# Sources
CPP_SOURCES = DaisySynth.cpp

# ReverbSc lives in DaisySP-LGPL and is linked only for this effect block.
USE_DAISYSP_LGPL = 1

# DaisyExamples is a sibling of hardware-toolkit in the current workspace.
# Override either variable on the make command line if your checkout differs.
LIBDAISY_DIR ?= ../../DaisyExamples/libDaisy
DAISYSP_DIR ?= ../../DaisyExamples/DaisySP

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
