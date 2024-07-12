export TARGET_EXEC := atom
export BUILD_DIR := ./build
export SRC_DIR := ./src

CPPFLAGS := -Wall -std=c++20 -fno-rtti -mbmi -mbmi2 -mpopcnt -msse2 -msse3 -msse4.1 -mavx2
RELEASE_CPPFLAGS := $(CPPFLAGS) -O3 -funroll-loops -finline -fomit-frame-pointer -flto -DNDEBUG

LDFLAGS := -Wall -std=c++20 -fno-rtti -mbmi -mbmi2 -mpopcnt -msse2 -msse3 -msse4.1 -mavx2
RELEASE_LDFLAGS := $(LDFLAGS) -flto -s -static

.PHONY: all debug release profile

all: release

release:
	$(MAKE) -f build.mk clean TARGET=Release
	$(MAKE) -f build.mk TARGET=Release CPPFLAGS="$(RELEASE_CPPFLAGS)" LDFLAGS="$(RELEASE_LDFLAGS)"

clean:
	$(MAKE) -f build.mk clean TARGET=Debug
	$(MAKE) -f build.mk clean TARGET=Release

