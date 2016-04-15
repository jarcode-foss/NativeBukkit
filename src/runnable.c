#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <jni.h>
#include <jni_md.h>

#include <jutils.h>

#include <runnable.h>

/* jrn: java runnable */

static jclass type;
static jmethodID id_init;
static jfieldID id_udata;
static jfieldID id_handle;

@(extern) ju_hook jrn_hooks[] = {
    { "jni/JNIRunnable", NULL, &type, JU_CLASS },
    { "<init>", "(JJ)V", &id_init, JU_METHOD },
    { "__udata", "J", &id_udata, JU_FIELD },
    { "__handle", "J", &id_handle, JU_FIELD },
    JU_NULL
};

JNIEXPORT JNICALL void Java_jni_JNIRunnable_run(JNIEnv* env, jobject this) {
    void (*handle) (void*) = (void (*) (void*))
        (intptr_t) (*env)->GetLongField(env, this, id_handle);
    void* udata = (void*) (intptr_t) (*env)->GetLongField(env, this, id_udata);
    handle(udata);
}

@() jobject jrn_new(JNIEnv* env, void* udata, void (*ptr) (void* udata)) {
    jobject ret = (*env)->NewObject(env, type, id_init,
                             (jlong) (intptr_t) udata,
                             (jlong) (intptr_t) ptr);
    ASSERTEX(env);
    return ret;
}
