
/* Core NativeBukkit definitions and macros */

#ifndef NBUKKIT_H
#define NBUKKIT_H

#define NB_API __attribute__ ((visibility("default")))
#define NB_SYM NB_API __attribute__ ((noinline))

#define NB_ENABLE_DEF(arg) NB_SYM void nb_enable_hook(nb_state* arg)
#define NB_DISABLE_DEF(arg) NB_SYM void nb_disable_hook(nb_state* arg)
#define NB_LOAD_DEF(arg) NB_SYM void nb_disable_hook(nb_state* arg)

/* exception state */
typedef struct {
    int type;
    char* reason;
} nb_ex;

/* plugin-specific state */
typedef struct {
#define NB_STATE_MEMBERS                        \
    const short idx;                            \
    const char *const name;                     \
    nb_ex ex;
    NB_STATE_MEMBERS
} nb_state;

/* logging functions; these do not interact with Bukkit's logging */
NB_API void nb_logf(nb_state* state, const char* format, ...);
NB_API void nb_log(nb_state* state, const char* info);

#endif /* NBUKKIT_H */
