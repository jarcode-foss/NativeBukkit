
#include <jni.h>
#include <jni_md.h>

#include <jutils.h>
#include <plugin.h>
#include <runnable.h>

#include <nbukkit_impl.h>
#include <hooks.h>

static ju_hook* hooks[] = {
    ju_hooks, /* needs to be resolved first, otherwise exceptions won't work */
    pl_hooks,
    jrn_hooks,
    NULL
};

@() void hk_resolveall(JNIEnv* env) {
    nb_log(&nb_stub, "resolving JNI classes and members...");
    ju_resb(env, hooks);
}
