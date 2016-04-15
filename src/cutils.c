#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <cutils.h>

@ {
    /* for atomics and pointer aliasing */
    #define sw_barrier() asm volatile("": : :"memory")
    #define hw_barrier() __sync_synchronize
    
    #ifdef __linux__
    #define smalloc(size) malloc(size)
    #define srealloc(ptr, size) realloc(size)
    #else
    #define smalloc(size) __safe_malloc(size)
    #define srealloc(ptr, size) __safe_realloc(ptr, size)
    #endif
    
    /* for other *nix variants; linux will lazily allocate memory and we'll
       probably have our process killed by the OOM killer before we run out
       actual, physical memory mapped to our process. */

    /* it's also likely in other systems that JVM code is more likely to abort
       than our own code since it interacts with the heap much more
       aggressively. */
    
    static inline void* __safe_malloc(size_t size) {
        void* ret;
        if (!(ret = malloc(size))) {
            fprintf(stderr, "malloc(): failed to allocate %d bytes", (int) size);
            abort();
        }
    }
    
    static inline void* __safe_realloc(void* ptr, size_t size) {
        void* ret;
        if (!(ret = realloc(ptr, size))) {
            fprintf(stderr, "realloc(): failed to re-allocate to %d bytes", (int) size);
            abort();
        }
    }
}
