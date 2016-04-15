
/* Core NativeBukkit type definitions and macros */

#ifndef NBUKKIT_H
#define NBUKKIT_H

#ifdef __cplusplus
#define NB_VISIBLE extern "C" __attribute__ ((visibility("default")))
#else
#define NB_VISIBLE __attribute__ ((visibility("default")))
#endif
#define NB_SYM NB_VISIBLE __attribute__ ((noinline))

/* compatibility version; this is incremented when an introduced change breaks
   this API's binary compatibility. */
#define NB_COMPAT_VERSION 3

/* macros for symbols that NativeBukkit requires for a library to be loaded */
#define NB_ENABLE_SYM nb_enable_hook
#define NB_DISABLE_SYM nb_disable_hook
#define NB_LOAD_SYM nb_load_hook
#define NB_VERSION_SYM nb_version

/* define symbol for NB_COMAPT_VERSION */
#define NB_VERSION_DEF() NB_VISIBLE int NB_VERSION_SYM = NB_COMPAT_VERSION

/* define enable, disable, and load functions*/
#define NB_ENABLE_DEF() NB_SYM void NB_ENABLE_SYM (void)
#define NB_DISABLE_DEF() NB_SYM void NB_DISABLE_SYM (void)
#define NB_LOAD_DEF(arg, api) NB_SYM void NB_LOAD_SYM (nb_state* arg, nb_api* api)

#define NBEX_INTERNAL 4  /* internal error or exception in the server, reason is set */
#define NBEX_GENFAIL 3   /* generic failure, reason is not set */
#define NBEX_BADARGS 2   /* bad arguments, reason is not set */
#define NBEX_UNKNOWN 1   /* unknown error, reason is not set */

#define NB_COMPLAIN(state, api)                                         \
    api->logf(state, "ERROR: %s", state->ex.type == NBEX_INTERNAL ? state->ex.reason \
              : "bad arguments or nonstandard error")

/* exception state */
typedef struct {
    int type;
    char* reason;
} nb_ex;

/* plugin-specific state */
typedef struct {
#define NB_STATE_MEMBERS                  \
    const char const* name;               \
    nb_ex ex;
    NB_STATE_MEMBERS
} nb_state;

struct nb_vtsender;

/* interface for anything that can send commands, recieve messages, and have permissions */
typedef struct {
    void* impl;       /* implementation */
    struct nb_vtsender* vt;  /* vtable */
} nb_sender;

typedef struct nb_vtsender {
    void         (*send)    (nb_sender self, const char* cmd);
    const char*  (*name)    (nb_sender self);
    int          (*hasperm) (nb_sender self, const char* perm);
} nb_vtsender;

/* function typedef for handling commands; implementations of this function should not use
   any of the pointer arguments after it has returned */
typedef void (*nb_cmdhandler) (nb_sender* sender, int argc, char** argv);

/* primary API interface, static once handed to plugin in enable() */
typedef struct {
    /* logging functions; these do not interact with Bukkit's logging */
    void (*logf)      (nb_state* state, const char* format, ...);
    void (*log)       (nb_state* state, const char* info);

    /* managed allocation functions (stubs) */
    void* (*alloc)    (nb_state* state, size_t size);
    void* (*realloc)  (nb_state* state, void* ptr, size_t size);
    void  (*free)     (nb_state* state, void* ptr);
    
    /* the following functions can set state->ex if a non-fatal error occurred */
    
    /* register a command */
    void  (*cmdreg)   (nb_state* state, const char* cmd, nb_cmdhandler handler);
    
    /* register a listener  */
    void  (*lreg)     (nb_state* state, short type, short priority, void (*handle) (void* event));

    /* register a task; if period > 0, the task will be repeating */
    void*  (*treg)     (nb_state* state, int delay, int period, void* udata, void (*task) (void* udata));
    void   (*tcancel)  (nb_state* state, void* handle);

    /* unsafe functions */
    struct {
        /* function to obtain JNIEnv* pointer for main server thread.
           returns NULL if not backed by a craftbukkit/spigot server */
        void* (*java_env) (nb_state* state);
        /* function to obtain jobject (not a pointer, cast straight to
           jobject), as a Runnable instance */
        void* (*java_runnable) (nb_state* state, void* udata, void (*task) (void* udata));
        void  (*java_setex)    (nb_state* state);
    } unsafe;
} nb_api;

typedef void (*nb_fenable) (void);
typedef void (*nb_fdisable) (void);
typedef void (*nb_fload) (nb_state*, nb_api* api);

#endif /* NBUKKIT_H */
