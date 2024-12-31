CXX := g++
# CXX := clang++

TARGET_EXEC := atom
BUILD_DIR := build
SRC_DIRS := src src/incbin src/nnue src/nnue/features src/nnue/layers

# Create list of source files and corresponding object files in build dir
SOURCES := $(wildcard $(addsuffix /*.cpp,$(SRC_DIRS)))
OBJECTS := $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# Setup CPU flags
COMMONFLAGS  := -Wall -std=c++20 -fno-rtti
SSE2FLAGS    := $(COMMONFLAGS) -msse2
SSE4FLAGS    := $(SSE2FLAGS) -msse3 -msse4 -msse4.1 -mpopcnt
AVX2FLAGS    := $(SSE4FLAGS) -mavx2
BMI2FLAGS    := $(AVX2FLAGS) -mbmi -mbmi2 -mmmx
AVX512FLAGS  := $(BMI2FLAGS) -mavx512f -mavx512bw -mavx512dq
VNNI512FLAGS := $(AVX512FLAGS) -mavx512vnni -mavx512vl -mprefer-vector-width=512

# Setup correct SIMD architecture for NNUE
SSE2FLAGS     += -DUSE_SSE2
SSE4FLAGS     += -DUSE_SSE41
AVX2FLAGS     += -DUSE_AVX2
BMI2FLAGS     += -DUSE_BMI2
AVX512FLAGS   += -DUSE_AVX512
VNNI512FLAGS  += -DUSE_VNNI

CPUFLAGS := $(shell ./scripts/detect_cpu_flags.sh)

CXXFLAGS := $($(CPUFLAGS))
LDFLAGS := $($(CPUFLAGS))

DEBUG_CXXFLAGS := $(CXXFLAGS) -g -O0 -DDEBUG
RELEASE_CXXFLAGS := $(CXXFLAGS) -O3 -DNDEBUG -funroll-loops -finline -fomit-frame-pointer \
    -flto=$(shell nproc) -flto-partition=one -mtune=native -march=native \
    -fno-asynchronous-unwind-tables -fno-exceptions -fno-rtti \
    -fira-loop-pressure -fira-hoist-pressure -ftree-vectorize \
    -ffast-math -funsafe-math-optimizations -fno-trapping-math \
    -ffinite-math-only -fno-signaling-nans -fcx-limited-range \
    -fno-stack-protector

PROFILE_CXXFLAGS := $(CXXFLAGS) $(RELEASE_CXXFLAGS) -g

DEBUG_LDFLAGS := $(LDFLAGS)
RELEASE_LDFLAGS := $(LDFLAGS) -s -static -flto -flto-partition=one -flto=jobserver
PROFILE_LDFLAGS := $(LDFLAGS) -g -static -flto -flto-partition=one -flto=jobserver

.PHONY: all nnue debug release profile clean tune

all: nnue release

tune: CXXFLAGS := $(RELEASE_CXXFLAGS) -DENABLE_TUNING
tune: LDFLAGS := $(RELEASE_LDFLAGS)
tune: $(TARGET_EXEC)

nnue:
	./scripts/nnue.sh

# Create build directories and compile object files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
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
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET_EXEC)