export TARGET_EXEC := atom
export BUILD_DIR := ./build
export SRC_DIR := ./src

COMMONFLAGS := -Wall -std=c++20 -fno-rtti
SSE2FLAGS   := $(COMMONFLAGS) -msse2 -DUSE_SSE -DUSE_SSE2
SSE4FLAGS   := $(SSE2FLAGS) -msse3 -msse4 -msse4.1 -mpopcnt -DUSE_SSE4 -DUSE_POPCNT
AVX2FLAGS   := $(SSE4FLAGS) -mavx2 -DUSE_AVX2
BMI2FLAGS   := $(AVX2FLAGS) -mbmi -mbmi2 -DUSE_BMI2
AVX512FLAGS := $(BMI2FLAGS) -mavx512f -mavx512bw -mavx512dq -DUSE_AVX512

BESTFLAGS := $(shell ./detect_cpu_flags.sh)

CPPFLAGS := $($(BESTFLAGS))
LDFLAGS := $($(BESTFLAGS))

DEBUG_CPPFLAGS := $(CPPFLAGS) -g -O0 -DDEBUG
RELEASE_CPPFLAGS := $(CPPFLAGS) -O3 -funroll-loops -finline -fomit-frame-pointer -flto -DNDEBUG -mtune=native
PROFILE_CPPFLAGS := $(CPPFLAGS) $(RELEASE_CPPFLAGS) -g

DEBUG_LDFLAGS := $(LDFLAGS)
RELEASE_LDFLAGS := $(LDFLAGS) -flto -s -static
PROFILE_LDFLAGS := $(LDFLAGS) -flto -g -static

.PHONY: all debug release profile clean

all: release

debug:
	@echo "Using flag set: $(BESTFLAGS)"
	$(MAKE) -f build.mk TARGET=Debug CPPFLAGS="$(DEBUG_CPPFLAGS)" LDFLAGS="$(DEBUG_LDFLAGS)"

release:
	@echo "Using flag set: $(BESTFLAGS)"
	$(MAKE) -f build.mk clean TARGET=Release
	$(MAKE) -f build.mk TARGET=Release CPPFLAGS="$(RELEASE_CPPFLAGS)" LDFLAGS="$(RELEASE_LDFLAGS)"

profile:
	@echo "Using flag set: $(BESTFLAGS)"
	$(MAKE) -f build.mk clean TARGET=Profile
	$(MAKE) -f build.mk TARGET=Profile CPPFLAGS="$(PROFILE_CPPFLAGS)" LDFLAGS="$(PROFILE_LDFLAGS)"

clean:
	$(MAKE) -f build.mk clean TARGET=Debug
	$(MAKE) -f build.mk clean TARGET=Release

