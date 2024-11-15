#if __has_include(<tracy/Tracy.hpp>)
#include <cstdlib>
#include <tracy/Tracy.hpp>
#endif

#ifdef SQLITEFS_ALLOCATORS_ENABLED
// NOLINTBEGIN
void* operator new(std::size_t size) {
    void* ptr = std::malloc(size);
    TracyAlloc(ptr, size);
    return ptr;
}

void* operator new[](std::size_t size) {
    void* ptr = std::malloc(size);
    TracyAlloc(ptr, size);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    TracyFree(ptr);
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    TracyFree(ptr);
    std::free(ptr);
}
// NOLINTEND
#endif