
/* Core NativeBukkit definitions and macros */

#ifndef NBUKKIT_H
#define NBUKKIT_H

#define NB_API __attribute__ ((visibility("default")))
#define NB_SYM NB_API __attribute__ ((noinline))

#define NB_ENABLE_SYM nb_enable_hook
#define NB_DISABLE_SYM nb_disable_hook
#define NB_LOAD_SYM nb_load_hook

#define NB_ENABLE_DEF(arg) NB_SYM void NB_ENABLE_SYM (nb_state* arg)
#define NB_DISABLE_DEF(arg) NB_SYM void NB_DISABLE_SYM (nb_state* arg)
#define NB_LOAD_DEF(arg) NB_SYM void NB_LOAD_SYM (nb_state* arg)

/* exception state */
typedef struct {
    int type;
    char* reason;
} nb_ex;

/* plugin-specific state */
typedef struct {
#define NB_STATE_MEMBERS                        \
    const char *const name;                     \
    nb_ex ex;
    NB_STATE_MEMBERS
} nb_state;

typedef void (*nb_fenable) (nb_state*);
typedef void (*nb_fdisable) (nb_state*);
typedef void (*nb_fload) (nb_state*);

/* logging functions; these do not interact with Bukkit's logging */
NB_API void nb_logf(nb_state* state, const char* format, ...);
NB_API void nb_log(nb_state* state, const char* info);

NB_API void* nb_alloc(nb_state* state, size_t size);
NB_API void* nb_realloc(nb_state* state, void* ptr, size_t size);
NB_API void nb_free(nb_state*, void* ptr);

#endif /* NBUKKIT_H */
