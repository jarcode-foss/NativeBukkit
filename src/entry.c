#include <jni.h>
#include <jni_md.h>

#include <stdlib.h>
#include <stdio.h>


/* entry JNI function called when this library is loaded */
JNIEXPORT void JNICALL Java_jni_JNIEntry_entry(JNIEnv* env, jobject this) {
    puts("\n[C] initializing...\n");
}
