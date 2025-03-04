.PHONY: clean

PLATFORM := $(shell uname -s)

VERSION_TAG := $(shell git describe --always --tags --abbrev=0)
COMMIT_HASH := $(shell git rev-parse --short HEAD)
OS_INFO := $(shell uname -rmo)

EXEC := iris

INCLUDE_DIRS := imgui imgui/backends frontend gl3w/include tomlplusplus/include
OUTPUT_DIR := bin

CXX := g++
CXXFLAGS := $(addprefix -I, $(INCLUDE_DIRS)) -iquote src $(shell sdl2-config --cflags --libs)
CXXFLAGS += -O3 -march=native -mtune=native -flto=auto -Wall -g -D_EE_USE_INTRINSICS
CXXFLAGS += -D_IRIS_VERSION="$(VERSION_TAG)"
CXXFLAGS += -D_IRIS_COMMIT="$(COMMIT_HASH)"
CXXFLAGS += -D_IRIS_OSVERSION="$(OS_INFO)"
CXXSRC := $(wildcard imgui/*.cpp)
CXXSRC += $(wildcard imgui/backends/imgui_impl_sdl2.cpp)
CXXSRC += $(wildcard imgui/backends/imgui_impl_opengl3.cpp)
CXXSRC += $(wildcard frontend/platform/stub.cpp)
CXXSRC += $(wildcard frontend/*.cpp)
CXXSRC += $(wildcard frontend/ui/*.cpp)
CXXSRC += $(wildcard src/ee/renderer/*.cpp)
CXXSRC += $(wildcard src/ee/*.cpp)
CXXOBJ := $(CXXSRC:.cpp=.o)

CC := gcc
CFLAGS := $(addprefix -I, $(INCLUDE_DIRS)) -iquote src $(shell sdl2-config --cflags --libs)
CFLAGS += -O3 -ffast-math -march=native -mtune=native -pedantic
CFLAGS += -flto=auto -Wall -mssse3 -msse4 -D_EE_USE_INTRINSICS
CSRC := $(wildcard src/*.c)
CSRC += $(wildcard src/ee/*.c)
CSRC += $(wildcard src/iop/*.c)
CSRC += $(wildcard src/shared/*.c)
CSRC += $(wildcard src/dev/*.c)
CSRC += $(wildcard gl3w/src/*.c)
CSRC += $(wildcard frontend/tfd/*.c)
COBJ := $(CSRC:.c=.o)

ifeq ($(PLATFORM),Darwin)
	CFLAGS += -mmacosx-version-min=10.9 -Wno-newline-eof
	CXXFLAGS += -mmacosx-version-min=10.9 -Wno-newline-eof
endif

all: $(OUTPUT_DIR) $(COBJ) $(CXXOBJ) $(OUTPUT_DIR)/$(EXEC)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(COBJ): %.o: %.c
	@echo " CC " $<; $(CC) -c $< -o $@ $(CFLAGS)

$(CXXOBJ): %.o: %.cpp
	@echo " CXX" $<; $(CXX) -c $< -o $@ $(CXXFLAGS)

$(OUTPUT_DIR)/$(EXEC): $(COBJ) $(CXXOBJ)
	$(CXX) $(COBJ) $(CXXOBJ) main.cpp -o $(OUTPUT_DIR)/$(EXEC) $(CXXFLAGS)

clean:
	rm -rf $(OUTPUT_DIR)
	rm $(COBJ) $(CXXOBJ)