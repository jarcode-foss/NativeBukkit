#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#include <time.h>

#include <jni.h>
#include <jni_md.h>

#include <cutils.h>
#include <jutils.h>

#include <runnable.h>
#include <nbukkit_impl.h>

@ {
    #include <nbukkit.h>
    typedef struct {
        NB_STATE_MEMBERS
        JNIEnv* env;
        jobject jplugin;
        nb_fenable enable;
        nb_fdisable disable;
        nb_fload load;
        char exbuf[1024];
        char namebuf[128];
    } nb_istate;
}

#define LOG_BUFSIZE 1024
#define LOG_TIMEFORMAT "%H:%M:%S"

static void* unsafe_jenv(nb_state* state) {
    return ((nb_istate*) state)->env;
}

static void* unsafe_jrunnable(nb_state* state, void* udata, void (*ptr) (void* udata)) {
    return jrn_new(((nb_istate*) state)->env, udata, ptr);
}

@(extern) jclass nb_jbukkit;        /* org.bukkit.Bukkit */
static jmethodID id_getsched;       /* static Bukkit#getScheduler() */
static jmethodID id_getsname;       /* static Bukkit#getName() */
static jmethodID id_getsversion;    /* static Bukkit#getVersion() */
static jmethodID id_getsbversion;   /* static Bukkit#getBukkitVersion() */

static jclass class_jsched;         /* BukkitScheduler singleton */
static jmethodID id_schedrepeating; /* BukkitScheduler#scheduleSyncRepeatingTask(...) */
static jmethodID id_scheddelayed;   /* BukkitScheduler#scheduleSyncDelayedTask(...) */
static jmethodID id_schedrm;        /* BukkitScheduler#cancelTask(id) */

static jclass class_type;           /* java.lang.Class */
static jmethodID id_getname;        /* Class#getName() */
static jclass throwable_type;       /* java.lang.Throwable */
static jmethodID id_getmessage;     /* Throwable#getMessage() */

@(extern) ju_hook nb_hooks[] = {
    { "org/bukkit/Bukkit", NULL, &nb_jbukkit, JU_CLASS },
    { "getScheduler", "()Lorg/bukkit/scheduler/BukkitScheduler;", &id_getsched, JU_STATIC_METHOD },
    { "getName", "()Ljava/lang/String;", &id_getsname, JU_STATIC_METHOD },
    { "getVersion", "()Ljava/lang/String;", &id_getsversion, JU_STATIC_METHOD },
    { "getBukkitVersion", "()Ljava/lang/String;", &id_getsbversion, JU_STATIC_METHOD },
    
    { "org/bukkit/scheduler/BukkitScheduler", "J", &class_jsched, JU_CLASS },
    { "scheduleSyncRepeatingTask",
      "(Lorg/bukkit/plugin/Plugin;Ljava/lang/Runnable;JJ)I", &id_schedrepeating, JU_METHOD },
    { "scheduleSyncDelayedTask",
      "(Lorg/bukkit/plugin/Plugin;Ljava/lang/Runnable;J)I", &id_scheddelayed, JU_METHOD },
    { "cancelTask", "(I)V", &id_schedrm, JU_METHOD },
    
    { "java/lang/Class", NULL, &class_type, JU_CLASS },
    { "getName", "()Ljava/lang/String;", &id_getname, JU_METHOD },
    
    { "java/lang/Throwable", NULL, &throwable_type, JU_CLASS },
    { "getMessage", "()Ljava/lang/String;", &id_getmessage, JU_METHOD },
    
    JU_NULL
};

static char bukkit_impl[128];
static char bukkit_ver[128];
static char bukkit_info[128];

/* there is only ever one version that Bukkit will report to us */
static char* bukkit_versions[] = { bukkit_ver, NULL };

@(extern) nb_state nb_stub = { .name = "loader" };

@(extern) nb_api nb_global_api = {
    .impl = bukkit_impl, .impl_versions = bukkit_versions, .impl_extra = bukkit_info,

    .unit = 50000000, .absolute_units = false, /* 50Hz tickrate, tied to server tickrate */
    
    .logf = &nb_logf, .log = &nb_log,
    
    .alloc = &nb_alloc, .realloc = &nb_realloc, .free = &nb_free,
    
    .lreg = &nb_lreg, .treg = &nb_treg, .tcancel = &nb_tcancel,
    
    .unsafe = {
        .java_env = &unsafe_jenv,
        .java_runnable = &unsafe_jrunnable,
        .java_setex = &nb_setex
    }
};

/* global references to singletons that are used in the bukkit API */
@(extern) jobject nb_jsched = NULL;

@() void nb_initsingletons(JNIEnv* e) {
    nb_jsched = (*e)->NewGlobalRef(e, (*e)->CallStaticObjectMethod(e, nb_jbukkit, id_getsched));
    ASSERTEX(e);
    jstring jimpl = (*e)->CallStaticObjectMethod(e, nb_jbukkit, id_getsname);
    ASSERTEX(e);
    jstring jver = (*e)->CallStaticObjectMethod(e, nb_jbukkit, id_getsversion);
    ASSERTEX(e);
    jstring jbver = (*e)->CallStaticObjectMethod(e, nb_jbukkit, id_getsbversion);
    ASSERTEX(e);
    
    if (jimpl) {
        const char* impl = (*e)->GetStringUTFChars(e, jimpl, NULL);
        strncpy(bukkit_impl, impl, 128);
        (*e)->ReleaseStringUTFChars(e, jimpl, impl);
    } else nb_global_api.impl = "NULL";
    
    if (jver) {
        const char* ver = (*e)->GetStringUTFChars(e, jver, NULL);
        strncpy(bukkit_ver, ver, 128);
        (*e)->ReleaseStringUTFChars(e, jver, ver);
    } else nb_global_api.impl_version = "NULL";
               
    if (jbver) {
        const char* bver = (*e)->GetStringUTFChars(e, jbver, NULL);
        strncpy(bukkit_info, bver, 128);
        (*e)->ReleaseStringUTFChars(e, jbver, bver);
    } else bukkit_versions[0] = "NULL";
}

@() void nb_logf(nb_state* state, const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    char buf[LOG_BUFSIZE];
    time_t raw;
    time(&raw);
    struct tm spec;
    localtime_r(&raw, &spec);
    size_t off = strftime(buf, sizeof(buf) - 1, "[" LOG_TIMEFORMAT, &spec);
    off += snprintf(buf + off, sizeof(buf) - (off + 1), " N:%s] ",
                             state ? state->name : "!");
    off += vsnprintf(buf + off, sizeof(buf) - (off + 1), format, argptr);
    va_end(argptr);
    fputs(buf, state ? stdout : stderr);
    fputc('\n', state ? stdout : stderr);
}

@() void nb_log(nb_state* state, const char* info) {
    char buf[LOG_BUFSIZE];
    time_t raw;
    time(&raw);
    struct tm spec;
    localtime_r(&raw, &spec);
    size_t off = strftime(buf, sizeof(buf) - 1, "[" LOG_TIMEFORMAT, &spec);
    off += (size_t) snprintf(buf + off, sizeof(buf) - (off + 1), " N:%s] ",
                             state ? state->name : "!");
    strncpy(buf + off, info, sizeof(buf) - (off + 1));
    fputs(buf, stdout ? stdout : stderr);
    fputc('\n', stdout ? stdout : stderr);
}

@() void nb_lreg(nb_state* state, short type, short priority, void (*handle) (void*)) {
    nb_istate* i = (nb_istate*) state;
    //TODO: finish
}

@() void* nb_treg(nb_state* state, int delay, int period, void* udata, void (*task) (void* udata)) {
    nb_istate* i = (nb_istate*) state;
    JNIEnv* e = i->env;
    
    jobject r = jrn_new(e, udata, task);

    jint ret;
    if (period > 0) {
        ret = (*e)->CallIntMethod(e, nb_jsched, id_schedrepeating, i->jplugin, r, delay, period);
    } else {
        ret = (*e)->CallIntMethod(e, nb_jsched, id_scheddelayed, i->jplugin, r, delay);
    }
    CHECKEX(e, err);
    if (ret == -1) goto bukkit_err;
    return (void*) (uintptr_t) ret; /* lazy solution to return the integer result/handle as a pointer
                                       won't be an issue since sizeof(jint) >= sizeof(void*) */
 err:        /* unchecked exception */
    nb_setex(state);
    return NULL;
 bukkit_err: /* according to the docs, Bukkit can apparently return -1 */
    i->ex.type = NBEX_GENFAIL;
    return NULL;
}

@() void nb_tcancel(nb_state* state, void* handle) {
    nb_istate* i = (nb_istate*) state;
    JNIEnv* e = i->env;
    
    (*e)->CallVoidMethod(e, nb_jsched, id_schedrm, (jint) (uinxtptr_t) handle);
    CHECKEX(e, err);
    return;
 err:
    nb_setex(state);
    return;
}

/*
  These are currently checked wrappers for malloc/free, but ideally should be using a custom allocator
  with its own heap (checked mmap syscalls?)
*/

@() void* nb_alloc(nb_state* state, size_t size) {
    void* ret;
    if (!(ret = smalloc(size))) {
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

@() void nb_setex(nb_state* state) {
    nb_istate* i = (nb_istate*) state;
    JNIEnv* e = i->env;
    
    jthrowable ex = (*e)->ExceptionOccurred(e);
    (*e)->ExceptionClear(e);
    jclass type = (*e)->GetObjectClass(e, ex);
    ASSERTEX(e);
    jstring type_name = (*e)->CallObjectMethod(e, type, id_getname);
    ASSERTEX(e);
    jstring ex_reason = (*e)->CallObjectMethod(e, ex, id_getmessage);
    ASSERTEX(e);

    const char* cname = (*e)->GetStringUTFChars(e, type_name, NULL);
    const char* creason = (*e)->GetStringUTFChars(e, ex_reason, NULL);

    snprintf(i->exbuf, 1024, "%s: %s", cname, creason);

    (*e)->ReleaseStringUTFChars(e, type_name, cname);
    (*e)->ReleaseStringUTFChars(e, ex_reason, creason);
        
    i->ex.type = NBEX_INTERNAL;
    i->ex.reason = i->exbuf;
}
