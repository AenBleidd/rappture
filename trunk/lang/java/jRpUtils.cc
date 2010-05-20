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
 * This file defines the native functions which are called by the java Utils
 * class methods, and in turn call the corresponding rappture RpUtils methods.
 */

#include "jRpUtils.h"
#include "rappture.h"

// progress
JNIEXPORT void JNICALL Java_rappture_Utils_jRpUtilsProgress
  (JNIEnv *env, jclass cls, jint percent, jstring javaText){
	  const char* nativeText = env->GetStringUTFChars(javaText, 0);
  Rappture::Utils::progress(percent, nativeText);
  env->ReleaseStringUTFChars(javaText, nativeText);
  return;
}

