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
 * This file defines a Java class that implemements the rappture XML library.
 * The class methods call the corresponding native code through c++ glue code
 * located in jRpLibrary.cc.
 */

package rappture;

public class Library{
  static {
    System.loadLibrary("JRappture");
  }

  // Constructor---------------------------------------------------------------
  public Library(String path){
    libPtr = jRpLibrary(path);
  }

  public Library(){
    libPtr = jRpLibrary(null);
  }

  // Pseudo-Destructor.  Called when the object is garbage collected.----------
  protected void finalize() throws Throwable {
    try {
      jRpDeleteLibrary(libPtr);
    } finally {
        super.finalize();
    }
  }

  // Public Methods------------------------------------------------------------
  
  public byte[] getData(String path){
    return jRpGetData(libPtr, path);
  }

  public double getDouble(String path){
    return jRpGetDouble(libPtr, path);
  }

  public String getString(String path){
    return jRpGetString(libPtr, path);
  }

  public int getInt(String path){
    return jRpGetInt(libPtr, path);
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

  public void putData(String path, byte[] b, boolean append){
    jRpPutData(libPtr, path, b, b.length, append);
  }

  public void putData(String path, byte[] b){
    jRpPutData(libPtr, path, b, b.length, false);
  }

  public void putFile(String path, String fileName,
                      boolean compress, boolean append){
    jRpPutFile(libPtr, path, fileName, compress, append);
  }

  public void putFile(String path, String fileName, boolean compress){
    jRpPutFile(libPtr, path, fileName, compress, false);
  }

  public void putFile(String path, String fileName){
    jRpPutFile(libPtr, path, fileName, true, false);
  }

  public void result(int exitStatus){
    jRpResult(libPtr, exitStatus);
  }

  public void result(){
    jRpResult(libPtr, 0);
  }

  public String xml(){
    return jRpXml(libPtr);
  }


  // Native Functions----------------------------------------------------------
  private native long jRpLibrary(String path);
  private native void jRpDeleteLibrary(long libPtr);

  private native byte[] jRpGetData(long libPtr, String path);
  private native double jRpGetDouble(long libPtr, String path);
  private native int jRpGetInt(long libPtr, String path);
  private native String jRpGetString(long libPtr, String path);

  private native void jRpPut(long libPtr, String path,
                             String value, boolean append);
  private native void jRpPutData(long libPtr, String path,
                                 byte[] b, int nbytes, boolean append);
  private native void jRpPutFile(long libPtr, String path, String fileName,
                                 boolean compress, boolean append);

  private native void jRpResult(long libPtr, int exitStatus);  
  private native String jRpXml(long libPtr);

  // Private Attributes--------------------------------------------------------
  private long libPtr;  //pointer to c++ RpLibrary 

}

