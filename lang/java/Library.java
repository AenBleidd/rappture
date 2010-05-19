package rappture;

public class Library{
  static {
    System.loadLibrary("jRappture");
  }

  // Constructor---------------------------------------------------------------
  public Library(String path){
    libPtr = jRpLibrary(path);
  }

  // Pseudo-Destructor---------------------------------------------------------
  protected void finalize() throws Throwable {
    try {
      jRpDeleteLibrary(libPtr);
    } finally {
        super.finalize();
    }
  }

  // Public Methods------------------------------------------------------------
  public double getDouble(String path){
    return jRpGetDouble(libPtr, path);
  }

  public String getString(String path){
    return jRpGetString(libPtr, path);
  }

  public String get(String path){  // alias for getString
    return jRpGetString(libPtr, path);
  }

  public void put(String path, Object value, boolean append){
    // convert generic object to string 
    jRpPut(libPtr, path, value.toString(), append);
  }

  public void put(String path, Object value){
    jRpPut(libPtr, path, value.toString(), false);
  }

  public void result(int exitStatus){
    jRpResult(libPtr, exitStatus);
  }

  public void result(){
    jRpResult(libPtr, 0);
  }


  // Native Functions----------------------------------------------------------
  private native long jRpLibrary(String path);
  private native void jRpDeleteLibrary(long libPtr);

  private native double jRpGetDouble(long libPtr, String path);
  private native String jRpGetString(long libPtr, String path);

  private native void jRpPut(long libPtr, String path, String value, boolean append);

  private native void jRpResult(long libPtr, int exitStatus);  

  // Private Attributes--------------------------------------------------------
  private long libPtr;  //pointer to c rpLibrary 

}



