#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <cutils.h>

@ {
    #define sw_barrier() asm volatile("": : :"memory")
    #define hw_barrier() __sync_synchronize
}
