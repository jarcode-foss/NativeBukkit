#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <dlfcn.h>

#include <jni.h>
#include <jni_md.h>

#include <cutils.h>
#include <jutils.h>
#include <nbukkit_impl.h>
#include <plugin.h>

/* pl: JNIPlugin implementation */

#define STRING(x) #x
#define MSTRING(x) STRING(x)

static jclass type;
static jfieldID id_internal;
static jfieldID id_handle;
static jfieldID id_name;
static jfieldID id_plugin;

@(extern) ju_hook pl_hooks[] = {
    { "jni/JNIPlugin", NULL, &type, JU_CLASS },
    { "__internal", "J", &id_internal, JU_FIELD },
    { "__handle", "J", &id_handle, JU_FIELD },
    { "__name", "Ljava/lang/String;", &id_name, JU_FIELD },
    { "__plugin", "Lorg/bukkit/plugin/Plugin;", &id_plugin, JU_FIELD },
    JU_NULL
};

JNIEXPORT JNICALL jlong Java_jni_JNIPlugin_open(JNIEnv* e, jclass type, jstring jstr) {
    if (!jstr) goto badarg;
    const char* str = (*e)->GetStringUTFChars(e, jstr, NULL);
    if (!str) goto badarg;
    void* handle;
    if (!(handle = dlopen(str, RTLD_LAZY))) {
        ju_throwf(e, "dlopen() failed: %s", dlerror());
        handle = NULL;
    } else nb_logf(&nb_stub, "obtained handle for %s at %p", str, handle);
    (*e)->ReleaseStringUTFChars(e, jstr, str);
    return (jlong) (intptr_t) handle;
 badarg:
    ju_throw(e, "cannot make call to dlopen() with NULL argument");
    return 0;
}

JNIEXPORT JNICALL void Java_jni_JNIPlugin_onLoad(JNIEnv* e, jobject this) {
    nb_istate* self = smalloc(sizeof(nb_istate));

    jobject plugin = (*e)->GetObjectField(e, this, id_plugin);
    ASSERTEX(e);
    jstring jstr = (*e)->GetObjectField(e, this, id_name);
    ASSERTEX(e);
    
    const char* bstr = (*e)->GetStringUTFChars(e, jstr, NULL);
    strncpy(self->namebuf, bstr, 128);
    (*e)->ReleaseStringUTFChars(e, jstr, bstr);

    *(char**) &self->name = self->namebuf;
    self->env = e;
    self->jplugin = (*e)->NewGlobalRef(e, plugin);
    self->ex.type = 0;
    
    (*e)->SetLongField(e, this, id_internal, (jlong) (intptr_t) self);
    ASSERTEX(e);
    
    void* handle = (void*) (intptr_t) (*e)->GetLongField(e, this, id_handle);
    ASSERTEX(e);
    char* err;
    int compat_version = *(int*) dlsym(handle, MSTRING(NB_VERSION_SYM));
    if ((err = dlerror()))
        goto dlsym_fail;
    if ((compat_version != NB_COMPAT_VERSION))
        goto version_mismatch;
    self->enable = dlsym(handle, MSTRING(NB_ENABLE_SYM));
    if ((err = dlerror()))
        goto dlsym_fail;
    self->disable = dlsym(handle, MSTRING(NB_DISABLE_SYM));
    if ((err = dlerror()))
        goto dlsym_fail;
    self->load = dlsym(handle, MSTRING(NB_LOAD_SYM));
    if ((err = dlerror()))
        goto dlsym_fail;
    
    sw_barrier(); /* aliasing */
    self->load((nb_state*) self, &nb_global_api);
    
    return;
 dlsym_fail:
    ju_throwf(e, "dlsym() failed (%p): %s", handle, err);
    return;
 version_mismatch:
    if (compat_version < NB_COMPAT_VERSION) {
        nb_log(NULL, "\n\n"
               "A plugin failed to load because it was compiled with an older\n"
               "version of the NativeBukkit API. Please update the plugin or\n"
               "report the issue to the author.\n");
    } else {
        nb_log(NULL, "\n\n"
               "A plugin was compiled with a newer version of the NativeBukkit API,\n"
               "this likely means you need to update NativeBukkit.\n");
    }
    ju_throwf(e, "failed to load plugin '%s': bad API compatibility version (got %d, expected %d)",
              self->name, compat_version, NB_COMPAT_VERSION);
    return;
}

JNIEXPORT JNICALL void Java_jni_JNIPlugin_onEnable(JNIEnv* e, jobject this) {
    nb_istate* self = (nb_istate*) (intptr_t) (*e)->GetLongField(e, this, id_internal);
    ASSERTEX(e);
    self->enable();
}

JNIEXPORT JNICALL void Java_jni_JNIPlugin_onDisable(JNIEnv* e, jobject this) {
    nb_istate* self = (nb_istate*) (intptr_t) (*e)->GetLongField(e, this, id_internal);
    ASSERTEX(e);
    self->disable();
}

JNIEXPORT JNICALL void Java_jni_JNIPlugin_close(JNIEnv* e, jobject this) {
    nb_istate* self = (nb_istate*) (intptr_t) (*e)->GetLongField(e, this, id_internal);
    void* handle = (void*) (intptr_t) (*e)->GetLongField(e, this, id_handle);
    ASSERTEX(e);
    if (dlclose(handle)) {
        ju_throwf(e, "dlclose() failed (%p): %s:", handle, dlerror());
        return;
    }
    if (self) {
        free(self);
        (*e)->SetLongField(e, this, id_internal, (jlong) 0);
        ASSERTEX(e);
    }
}
