/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpLibraryFInterface.h"
#include "RpBindingsDict.h"
#include "RpLibraryFStubs.c"

int rp_lib(const char* filePath, int filePath_len) 
{
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

void rp_lib_element_id(   int* handle, /* integer handle of library */
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
        retStr = lib->element(inPath)->nodeId();
        if (!retStr.empty()) {
            fortranify(retStr.c_str(),retText,retText_len);
        }
    }
}

void rp_lib_element_type( int* handle, /* integer handle of library */
                            char* path,      /* search path inside xml */
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
        retStr = lib->element(inPath)->nodeType();
        fortranify(retStr.c_str(),retText,retText_len);
    }
}

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
            if (*childHandle < 1) {
                // check to see if there were any previously returned children
                childNode = getObject_Lib(*childHandle);
            }

            // call the children base method
            childNode = lib->children(path,childNode);

            // store the childNode in the dictionary.
            //
            // because the base method is using static memory to get store the
            // children we should be able to chekc and see if the childHandle 
            // was valud.
            // if so, then we can just return the childHandle back to the user
            // if not, store the object in the dictionary and return the new
            // handle.

            if (childNode) {
                if (*childHandle < 1) {
                    newObjHandle = storeObject_Lib(childNode);
                }
                else {
                    newObjHandle = *childHandle;
                }
            }
        }
    }

    return newObjHandle;

}

/*
int rp_lib_child_num( int* handle, 
                         char* path, 
                         int path_len
                       )
{
    int numChildren = 0;

    PyObject* lib = NULL;
    PyObject* list = NULL;
    char* inPath = NULL;

    inPath = null_terminate(path,path_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject_Lib(*handle);

            if (lib) {
                list = rpChildren_f(lib, inPath, "type");
                if (list) {
                    numChildren = PyList_Size(list);
                    Py_DECREF(list);
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

    return numChildren;

}
*/

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
            xmlText = lib->getString(inPath);
            if (!xmlText.empty()) {
                fortranify(xmlText.c_str(),retText,retText_len);
            }

        }
    }
}

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

void rp_quit()
{

    // clean up the dictionary

/*
    RpDictEntry DICT_TEMPLATE_L *hPtr;
    // RpDictIterator DICT_TEMPLATE iter(fortObjDict_Lib);
    // should rp_quit clean up the dict or some function in RpBindingsCommon.h
    RpDictIterator DICT_TEMPLATE_L iter(ObjDict_Lib);

    hPtr = iter.first();

    while (hPtr) {
        // Py_DECREF(*(hPtr->getValue()));
        hPtr->erase();
        hPtr = iter.next();
    }

    // if (fortObjDict_Lib.size()) {
    if (ObjDict_Lib.size()) {
        // probably want to change the warning sometime
        // printf("\nWARNING: internal dictionary is not empty..deleting\n");
    }
*/
    cleanLibDict();
}

void rp_result(int* handle) {
    RpLibrary* lib = NULL;

    if (handle && *handle != 0) {
        lib = getObject_Lib(*handle);
        if (lib) {
            lib->result();
        }
    }
}

