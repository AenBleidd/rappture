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
 * This file defines a java class that implemements the rappture units module.
 * The class methods call the corresponding native code through c++ glue code
 * located in jRpUnits.cc.
 */


package rappture;

public class Units{
  static {
    System.loadLibrary("JRappture");
  }

  public static String convert(String fromVal, String to, boolean units){
    return jRpUnitsConvert(fromVal, to, units);
  }

  public static String convert(String fromVal, String to){
    return jRpUnitsConvert(fromVal, to, true);
  }

  private static native String jRpUnitsConvert(String fromVal, String to, boolean units);
}



