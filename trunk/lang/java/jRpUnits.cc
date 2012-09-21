/*
 * ======================================================================
 *  AUTHOR:  Ben Rafferty, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

/*
 * This file defines the native functions which are called by the java Units
 * class methods, and in turn call the corresponding rappture RpUnits methods.
 */

#include "jRpUnits.h"
#include "rappture.h"

// convertString
JNIEXPORT jstring JNICALL Java_rappture_Units_jRpUnitsConvert
  (JNIEnv *env, jclass cls, jstring javaFromVal, jstring javaTo, jboolean units){
  const char* nativeFromVal = env->GetStringUTFChars(javaFromVal, 0);
  const char* nativeTo = env->GetStringUTFChars(javaTo, 0);
  int err;
  jclass ex;
  std::string errorMsg;

  // perform conversion
  std::string retStr = RpUnits::convert(nativeFromVal, nativeTo,
                                       (int)units, &err);

  // raise a runtime exception on error
  if (err){
    ex = env->FindClass("java/lang/RuntimeException");
    if (ex){
      errorMsg = "Cannot convert ";
      errorMsg += nativeFromVal;
      errorMsg += " to ";
      errorMsg += nativeTo;
      env->ThrowNew(ex, errorMsg.c_str());
    }
    env->DeleteLocalRef(ex);
  }

  env->ReleaseStringUTFChars(javaFromVal, nativeFromVal);
  env->ReleaseStringUTFChars(javaTo, nativeTo);

  // create new java string and return
  return (env->NewStringUTF(retStr.c_str()));
} 

