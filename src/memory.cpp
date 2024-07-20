#include "memory.h"

#include <cstdlib>

#include <sys/mman.h>

namespace Atom {

void* std_aligned_alloc(size_t alignment, size_t size) {
    return aligned_alloc(alignment, size);
}

void std_aligned_free(void* ptr) {
    free(ptr);
}

void* aligned_large_pages_alloc(size_t allocSize) {

    constexpr size_t alignment = 2 * 1024 * 1024;

    // Round up to multiples of alignment
    size_t size = ((allocSize + alignment - 1) / alignment) * alignment;
    void*  mem  = std_aligned_alloc(alignment, size);
    #if defined(MADV_HUGEPAGE)
    madvise(mem, size, MADV_HUGEPAGE);
    #endif
    return mem;
}

void aligned_large_pages_free(void* mem) { std_aligned_free(mem); }

} // namespace Atom
