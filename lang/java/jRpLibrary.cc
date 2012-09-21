/* * ======================================================================
 *  AUTHOR:  Ben Rafferty, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

/*
 * This file defines the native functions which are called by the java Library
 * class methods, and in turn call the corresponding rappture RpLibrary methods.
 */

#include "jRpLibrary.h"
#include "rappture.h"

/*
 * Constructor.  Creates a C++ RpLibrary object and returns its address to 
 * java as a long int.
 */
JNIEXPORT jlong JNICALL Java_rappture_Library_jRpLibrary
  (JNIEnv *env, jobject obj, jstring javaPath){
  RpLibrary* lib = NULL;
  const char* nativePath;
  jclass ex;
  
  if (javaPath == NULL){
    lib = new RpLibrary();
  }
  else{
    nativePath = env->GetStringUTFChars(javaPath, 0);
    lib = new RpLibrary(nativePath);
    env->ReleaseStringUTFChars(javaPath, nativePath);
  }

  if (lib == NULL){
    ex = env->FindClass("java/lang/NullPointerException");
    if (ex){
      env->ThrowNew(ex, "Could not create rappture library.");
    }
    env->DeleteLocalRef(ex);
  }

  return (jlong)lib;
}

/*
 * Destructor.  This function is called by the java Library class's
 * finalizer.  Deletes the C++ RpLibrary object when the java Library
 * is garbage collected.
 */
JNIEXPORT void JNICALL Java_rappture_Library_jRpDeleteLibrary
  (JNIEnv *env, jobject obj, jlong libPtr){
  delete (RpLibrary*) libPtr;
  return;
}

// getData 
JNIEXPORT jbyteArray JNICALL Java_rappture_Library_jRpGetData
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  Rappture::Buffer buf = ((RpLibrary*)libPtr)->getData(nativePath);
  size_t size = buf.size();
  _jbyteArray* jbuf = env->NewByteArray(size);
  env->SetByteArrayRegion(jbuf, 0, size, (const jbyte*)buf.bytes());
  env->ReleaseStringUTFChars(javaPath, nativePath);
  return jbuf;
}

// getDouble
JNIEXPORT jdouble JNICALL Java_rappture_Library_jRpGetDouble
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  double retDVal = ((RpLibrary*)libPtr)->getDouble(nativePath);
  env->ReleaseStringUTFChars(javaPath, nativePath);
  return (jdouble)retDVal;
}

// getInt
JNIEXPORT jint JNICALL Java_rappture_Library_jRpGetInt
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  int retIVal = ((RpLibrary*)libPtr)->getInt(nativePath);
  env->ReleaseStringUTFChars(javaPath, nativePath);
  return (jint)retIVal;
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

// putData
JNIEXPORT void JNICALL Java_rappture_Library_jRpPutData
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath,
    jbyteArray jb, jint nbytes, jboolean append){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  jbyte* b = env->GetByteArrayElements(jb, NULL);
  ((RpLibrary*)libPtr)->putData(nativePath, (const char*)b, 
                                nbytes, (int)append);
  env->ReleaseByteArrayElements(jb, b, 0);
  env->ReleaseStringUTFChars(javaPath, nativePath);
}

// putFile
JNIEXPORT void JNICALL Java_rappture_Library_jRpPutFile
  (JNIEnv *env, jobject obj, jlong libPtr, jstring javaPath, 
   jstring javaFileName, jboolean compress, jboolean append){
  const char* nativePath = env->GetStringUTFChars(javaPath, 0);
  const char* nativeFileName = env->GetStringUTFChars(javaFileName, 0);
  ((RpLibrary*)libPtr)->putFile(nativePath, nativeFileName, 
                                (int)compress, (int)append);
  env->ReleaseStringUTFChars(javaPath, nativePath);
  env->ReleaseStringUTFChars(javaFileName, nativeFileName);
}

// result
JNIEXPORT void JNICALL Java_rappture_Library_jRpResult
  (JNIEnv *env, jobject obj, jlong libPtr, jint exitStatus){
  ((RpLibrary*)libPtr)->result(exitStatus);
}

// xml
JNIEXPORT jstring JNICALL Java_rappture_Library_jRpXml
  (JNIEnv *env, jobject obj, jlong libPtr) {
  std::string retStr = ((RpLibrary*)libPtr)->xml();
  return(env->NewStringUTF(retStr.c_str()));
}

