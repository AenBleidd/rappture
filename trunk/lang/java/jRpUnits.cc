/*
 * ======================================================================
 *  AUTHOR:  Ben Rafferty, Purdue University
 *  Copyright (c) 2010  Purdue Research Foundation
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

// convert
JNIEXPORT jstring JNICALL Java_rappture_Units_jRpUnitsConvert
  (JNIEnv *env, jclass cls, jstring javaFromVal, jstring javaTo, jboolean units){
  const char* nativeFromVal = env->GetStringUTFChars(javaFromVal, 0);
  const char* nativeTo = env->GetStringUTFChars(javaTo, 0);
  std::string retStr = RpUnits::convert(nativeFromVal, nativeTo, (int)units);
  env->ReleaseStringUTFChars(javaFromVal, nativeFromVal);
  env->ReleaseStringUTFChars(javaTo, nativeTo);
  return (env->NewStringUTF(retStr.c_str()));
} 

