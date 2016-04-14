#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <dlfcn.h>

#include <jni.h>
#include <jni_md.h>

#include <nbukkit_impl.h>

#include <cutils.h>
#include <jutils.h>
#include <plugin.h>

/* pl: JNIPlugin implementation */

#define MSTRING(x) #x

static jclass type;
static jfieldID id_internal;
static jfieldID id_handle;
static jfieldID id_name;

@(extern) ju_hook pl_hooks[] = {
    { "jni/JNIPlugin", NULL, &type, JU_CLASS },
    { "__internal", "J", &id_internal, JU_FIELD },
    { "__handle", "J", &id_handle, JU_FIELD },
    { "__name", "Ljava/lang/String;", &id_name, JU_FIELD },
    JU_NULL
};

JNIEXPORT JNICALL jlong Java_jni_JNIPlugin_open(JNIEnv* env, jobject jstr) {
    const char* str = (*env)->GetStringUTFChars(env, jstr, NULL);
    ASSERTEX(env);
    void* handle;
    if ((handle = dlopen(str, RTLD_LAZY))) {
        ju_throwf(env, "dlopen() failed: %s", dlerror());
        handle = NULL;
    }
    (*env)->ReleaseStringUTFChars(env, jstr, str);
    ASSERTEX(env);
    return (jlong) (intptr_t) handle;
}

JNIEXPORT JNICALL void Java_jni_JNIPlugin_onLoad(JNIEnv* env, jobject this) {
    nb_istate* self = smalloc(sizeof(nb_istate));

    jstring jstr = (*env)->GetObjectField(env, this, id_name);
    ASSERTEX(env);
    *((const char**) &self->name) = (*env)->GetStringUTFChars(env, jstr, NULL);
    self->env = env;
    (*env)->SetLongField(env, this, id_internal, (jlong) (intptr_t) self);
    ASSERTEX(env);
    
    void* handle = (void*) (intptr_t) (*env)->GetLongField(env, this, id_handle);
    ASSERTEX(env);
    char* err;
    self->enable = dlsym(handle, MSTRING(NB_ENABLE_SYM));
    if ((err = dlerror())) goto ex;
    self->disable = dlsym(handle, MSTRING(NB_DISABLE_SYM));
    if ((err = dlerror())) goto ex;
    self->load = dlsym(handle, MSTRING(NB_LOAD_SYM));
    if ((err = dlerror())) goto ex;
    
    sw_barrier(); /* aliasing */
    self->load((nb_state*) self, &nb_api);
    
    return;
 ex:
    ju_throwf(env, "dlsym() failed: ", err);
}

JNIEXPORT JNICALL void Java_jni_JNIPlugin_onEnable(JNIEnv* env, jobject this) {
    nb_istate* self = (nb_istate*) (intptr_t) (*env)->GetLongField(env, this, id_internal);
    ASSERTEX(env);
    self->enable((nb_state*) self);
}

JNIEXPORT JNICALL void Java_jni_JNIPlugin_onDisable(JNIEnv* env, jobject this) {
    nb_istate* self = (nb_istate*) (intptr_t) (*env)->GetLongField(env, this, id_internal);
    ASSERTEX(env);
    self->disable((nb_state*) self);
}

JNIEXPORT JNICALL void Java_jni_JNIPlugin_close(JNIEnv* env, jobject this) {
    nb_istate* self = (nb_istate*) (intptr_t) (*env)->GetLongField(env, this, id_internal);
    void* handle = (void*) (intptr_t) (*env)->GetLongField(env, this, id_handle);
    ASSERTEX(env);
    if (dlclose(handle)) {
        ju_throwf(env, "dlclose() failed (%p): %s:", handle, dlerror());
    }
    if (self) {
        free(self);
        (*env)->SetLongField(env, this, id_internal, (jlong) 0);
    }
}
