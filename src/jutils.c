#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <jni.h>
#include <jni_md.h>

#include <nbukkit_impl.h>
#include <jutils.h>

/* ju: java utils */

static jclass exclass;

@ {
    #define JU_DEBUG 0
    
    #if JU_DEBUG >= 1
    #define ASSERTEX(e) \
        do { if ((*e)->ExceptionCheck(e) == JNI_TRUE) { ju_afatal(e, __func__ ":" __LINE__); } } while (0)
    #else
    #define ASSERTEX(e) do {} while (0)
    #endif

    #define CHECKEX(e, b) do { if ((*e)->ExceptionCheck(e) == JNI_TRUE) { goto b; } } while (0)

    #define JU_NULL { NULL, NULL, NULL, NONE }

    typedef enum {
        JU_CLASS, JU_METHOD, JU_STATIC_METHOD, JU_FIELD, NONE
    } ju_hooktype;
    
    typedef struct {
        const char* name; /* name of class or member */
        const char* sig;  /* if memeber, its signature */
        void* mem;        /* pointer to memory storage for class or id */
        ju_hooktype type; /* type of hook */
    } ju_hook;
}

@(extern) ju_hook ju_hooks[] = {
    { "jni/NativeException", NULL, &exclass, JU_CLASS }, JU_NULL
};

/* Fatal errors should be used sparingly! They are only for when classes and members cannot be resolved, or
   for handling exceptions that _should_ not be raised when calling into Java. Checked exceptions should
   always stop the current API call, and then set the error state accordingly for a plugin's nb_state */

/* fatal error from ASSERTEX, expects a pending exception */
@(__attribute__ ((noreturn))) void ju_afatal(JNIEnv* env, char* info) {
    nb_logf(NULL, "JNI Assertion failed (%s)", info);
    (*env)->ExceptionDescribe(env);
    abort();
}

/* fatal error, usually unhandled exception or issues with getting handles/ids */
@(__attribute__ ((noreturn))) void ju_fatal(JNIEnv* env) {
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        nb_log(NULL, "Encountered an unhandled, fatal exception:");
        (*env)->ExceptionDescribe(env);
    } else nb_log(NULL, "Encountered a fatal error, aborting");
    abort();
}

@() jmethodID ju_resolvem(JNIEnv* env, jclass type, const char* method, const char* signature) {
    jmethodID ret = (*env)->GetMethodID(env, type, method, signature);
    CHECKEX(env, ex);
    return ret;
 ex:
    ju_fatal(env);
}

@() jfieldID ju_resolvef(JNIEnv* env, jclass type, const char* field, const char* signature) {
    jfieldID ret = (*env)->GetFieldID(env, type, field, signature);
    CHECKEX(env, ex);
    return ret;
 ex:
    ju_fatal(env);
}

@() jmethodID ju_resolvesm(JNIEnv* env, jclass type, const char* method, const char* signature) {
    jmethodID ret = (*env)->GetStaticMethodID(env, type, method, signature);
    CHECKEX(env, ex);
    return ret;
 ex:
    ju_fatal(env);
}

/* handle 2D array of hooks, null terminated */
@() void ju_resb(JNIEnv* env, ju_hook** hook_sets) {
    ju_hook* arr;
    size_t i;
    for (i = 0; (arr = hook_sets[i]) != NULL; ++i) {
        ju_res(env, arr);
    }
}

/* handle array of hooks, null terminated */
@() void ju_res(JNIEnv* env, ju_hook* hooks) {
    ju_hook* at;
    size_t i;
    jclass last = NULL;
    for (i = 0; (at = hooks + i)->name != NULL; ++i) {
        switch (at->type) {
        case JU_CLASS:
            last = ju_classreg(env, at->name, (jclass*) at->mem);
            break;
        case JU_STATIC_METHOD:
            *((jmethodID*) at->mem) = ju_resolvesm(env, last, at->name, at->sig);
            break;
        case JU_METHOD:
            *((jmethodID*) at->mem) = ju_resolvem(env, last, at->name, at->sig);
            break;
        case JU_FIELD:
            *((jfieldID*) at->mem) = ju_resolvef(env, last, at->name, at->sig);
            break;
        case NONE: break;
        }
    }
}

@() jclass ju_classreg(JNIEnv* env, const char* name, jclass* mem) {
    jobject local = (*env)->FindClass(env, name);
    CHECKEX(env, ex);
    if (local == NULL) {
        ju_fatal(env);
    }
    *mem = (*env)->NewGlobalRef(env, local);
    CHECKEX(env, ex);
    (*env)->DeleteLocalRef(env, local);
    CHECKEX(env, ex);
    if (!(*mem)) goto ex;
    return *mem;
 ex:
    ju_fatal(env);
}

@() int ju_throwf(JNIEnv* env, const char* format, ...) {
    va_list argptr;
    jint ret;
    va_start(argptr, format);
    char buf[128];
    vsnprintf(buf, 128, format, argptr);
    ret = exclass ? (*env)->ThrowNew(env, exclass, buf) : 0;
    va_end(argptr);
    return ret;
}

@() int ju_throw(JNIEnv* env, const char* message) {
    if (exclass)
        return (*env)->ThrowNew(env, exclass, message);
    else return 0;
}
