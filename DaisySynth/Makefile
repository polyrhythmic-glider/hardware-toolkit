# Project Name
TARGET = TheSynthDaisy

# Sources
CPP_SOURCES = DaisySynth.cpp SynthEngine.cpp

# Set to 0 for release builds that should skip the pre-audio worst-case test.
SYNTH_ENABLE_STARTUP_VALIDATION ?= 1
C_DEFS += -DSYNTH_ENABLE_STARTUP_VALIDATION=$(SYNTH_ENABLE_STARTUP_VALIDATION)

# ReverbSc lives in DaisySP-LGPL and is linked only for this effect block.
USE_DAISYSP_LGPL = 1

# DaisyExamples is a sibling of hardware-toolkit in the current workspace.
# Override either variable on the make command line if your checkout differs.
LIBDAISY_DIR ?= ../../DaisyExamples/libDaisy
DAISYSP_DIR ?= ../../DaisyExamples/DaisySP

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

HOST_CXX ?= g++
HOST_TEST_BINARY = build/SynthEngineHostTest
HOST_TEST_SOURCES = \
	tests/SynthEngineHostTest.cpp \
	SynthEngine.cpp \
	$(DAISYSP_DIR)/Source/Control/adsr.cpp \
	$(DAISYSP_DIR)/Source/Effects/wavefolder.cpp \
	$(DAISYSP_DIR)/Source/Filters/svf.cpp \
	$(DAISYSP_DIR)/Source/Synthesis/oscillator.cpp

.PHONY: test-host
test-host:
	@mkdir -p build
	$(HOST_CXX) -std=c++14 -O2 -Wall \
		-I. -I$(DAISYSP_DIR)/Source -I$(DAISYSP_DIR)/Source/Utility \
		$(HOST_TEST_SOURCES) -lm -o $(HOST_TEST_BINARY)
	$(HOST_TEST_BINARY)
