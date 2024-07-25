CXX := g++
# CXX := clang++

TARGET_EXEC := atom
SRC_DIRS := src src/incbin src/nnue src/nnue/features src/nnue/layers
SOURCES := $(wildcard $(addsuffix /*.cpp,$(SRC_DIRS)))
OBJECTS := $(SOURCES:.cpp=.o)

COMMONFLAGS  := -Wall -std=c++20 -fno-rtti -lpthread -DUSE_PTHREADS
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
DEBUG_LDFLAGS := $(LDFLAGS)

RELEASE_CXXFLAGS := $(CXXFLAGS) -O3 -funroll-loops -finline-functions -finline-limit=1000 -fomit-frame-pointer -flto -flto-partition=one -flto=jobserver -ftree-vectorize -fprefetch-loop-arrays -fpeel-loops -funroll-all-loops -march=native -DNDEBUG -mtune=native
RELEASE_LDFLAGS := $(LDFLAGS) -flto -s -static

PROFILE_CXXFLAGS := $(CXXFLAGS) $(RELEASE_CXXFLAGS) -g
PROFILE_LDFLAGS := $(LDFLAGS) -flto -g -static

.PHONY: all debug release profile clean

all: release

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

debug: CXXFLAGS := $(DEBUG_CXXFLAGS)
debug: LDFLAGS := $(DEBUG_LDFLAGS)
debug: $(TARGET_EXEC)

release: CXXFLAGS := $(RELEASE_CXXFLAGS)
release: LDFLAGS := $(RELEASE_LDFLAGS)
release: $(TARGET_EXEC)

profile: CXXFLAGS := $(PROFILE_CXXFLAGS)
profile: LDFLAGS := $(PROFILE_LDFLAGS)
profile: $(TARGET_EXEC)

$(TARGET_EXEC): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(OBJECTS)
	rm -f $(TARGET_EXEC)
