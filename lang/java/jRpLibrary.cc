#include "jRpLibrary.h"
#include "rappture.h"

// constructor
JNIEXPORT jlong JNICALL Java_rappture_Library_jRpLibrary
  (JNIEnv *env, jobject obj, jstring javaPath){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  RpLibrary* lib = new RpLibrary(nativePath);
  env->ReleaseStringUTFChars(javaPath, nativePath);
  return (jlong)lib;
}

// Pseudo-destructor
JNIEXPORT void JNICALL Java_rappture_Library_jRpDeleteLibrary
  (JNIEnv *env, jobject obj, jlong libPtr){
  delete (RpLibrary*) libPtr;
  return;
}

// getDouble
JNIEXPORT jdouble JNICALL Java_rappture_Library_jRpGetDouble
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  double retDVal = ((RpLibrary*)libPtr)->getDouble(nativePath);
  env->ReleaseStringUTFChars(javaPath, nativePath);
  return (jdouble)retDVal;
}

// getString
JNIEXPORT jstring JNICALL Java_rappture_Library_jRpGetString
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  std::string retStr = ((RpLibrary*)libPtr)->getString(nativePath);
  env->ReleaseStringUTFChars(javaPath, nativePath);
  return(env->NewStringUTF(retStr.c_str()));
}

// putDouble
JNIEXPORT void JNICALL Java_rappture_Library_jRpPutDouble
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath, 
   jdouble value, jboolean append){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  ((RpLibrary*)libPtr)->put(nativePath, value, "", (int)append);
  env->ReleaseStringUTFChars(javaPath, nativePath);
}

// put
JNIEXPORT void JNICALL Java_rappture_Library_jRpPut
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath, jstring javaValue, jboolean append){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  const char* nativeValue = env->GetStringUTFChars(javaValue, 0);
  ((RpLibrary*)libPtr)->put(nativePath, nativeValue, "", (int)append);
  env->ReleaseStringUTFChars(javaPath, nativePath);
  env->ReleaseStringUTFChars(javaValue, nativeValue);
}


// result
JNIEXPORT void JNICALL Java_rappture_Library_jRpResult
  (JNIEnv *env, jobject obj, jlong libPtr, jint exitStatus){
  ((RpLibrary*)libPtr)->result(exitStatus);
}


