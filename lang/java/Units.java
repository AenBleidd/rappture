package rappture;

public class Units{
  static {
    System.loadLibrary("jRappture");
  }

  public static String convert(String fromVal, String to, boolean units){
    return jRpUnitsConvert(fromVal, to, units);
  }

  private static native String jRpUnitsConvert(String fromVal, String to, boolean units);
}



