CXX := g++
# CXX := clang++

TARGET_EXEC := atom
SRC_DIRS := src src/incbin src/nnue src/nnue/features src/nnue/layers
SOURCES := $(wildcard $(addsuffix /*.cpp,$(SRC_DIRS)))

COMMONFLAGS  := -Wall -std=c++20 -fno-rtti
SSE2FLAGS    := $(COMMONFLAGS) -msse2 -DUSE_SSE -DUSE_SSE2
SSE4FLAGS    := $(SSE2FLAGS) -msse3 -msse4 -msse4.1 -mpopcnt -DUSE_SSE41 -DUSE_POPCNT
AVX2FLAGS    := $(SSE4FLAGS) -mavx2 -DUSE_AVX2
BMI2FLAGS    := $(AVX2FLAGS) -mbmi -mbmi2 -DUSE_BMI2
AVX512FLAGS  := $(BMI2FLAGS) -mavx512f -mavx512bw -mavx512dq -DUSE_AVX512
VNNI512FLAGS := $(AVX512FLAGS) -mavx512vnni -mavx512vl -mprefer-vector-width=512 -DUSE_VNNI

CPUFLAGS := $(shell ./detect_cpu_flags.sh)

CXXFLAGS := $($(CPUFLAGS))
LDFLAGS := $($(CPUFLAGS))

DEBUG_CXXFLAGS := $(CXXFLAGS) -g -O0 -DDEBUG
RELEASE_CXXFLAGS := $(CXXFLAGS) -O3 -funroll-loops -finline -fomit-frame-pointer -flto -DNDEBUG -mtune=native
PROFILE_CXXFLAGS := $(CXXFLAGS) $(RELEASE_CXXFLAGS) -g

DEBUG_LDFLAGS := $(LDFLAGS)
RELEASE_LDFLAGS := $(LDFLAGS) -flto -s -static
PROFILE_LDFLAGS := $(LDFLAGS) -flto -g -static

.PHONY: all debug release profile clean

all: release

debug:
	@echo "Using flag set: $(CPUFLAGS)"
	$(CXX) $(DEBUG_CXXFLAGS) $(DEBUG_LDFLAGS) -o $(TARGET_EXEC) $(SOURCES)

release:
	@echo "Using flag set: $(CPUFLAGS)"
	$(CXX) $(RELEASE_CXXFLAGS) $(RELEASE_LDFLAGS) -o $(TARGET_EXEC) $(SOURCES)

profile:
	@echo "Using flag set: $(CPUFLAGS)"
	$(CXX) $(PROFILE_CXXFLAGS) $(PROFILE_LDFLAGS) -o $(TARGET_EXEC) $(SOURCES)

clean:
	rm -rf *.o
	rm -f $(TARGET_EXEC)

