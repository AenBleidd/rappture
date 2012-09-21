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
 * This file defines a java class that implemements the rappture units module.
 * The class methods call the corresponding native code through c++ glue code
 * located in jRpUnits.cc.
 */


package rappture;

public class Units{
  static {
    System.loadLibrary("JRappture");
  }

  // Public Methods------------------------------------------------------------
  public static double convertDouble(double fromVal, String to){
    return Double.parseDouble(
      jRpUnitsConvert(Double.toString(fromVal), to, false));
  }

  public static String convertString(String fromVal, String to, boolean units){
    return jRpUnitsConvert(fromVal, to, units);
  }

  public static String convertString(String fromVal, String to){
    return jRpUnitsConvert(fromVal, to, true);
  }

  // Native Functions----------------------------------------------------------
  private static native String jRpUnitsConvert(String fromVal, String to,
    boolean units);
}



