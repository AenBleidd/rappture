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
 * This file defines a java class that implemements the rappture utils module.
 * The class methods call the corresponding native code through c++ glue code
 * located in jRpUtils.cc.
 */

package rappture;

public class Utils{
  static{
    System.loadLibrary("JRappture");
  }

  public static void progress(int percent, String text){
    jRpUtilsProgress(percent, text);
  }

  public static void progress(int percent){
    jRpUtilsProgress(percent, "");
  }

  private static native void jRpUtilsProgress(int percent, String text);

}

