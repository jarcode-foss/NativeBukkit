#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <time.h>

#include <jni.h>
#include <jni_md.h>

#include <jutils.h>
#include <nbukkit_impl.h>

@ {
    #include <nbukkit.h>
    typedef struct {
        NB_STATE_MEMBERS
        JNIEnv* env;
        nb_fenable enable;
        nb_fdisable disable;
        nb_fload load;
    } nb_istate;
}

static void* unsafe_jenv(nb_state* state) {
    return ((nb_istate*) state)->env;
}

@(extern) nb_state nb_stub = { .name = "loader" };

@(extern) nb nb_api = {
    .logf = &nb_logf,
    .log = &nb_log,
    .alloc = &nb_alloc,
    .realloc = &nb_realloc,
    .free = &nb_free,
    .unsafe = {
        .java_env = &unsafe_jenv
    }
};

#define LOG_BUFSIZE 1024
#define LOG_TIMEFORMAT "%H:%M:%S"

@() void nb_logf(nb_state* state, const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    char buf[LOG_BUFSIZE];
    time_t raw;
    time(&raw);
    struct tm spec;
    localtime_r(&raw, &spec);
    size_t off = strftime(buf, sizeof(buf) - 1, "[" LOG_TIMEFORMAT "]", &spec);
    off += (size_t) snprintf(buf + off, sizeof(buf) - (off + 1), "[N:%s] ",
                             state ? state->name : "!");
    off += (size_t) vsnprintf(buf + off, sizeof(buf) - (off + 1), format, argptr);
    va_end(argptr);
    buf[off] = '\0';
    fputs(buf, state ? stdout : stderr);
    fputc('\n', state ? stdout : stderr);
}

@() void nb_log(nb_state* state, const char* info) {
    char buf[LOG_BUFSIZE];
    time_t raw;
    time(&raw);
    struct tm spec;
    localtime_r(&raw, &spec);
    size_t off = strftime(buf, sizeof(buf) - 1, "[" LOG_TIMEFORMAT "]", &spec);
    off += (size_t) snprintf(buf + off, sizeof(buf) - (off + 1), "[N:%s] ",
                             state ? state->name : "!");
    off += (size_t) strncpy(buf + off, info, sizeof(buf) - (off + 1));
    buf[off] = '\0';
    fputs(buf, stdout ? stdout : stderr);
    fputc('\n', stdout ? stdout : stderr);
}

/*
  These are currently checked wrappers for malloc/free, but ideally should be using a custom allocator
  with its own heap (checked mmap syscalls?)
*/

@() void* nb_alloc(nb_state* state, size_t size) {
    void* ret;
    if (!(ret = malloc(size))) {
        nb_logf(NULL, "failed to allocate %n bytes", (int) size);
        ju_fatal(((nb_istate*) state)->env);
    }
    return ret;
}

@() void* nb_realloc(nb_state* state, void* ptr, size_t size) {
    void* ret;
    if (!(ret = realloc(ptr, size))) {
        nb_logf(NULL, "failed to re-allocate %n bytes at %p", (int) size, ptr);
        ju_fatal(((nb_istate*) state)->env);
    }
    return ret;
}

@() void nb_free(nb_state* state, void* ptr) {
    return free(ptr);
}
