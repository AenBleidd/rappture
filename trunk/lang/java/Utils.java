package rappture;

public class Utils{
  static{
    System.loadLibrary("jRappture");
  }

  public static void progress(int percent, String text){
    jRpUtilsProgress(percent, text);
  }

  private static native void jRpUtilsProgress(int percent, String text);

}

