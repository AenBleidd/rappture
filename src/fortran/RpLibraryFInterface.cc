/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2007  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpLibraryFInterface.h"
#include "RpBindingsDict.h"
#include "RpLibraryFStubs.c"

/**********************************************************************/
// FUNCTION: rp_lib(const char* filePath, int filePath_len)
/// Open the file at 'filePath' and return Rappture Library Object.
/**
 */

int rp_lib(const char* filePath, int filePath_len) {

    RpLibrary* lib = NULL;
    int handle = -1;
    std::string inFilePath = "";

    inFilePath = null_terminate_str(filePath, filePath_len);

    // create a RapptureIO object and store in dictionary
    lib = new RpLibrary(inFilePath);

    if (lib) {
        handle = storeObject_Lib(lib);
    }

    return handle;
}

/**********************************************************************/
// FUNCTION: rp_lib_element_obj()
/// Return the node located at 'path'.
/**
 */
int rp_lib_element_obj(int* handle,     /* integer handle of library */
                          char* path,     /* null terminated path */
                          int path_len
                        )
{
    int newObjHandle = -1;

    std::string inPath = "";

    RpLibrary* lib = NULL;
    RpLibrary* retObj = NULL;

    inPath = null_terminate_str(path,path_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);
        if (lib) {
            retObj = lib->element(inPath);

            if (retObj) {
                newObjHandle = storeObject_Lib(retObj);
            }
        }
    }

    return newObjHandle;
}

/**********************************************************************/
// FUNCTION: rp_lib_element_comp()
/// Return the component name of the node located at 'path'.
/**
 */
void rp_lib_element_comp( int* handle, /* integer handle of library */
                            char* path,      /* null terminated path */
                            char* retText,   /* return buffer for fortran*/
                            int path_len,
                            int retText_len /* length of return text buffer */
                          )
{
    std::string inPath = "";
    RpLibrary* lib = NULL;
    std::string retStr = "";

    inPath = null_terminate_str(path,path_len);

    lib = getObject_Lib(*handle);

    if (lib) {
        retStr = lib->element(inPath)->nodeComp();
        if (!retStr.empty()) {
            fortranify(retStr.c_str(),retText,retText_len);
        }
    }

}

/**********************************************************************/
// FUNCTION: rp_lib_element_id()
/// Return the 'id' of node located at 'path'.
/**
 */
void rp_lib_element_id(   int* handle,       /* integer handle of library    */
                            char* path,      /* path of element within xml   */
                            char* retText,   /* return buffer for fortran    */
                            int path_len,
                            int retText_len  /* length of return text buffer */
                        ) 
{
    std::string inPath = "";
    RpLibrary* lib = NULL;
    std::string retStr = "";

    inPath = null_terminate_str(path,path_len);

    lib = getObject_Lib(*handle);

    if (lib) {
        retStr = lib->element(inPath)->nodeId();
        if (!retStr.empty()) {
            fortranify(retStr.c_str(),retText,retText_len);
        }
    }
}

/**********************************************************************/
// FUNCTION: rp_lib_element_type()
/// Return the type of the node located at 'path'.
/**
 */
void rp_lib_element_type( int* handle,       /* integer handle of library */
                            char* path,      /* search path inside xml */
                            char* retText,   /* return buffer for fortran*/
                            int path_len,
                            int retText_len  /* length of return text buffer */
                          )
{
    std::string inPath = "";
    RpLibrary* lib = NULL;
    std::string retStr = "";

    inPath = null_terminate_str(path,path_len);

    lib = getObject_Lib(*handle);

    if (lib) {
        retStr = lib->element(inPath)->nodeType();
        fortranify(retStr.c_str(),retText,retText_len);
    }
}

/**********************************************************************/
// FUNCTION: rp_lib_children(const char* filePath, int filePath_len)
/// Retrieve the next child of 'path' from the Rappture Library Object.
/**
 */
int rp_lib_children (   int* handle, /* integer handle of library */
                        char* path, /* search path of the xml */
                        int* childHandle, /*integer hanlde of last returned child*/
                        int path_len  /* length of the search path buffer */
                    ) {

    std::string inPath = "";
    RpLibrary* lib = NULL;
    RpLibrary* childNode = NULL;
    int newObjHandle = -1;

    inPath = null_terminate_str(path,path_len);

    if (handle && (*handle >= 0) ) {
        lib = getObject_Lib(*handle);
        if (lib) {

            if (*childHandle > 0) {
                // check to see if there were any previously returned children
                childNode = getObject_Lib(*childHandle);
            }

            // call the children base method
            childNode = lib->children(inPath,childNode);

            // store the childNode in the dictionary.
            //
            // because the base method is using static memory to get store the
            // children we should be able to check and see if the childHandle 
            // was valid.
            // if so, then we can just return the childHandle back to the user
            // if not, store the object in the dictionary and return the new
            // handle.

            if (childNode) {
                if (*childHandle < 1) {
                    newObjHandle = storeObject_Lib(childNode);
                }
                else {
                    newObjHandle = storeObject_Lib(childNode,*childHandle);
                }
            }
        }
    }

    return newObjHandle;

}


//int rp_lib_child_comp (   int* handle,    /* integer handle of library */
//                            char* path,     /* DOM path of requested object */
//                            char* type,     /* specific name of element */
//                            int* childNum,  /* child number for iteration */
//                            char* retText,  /* buffer to store return text */
//                            int path_len,   /* length of path */
//                            int type_len,   /* length of type */
//                            int retText_len /* length of return text buffer */
//                       )
/*
{
    int retVal = 0;

    char* inPath = NULL;
    char* inType = NULL;

    inPath = null_terminate(path,path_len);
    inType = null_terminate(type,type_len);

    if (inPath) {
        path_len = strlen(inPath) + 1;
    }
    else {
        path_len = 0;
    }

    if (inType) {
        type_len = strlen(inType) + 1;
    }
    else {
        type_len = 0;
    }

    retVal = rp_lib_child_str( handle,
                                 inPath,
                                 "component",
                                 inType,
                                 childNum,
                                 retText,
                                 path_len,
                                 10,
                                 type_len,
                                 retText_len
                               );

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inType) {
        free(inType);
        inType = NULL;
    }

    return retVal;
}
*/

//int rp_lib_child_id(   int* handle,    /* integer handle of library */
//                          char* path,     /* DOM path of requested object */
//                          char* type,     /* specific name of element */
//                          int* childNum,  /* child number for iteration */
//                          char* retText,  /* buffer to store return text */
//                          int path_len,   /* length of path */
//                          int type_len,   /* length of type */
//                          int retText_len /* length of return text buffer */
//                       )
/*
{
    int retVal = 0;
    char* inPath = NULL;
    char* inType = NULL;

    inPath = null_terminate(path,path_len);
    inType = null_terminate(type,type_len);

    if (inPath) {
        path_len = strlen(inPath) + 1;
    }
    else {
        path_len = 0;
    }

    if (inType) {
        type_len = strlen(inType) + 1;
    }
    else {
        type_len = 0;
    }

    retVal = rp_lib_child_str( handle,
                                 inPath,
                                 "id",
                                 inType,
                                 childNum,
                                 retText,
                                 path_len,
                                 3,
                                 type_len,
                                 retText_len
                               );

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inType) {
        free(inType);
        inType = NULL;
    }

    return retVal;
}
*/

//int rp_lib_child_type (   int* handle,    /* integer handle of library */
//                            char* path,     /* DOM path of requested object */
//                            char* type,     /* specific name of element */
//                            int* childNum,  /* child number for iteration */
//                            char* retText,  /* buffer to store return text */
//                            int path_len,   /* length of path */
//                            int type_len,   /* length of type */
//                            int retText_len /* length of return text buffer */
//                       )
/*
{
    int retVal = 0;
    char* inPath = NULL;
    char* inType = NULL;

    inPath = null_terminate(path,path_len);
    inType = null_terminate(type,type_len);

    if (inPath) {
        path_len = strlen(inPath) + 1;
    }
    else {
        path_len = 0;
    }

    if (inType) {
        type_len = strlen(inType) + 1;
    }
    else {
        type_len = 0;
    }

    retVal = rp_lib_child_str( handle,
                                 inPath,
                                 "type",
                                 inType,
                                 childNum,
                                 retText,
                                 path_len,
                                 5,
                                 type_len,
                                 retText_len
                               );

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inType) {
        free(inType);
        inType = NULL;
    }

    return retVal;
}
*/

/*
int rp_lib_child_obj ( int* handle, 
                            char* path, 
                            char* type, 
                            int path_len,
                            int type_len
                          )
{
    int newObjHandle = -1;

    PyObject* lib = NULL;
    PyObject* list = NULL;

    char* inPath = NULL;
    char* inType = NULL;

    inPath = null_terminate(path,path_len);
    inType = null_terminate(type,type_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject_Lib(*handle);

            if (lib) {
                list = rpChildren_f(lib, inPath, "object");
                if (list) {
                    // store the whole list,
                    // need to make a way to read the nodes
                    newObjHandle = storeObject_Lib(list);
                    // Py_DECREF(list);
                }
                else {

                }
            }
        }
    }

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inType) {
        free(inType);
        inType = NULL;
    }

    return newObjHandle;

}
*/

/**********************************************************************/
// FUNCTION: rp_lib_get()
/// Get data located at 'path' and return it as a string value.
/**
 */
void rp_lib_get( int* handle, /* integer handle of library */
                   char* path,      /* null terminated path */
                   char* retText,   /* return text buffer for fortran*/
                   int path_len,
                   int retText_len /* length of return text buffer */
                 )
{
    std::string xmlText = "";

    RpLibrary* lib = NULL;

    std::string inPath = "";

    inPath = null_terminate_str(path,path_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            xmlText = lib->get(inPath);
            if (!xmlText.empty()) {
                fortranify(xmlText.c_str(),retText,retText_len);
            }

        }
    }
}

/**********************************************************************/
// FUNCTION: rp_lib_get_str()
/// Get data located at 'path' and return it as a string value.
/**
 */
void rp_lib_get_str( int* handle, /* integer handle of library */
                   char* path,      /* null terminated path */
                   char* retText,   /* return text buffer for fortran*/
                   int path_len,
                   int retText_len /* length of return text buffer */
                 )
{
    std::string xmlText = "";

    RpLibrary* lib = NULL;

    std::string inPath = "";

    inPath = null_terminate_str(path,path_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            xmlText = lib->getString(inPath);
            if (!xmlText.empty()) {
                fortranify(xmlText.c_str(),retText,retText_len);
            }

        }
    }
}


/**********************************************************************/
// FUNCTION: rp_lib_get_double()
/// Get data located at 'path' and return it as a double precision value.
/**
 */
double rp_lib_get_double( int* handle,   /* integer handle of library */
                            char* path,    /* null terminated path */
                            int path_len
                          )
{
    double retVal = 0.0;

    RpLibrary* lib = NULL;

    std::string inPath = "";

    inPath = null_terminate_str(path,path_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            retVal = lib->getDouble(inPath);
        }
    }

    return retVal;
}


/**********************************************************************/
// FUNCTION: rp_lib_get_integer()
/// Get data located at 'path' and return it as an integer value.
/**
 */
int rp_lib_get_integer( int* handle,   /* integer handle of library */
                        char* path,    /* null terminated path */
                        int path_len
                      )
{
    int retVal = 0;

    RpLibrary* lib = NULL;

    std::string inPath = "";

    inPath = null_terminate_str(path,path_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            retVal = lib->getInt(inPath);
        }
    }

    return retVal;
}


/**********************************************************************/
// FUNCTION: rp_lib_get_boolean()
/// Get data located at 'path' and return it as an integer value.
/**
 */
int rp_lib_get_boolean( int* handle,   /* integer handle of library */
                        char* path,    /* null terminated path */
                        int path_len
                      )
{
    bool value = false;
    int retVal = 0;

    RpLibrary* lib = NULL;

    std::string inPath = "";

    inPath = null_terminate_str(path,path_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            value = lib->getBool(inPath);
            if (value == true) {
                retVal = 1;
            }
            else {
                retVal = 0;
            }
        }
    }

    return retVal;
}


/**********************************************************************/
// FUNCTION: rp_lib_put_str()
/// Put string into Rappture Library Object at location 'path'.
/**
 */
void rp_lib_put_str( int* handle, 
                        char* path, 
                        char* value, 
                        int* append,
                        int path_len,
                        int value_len
                      )
{
    std::string inPath = "";
    std::string inValue = "";
    RpLibrary* lib = NULL;

    inPath = null_terminate_str(path,path_len);
    inValue = null_terminate_str(value,value_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            lib->put(inPath,inValue,"",*append);
        }
    }

    return;

}


/**********************************************************************/
// FUNCTION: rp_lib_put_id_str()
/// Put string into Rappture Library Object at location 'path' with id = 'id'
/**
 */
void rp_lib_put_id_str ( int* handle, 
                        char* path, 
                        char* value, 
                        char* id, 
                        int* append,
                        int path_len,
                        int value_len,
                        int id_len
                      ) 
{
    RpLibrary* lib = NULL;

    std::string inPath = "";
    std::string inValue = "";
    std::string inId = "";

    inPath = null_terminate_str(path,path_len);
    inValue = null_terminate_str(value,value_len);
    inId = null_terminate_str(id,id_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            lib->put(inPath,inValue,inId,*append);
        }
    }

}


/**********************************************************************/
// FUNCTION: rp_lib_put_data()
/// Put string into Rappture Library Object at location 'path'.
/**
 */
void rp_lib_put_data( int* handle,
                        char* path,
                        char* bytes,
                        int* nbytes,
                        int* append,
                        int path_len,
                        int bytes_len
                      )
{
    std::string inPath = "";
    RpLibrary* lib = NULL;
    int bufferSize = 0;

    inPath = null_terminate_str(path,path_len);

    if (*nbytes < bytes_len) {
        bufferSize = *nbytes;
    }
    else {
        bufferSize = bytes_len;
    }

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            lib->putData(inPath,bytes,bufferSize,*append);
        }
    }

    return;

}


/**********************************************************************/
// FUNCTION: rp_lib_put_file()
/// Put string into Rappture Library Object at location 'path'.
/**
 */
void rp_lib_put_file( int* handle,
                        char* path,
                        char* fileName,
                        int* compress,
                        int* append,
                        int path_len,
                        int fileName_len
                      )
{
    std::string inPath = "";
    std::string inFileName = "";
    RpLibrary* lib = NULL;

    inPath = null_terminate_str(path,path_len);
    inFileName = null_terminate_str(fileName,fileName_len);

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            lib->putFile(inPath,inFileName,*compress,*append);
        }
    }

    return;

}


//void rp_lib_put_obj( int* handle, 
//                        char* path, 
//                        int* valHandle, 
//                        int* append,
//                        int path_len
//                      ) 
//{
//    std::string inPath = "";
//    RpLibrary* lib = NULL;
//
//    // inPath = null_terminate(path,path_len);
//    inPath = std::string(path,path_len);
//
//    /*
//    if (inPath) {
//        path_len = strlen(inPath) + 1;
//    }
//    */
//
//    if ((handle) && (*handle != 0)) {
//        lib = getObject_Lib(*handle);
//        
//        if (lib) {
//            // rpPut(lib, inPath, inValue, inId, *append);
//            lib->put(inPath,inValue,"",*append);
//        }
//    }
//
//    /*
//    rp_lib_put_id_obj(handle,inPath,valHandle,NULL,append,path_len,0);
//
//    if (inPath) {
//        free(inPath);
//        inPath = NULL;
//    }
//    */
//
//}

/*
void rp_lib_put_id_obj ( int* handle, 
                        char* path, 
                        int* valHandle, 
                        char* id, 
                        int* append,
                        int path_len,
                        int id_len
                      ) 
{
    PyObject* lib = NULL;
    PyObject* value = NULL;

    char* inPath = NULL;
    char* inId = NULL;

    inPath = null_terminate(path,path_len);
    inId = null_terminate(id,id_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject_Lib(*handle);
            value = getObject_Lib(*valHandle);

            if (lib && value) {
                // retObj is a borrowed object 
                // whose contents must not be modified
                rpPutObj(lib, inPath, value, inId, *append);
            }
        }
    }

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inId) {
        free(inId);
        inId = NULL;
    }

}
*/

/*
int rp_lib_remove (int* handle, char* path, int path_len) 
{
    int newObjHandle = -1;

    PyObject* lib = NULL;
    PyObject* removedObj = NULL;

    char* inPath = NULL;

    inPath = null_terminate(path,path_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject_Lib(*handle);

            if (lib) {
                removedObj = rpRemove(lib, inPath);

                if (removedObj) {
                    newObjHandle = storeObject_Lib(removedObj);
                    // Py_DECREF(removedObj);
                }

            }
        }
    }

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    return newObjHandle;
}
*/

/*
int rp_lib_xml_len (int* handle)
{
    int length = -1;
    char* xmlText = NULL;

    PyObject* lib = NULL;

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject_Lib(*handle);

            if (lib) {
                // dont modify xmlText, borrowed pointer
                xmlText = rpXml(lib);

                if (xmlText) {
                    length = strlen(xmlText) + 1;
                    free(xmlText);
                    // printf("len = :%d:\n",length);
                }
            }
        }
    }
    return length;
}
*/

/**********************************************************************/
// FUNCTION: rp_lib_write_xml()
/// Write the xml text into a file.
/**
 * \sa {rp_result}
 */
int rp_lib_write_xml(int* handle, char* outFile, int outFile_len)
{
    int retVal = -1;
    RpLibrary* lib = NULL;
    std::string inOutFile = "";
    std::string xmlText = "";
    std::fstream file;

    inOutFile = null_terminate_str(outFile, outFile_len);

    if (!inOutFile.empty() ) {
        file.open(inOutFile.c_str(),std::ios::out);
    }

    if ( file.is_open() ) {
        if ((handle) && (*handle != 0)) {
            lib = getObject_Lib(*handle);

            if (lib) {
                xmlText = lib->xml();
                if (!xmlText.empty()) {
                    file << xmlText;
                    retVal = 0;
                }
            }
        }

        file.close();
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_lib_xml()
/// Return this node's xml text
/**
 */
void  rp_lib_xml(int* handle, char* retText, int retText_len)
{
    std::string xmlText = "";

    RpLibrary* lib = NULL;

    if ((handle) && (*handle != 0)) {
        lib = getObject_Lib(*handle);

        if (lib) {
            xmlText = lib->xml();
            if (!xmlText.empty()) {
                fortranify(xmlText.c_str(), retText, retText_len);
            }
        }
    }
}

/**********************************************************************/
// FUNCTION: rp_lib_node_comp()
/// Return this node's component name
/**
 */
void rp_lib_node_comp ( int* handle, char* retText, int retText_len ) {

    std::string retStr = "";
    RpLibrary* node = NULL;

    if ((handle) && (*handle != 0)) {
        node = getObject_Lib(*handle);

        if (node) {
            retStr = node->nodeComp();
            if (!retStr.empty()) {
                fortranify(retStr.c_str(), retText, retText_len);
            }
        }
    }
}

/**********************************************************************/
// FUNCTION: rp_lib_node_type()
/// Return this node's type
/**
 */
void rp_lib_node_type ( int* handle, char* retText, int retText_len ) {

    std::string retStr = "";
    RpLibrary* node = NULL;

    if ((handle) && (*handle != 0)) {
        node = getObject_Lib(*handle);

        if (node) {
            retStr = node->nodeType();
            if (!retStr.empty()) {
                fortranify(retStr.c_str(), retText, retText_len);
            }
        }
    }
}

/**********************************************************************/
// FUNCTION: rp_lib_node_id()
/// Return this node's id.
/**
 */
void rp_lib_node_id ( int* handle, char* retText, int retText_len ) {

    std::string retStr = "";
    RpLibrary* node = NULL;

    if ((handle) && (*handle != 0)) {
        node = getObject_Lib(*handle);

        if (node) {
            retStr = node->nodeId();
            if (!retStr.empty()) {
                fortranify(retStr.c_str(), retText, retText_len);
            }
        }
    }
}

/**********************************************************************/
// FUNCTION: rp_result()
/// Write xml text to a run.xml file and signal the program has completed
/**
 */
void rp_result(int* handle) {
    RpLibrary* lib = NULL;

    if (handle && *handle != 0) {
        lib = getObject_Lib(*handle);
        if (lib) {
            lib->result();
        }
    }

    // do no delete this, still working on testing this
    cleanLibDict();
}

