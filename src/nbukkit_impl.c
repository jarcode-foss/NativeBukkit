#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <time.h>

#include <jni.h>
#include <jni_md.h>

#include <nbukkit_impl.h>

@ {
    #include <nbukkit.h>
    typedef struct {
        NB_STATE_MEMBERS
        JNIEnv* env;
    } nb_istate;
}

#define LOG_BUFSIZE 1024
#define LOG_TIMEFORMAT "%H:%M:%S"

void nb_logf(nb_state* state, const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    char buf[LOG_BUFSIZE];
    time_t raw;
    time(&raw);
    struct tm spec;
    localtime_r(&raw, &spec);
    size_t off = strftime(buf, sizeof(buf) - 1, "[" LOG_TIMEFORMAT "]", &spec);
    off += (size_t) snprintf(buf + off, sizeof(buf) - (off + 1), "[N:%s] ", state->name);
    off += (size_t) vsnprintf(buf + off, sizeof(buf) - (off + 1), format, argptr);
    va_end(argptr);
    buf[off] = '\0';
    fputs(buf, stdout);
}

void nb_log(nb_state* state, const char* info) {
    char buf[LOG_BUFSIZE];
    time_t raw;
    time(&raw);
    struct tm spec;
    localtime_r(&raw, &spec);
    size_t off = strftime(buf, sizeof(buf) - 1, "[" LOG_TIMEFORMAT "]", &spec);
    off += (size_t) snprintf(buf + off, sizeof(buf) - (off + 1), "[N:%s] ", state->name);
    off += (size_t) strncpy(buf + off, info, sizeof(buf) - (off + 1));
    buf[off] = '\0';
    fputs(buf, stdout);
}
