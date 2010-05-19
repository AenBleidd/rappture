#include "jRpUtils.h"
#include "rappture.h"

JNIEXPORT void JNICALL Java_rappture_Utils_jRpUtilsProgress
  (JNIEnv *env, jclass cls, jint percent, jstring javaText){
  const char* nativeText = env->GetStringUTFChars(javaText, 0);
  Rappture::Utils::progress(percent, nativeText);
  env->ReleaseStringUTFChars(javaText, nativeText);
  return;
}

