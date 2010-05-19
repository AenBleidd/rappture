#include "jRpUnits.h"
#include "rappture.h"

JNIEXPORT jstring JNICALL Java_rappture_Units_jRpUnitsConvert
  (JNIEnv *env, jclass cls, jstring javaFromVal, jstring javaTo, jboolean units){
  const char* nativeFromVal = env->GetStringUTFChars(javaFromVal, 0);
  const char* nativeTo = env->GetStringUTFChars(javaTo, 0);
  std::string retStr = RpUnits::convert(nativeFromVal, nativeTo, (int)units);
  env->ReleaseStringUTFChars(javaFromVal, nativeFromVal);
  env->ReleaseStringUTFChars(javaTo, nativeTo);
  return (env->NewStringUTF(retStr.c_str()));
} 

