#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <cutils.h>

@ {
    #define sw_barrier() asm volatile("": : :"memory")
    #define hw_barrier() __sync_synchronize
    
    /* On Linux, malloc is extremely unlikely to fail */
    
    #ifdef __linux__
    #define smalloc(size) malloc(size)
    #else
    #define smalloc(size) __safe_malloc(size)
    #endif
    
    /* For other *nix variants */
    
    static inline void* __safe_malloc(size_t size) {
        void* ret;
        if (!(ret = malloc(size))) {
            fprintf(stderr, "malloc(): failed to allocate %d bytes", (int) size);
            abort();
        }
    }
}
