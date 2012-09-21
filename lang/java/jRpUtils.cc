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
 * This file defines the native functions which are called by the java Utils
 * class methods, and in turn call the corresponding rappture RpUtils methods.
 */

#include "jRpUtils.h"
#include "rappture.h"

// progress
JNIEXPORT void JNICALL Java_rappture_Utils_jRpUtilsProgress
  (JNIEnv *env, jclass cls, jint percent, jstring javaText){
  const char* nativeText = env->GetStringUTFChars(javaText, 0);
  int err = Rappture::Utils::progress(percent, nativeText);
  jclass ex;
  if (err){
    ex = env->FindClass("java/lang/RuntimeException");
    if (ex){
      env->ThrowNew(ex, "rappture.Utils.progress failed.");
    }
    env->DeleteLocalRef(ex);
  }
  env->ReleaseStringUTFChars(javaText, nativeText);
  return;
}

