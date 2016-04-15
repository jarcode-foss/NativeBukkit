#include <jni.h>
#include <jni_md.h>

#include <stdlib.h>
#include <stdio.h>

#include <jutils.h>

#include <hooks.h>
#include <nbukkit_impl.h>

/* entry JNI function called when this library is loaded */
JNIEXPORT void JNICALL Java_jni_JNIEntry_entry(JNIEnv* env, jobject this) {
    nb_log(&nb_stub, "initializing...");
    hk_resolveall(env);
}
