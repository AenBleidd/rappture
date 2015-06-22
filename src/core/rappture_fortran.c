/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "rappture_interface.h"
#include "RpFortranCommon.h"
#include "RpDict.h"
#include <string.h>

/*
// rappture-fortran api
*/

#ifdef COMPNAME_ADD1UNDERSCORE
#   define rp_init                 rp_init_
#   define rp_lib                  rp_lib_
#   define rp_lib_element_comp     rp_lib_element_comp_
#   define rp_lib_element_id       rp_lib_element_id_
#   define rp_lib_element_type     rp_lib_element_type_
#   define rp_lib_element_str      rp_lib_element_str_
#   define rp_lib_element_obj      rp_lib_element_obj_
#   define rp_lib_child_num        rp_lib_child_num_
#   define rp_lib_child_comp       rp_lib_child_comp_
#   define rp_lib_child_id         rp_lib_child_id_
#   define rp_lib_child_type       rp_lib_child_type_
#   define rp_lib_child_str        rp_lib_child_str_
#   define rp_lib_child_obj        rp_lib_child_obj_
#   define rp_lib_get              rp_lib_get_
#   define rp_lib_get_double       rp_lib_get_double_
#   define rp_lib_put_str          rp_lib_put_str_
#   define rp_lib_put_id_str       rp_lib_put_id_str_
#   define rp_lib_put_obj          rp_lib_put_obj_
#   define rp_lib_put_id_obj       rp_lib_put_id_obj_
#   define rp_lib_remove           rp_lib_remove_
#   define rp_lib_xml_len          rp_lib_xml_len_
#   define rp_lib_write_xml        rp_lib_write_xml_
#   define rp_lib_xml              rp_lib_xml_
#   define rp_quit                 rp_quit_
#   define rp_result               rp_result_
#elif defined(COMPNAME_ADD2UNDERSCORE)
#   define rp_init                 rp_init__
#   define rp_lib                  rp_lib__
#   define rp_lib_element_comp     rp_lib_element_comp__
#   define rp_lib_element_id       rp_lib_element_id__
#   define rp_lib_element_type     rp_lib_element_type__
#   define rp_lib_element_str      rp_lib_element_str__
#   define rp_lib_element_obj      rp_lib_element_obj__
#   define rp_lib_child_num        rp_lib_child_num__
#   define rp_lib_child_comp       rp_lib_child_comp__
#   define rp_lib_child_id         rp_lib_child_id__
#   define rp_lib_child_type       rp_lib_child_type__
#   define rp_lib_child_str        rp_lib_child_str__
#   define rp_lib_child_obj        rp_lib_child_obj__
#   define rp_lib_get              rp_lib_get__
#   define rp_lib_get_double       rp_lib_get_double__
#   define rp_lib_put_str          rp_lib_put_str__
#   define rp_lib_put_id_str       rp_lib_put_id_str__
#   define rp_lib_put_obj          rp_lib_put_obj__
#   define rp_lib_put_id_obj       rp_lib_put_id_obj__
#   define rp_lib_remove           rp_lib_remove__
#   define rp_lib_xml_len          rp_lib_xml_len__
#   define rp_lib_write_xml        rp_lib_write_xml__
#   define rp_lib_xml              rp_lib_xml__
#   define rp_quit                 rp_quit__
#   define rp_result               rp_result__
#elif defined(COMPNAME_NOCHANGE)
#   define rp_init                 rp_init
#   define rp_lib                  rp_lib
#   define rp_lib_element_comp     rp_lib_element_comp
#   define rp_lib_element_id       rp_lib_element_id
#   define rp_lib_element_type     rp_lib_element_type
#   define rp_lib_element_str      rp_lib_element_str
#   define rp_lib_element_obj      rp_lib_element_obj
#   define rp_lib_child_num        rp_lib_child_num
#   define rp_lib_child_comp       rp_lib_child_comp
#   define rp_lib_child_id         rp_lib_child_id
#   define rp_lib_child_type       rp_lib_child_type
#   define rp_lib_child_str        rp_lib_child_str
#   define rp_lib_child_obj        rp_lib_child_obj
#   define rp_lib_get              rp_lib_get
#   define rp_lib_get_double       rp_lib_get_double
#   define rp_lib_put_str          rp_lib_put_str
#   define rp_lib_put_id_str       rp_lib_put_id_str
#   define rp_lib_put_obj          rp_lib_put_obj
#   define rp_lib_put_id_obj       rp_lib_put_id_obj
#   define rp_lib_remove           rp_lib_remove
#   define rp_lib_xml_len          rp_lib_xml_len
#   define rp_lib_write_xml        rp_lib_write_xml
#   define rp_lib_xml              rp_lib_xml
#   define rp_quit                 rp_quit
#   define rp_result               rp_result
#elif defined(COMPNAME_UPPERCASE)
#   define rp_init                 RP_INIT
#   define rp_lib                  RP_LIB
#   define rp_lib_element_comp     RP_LIB_ELEMENT_COMP
#   define rp_lib_element_id       RP_LIB_ELEMENT_ID
#   define rp_lib_element_type     RP_LIB_ELEMENT_TYPE
#   define rp_lib_element_str      RP_LIB_ELEMENT_STR
#   define rp_lib_element_obj      RP_LIB_ELEMENT_OBJ
#   define rp_lib_child_num        RP_LIB_CHILD_NUM
#   define rp_lib_child_comp       RP_LIB_CHILD_COMP
#   define rp_lib_child_id         RP_LIB_CHILD_ID
#   define rp_lib_child_type       RP_LIB_CHILD_TYPE
#   define rp_lib_child_str        RP_LIB_CHILD_STR
#   define rp_lib_child_obj        RP_LIB_CHILD_OBJ
#   define rp_lib_get              RP_LIB_GET
#   define rp_lib_get_double       RP_LIB_GET_DOUBLE
#   define rp_lib_put_str          RP_LIB_PUT_STR
#   define rp_lib_put_id_str       RP_LIB_PUT_ID_STR
#   define rp_lib_put_obj          RP_LIB_PUT_OBJ
#   define rp_lib_put_id_obj       RP_LIB_PUT_ID_OBJ
#   define rp_lib_remove           RP_LIB_REMOVE
#   define rp_lib_xml_len          RP_LIB_XML_LEN
#   define rp_lib_write_xml        RP_LIB_WRITE_XML
#   define rp_lib_xml              RP_LIB_XML
#   define rp_quit                 RP_QUIT
#   define rp_result               RP_RESULT
#endif


extern "C"
void rp_init();

extern "C"
int rp_lib(const char* filePath, int filePath_len);

extern "C"
void rp_lib_element_comp(int* handle,
                         char* path,
                         char* retText,
                         int path_len,
                         int retText_len);

extern "C"
void rp_lib_element_id(int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len);

extern "C"
void rp_lib_element_type(int* handle,
                         char* path,
                         char* retText,
                         int path_len,
                         int retText_len);

extern "C"
void rp_lib_element_str(int* handle,
                        char* path,
                        char* flavor,
                        char* retText,
                        int path_len,
                        int flavor_len,
                        int retText_len);

extern "C"
int rp_lib_element_obj(int* handle,
                       char* path,
                       int path_len);

extern "C"
int rp_lib_child_num(int* handle,
                     char* path,
                     int path_len);

extern "C"
int rp_lib_child_comp(int* handle,    /* integer handle of library */
                      char* path,     /* DOM path of requested object */
                      char* type,     /* specific name of element */
                      int* childNum,  /* child number for iteration */
                      char* retText,  /* buffer to store return text */
                      int path_len,   /* length of path */
                      int type_len,   /* length of type */
                      int retText_len /* length of return text buffer */
                      );

extern "C"
int rp_lib_child_id(int* handle,    /* integer handle of library */
                    char* path,     /* DOM path of requested object */
                    char* type,     /* specific name of element */
                    int* childNum,  /* child number for iteration */
                    char* retText,  /* buffer to store return text */
                    int path_len,   /* length of path */
                    int type_len,   /* length of type */
                    int retText_len /* length of return text buffer */
                    );

extern "C"
int rp_lib_child_type(int* handle,    /* integer handle of library */
                      char* path,     /* DOM path of requested object */
                      char* type,     /* specific name of element */
                      int* childNum,  /* child number for iteration */
                      char* retText,  /* buffer to store return text */
                      int path_len,   /* length of path */
                      int type_len,   /* length of type */
                      int retText_len /* length of return text buffer */
                      );

extern "C"
int rp_lib_child_str(int* handle,    /* integer handle of library */
                     char* path,     /* DOM path of requested object */
                     char* flavor,   /* information you want returned*/
                     char* type,     /* specific name of element */
                     int* childNum,  /* child number for iteration */
                     char* retText,  /* buffer to store return text */
                     int path_len,   /* length of path */
                     int flavor_len, /* length of flavor */
                     int type_len,   /* length of type */
                     int retText_len /* length of return text buffer */
                     );

extern "C"
int rp_lib_child_obj(int* handle,
                     char* path,
                     char* type,
                     int path_len,
                     int type_len);

extern "C"
void rp_lib_get(int* handle,
                char* path,
                char* retText,
                int path_len,
                int retText_len);

extern "C"
double rp_lib_get_double(int* handle,
                         char* path,
                         int path_len);

extern "C"
void rp_lib_put_str(int* handle,
                    char* path,
                    char* value,
                    int* append,
                    int path_len,
                    int value_len);

extern "C"
void rp_lib_put_id_str(int* handle,
                       char* path,
                       char* value,
                       char* id,
                       int* append,
                       int path_len,
                       int value_len,
                       int id_len);

extern "C"
void rp_lib_put_obj(int* handle,
                    char* path,
                    int* valHandle,
                    int* append,
                    int path_len);

extern "C"
void rp_lib_put_id_obj(int* handle,
                       char* path,
                       int* valHandle,
                       char* id,
                       int* append,
                       int path_len,
                       int id_len);

extern "C"
int rp_lib_remove(int* handle,
                  char* path,
                  int path_len);

extern "C"
int rp_lib_xml_len(int* handle);

extern "C"
void rp_lib_xml(int* handle,
                char* retText,
                int retText_len);

extern "C"
int rp_lib_write_xml(int* handle,
                     char* outFile,
                     int outFile_len);

extern "C"
void rp_quit();

extern "C"
void rp_result(int* handle);


/**********************************************************/

/**********************************************************/

// private member functions
int storeNode(PyObject* node);
int objType( char* flavor);

int storeObject(PyObject* objectName);
PyObject* getObject(int objKey);
// char* null_terminate(char* inStr, int len);

// global vars
// dictionary to hold the python objects that
// cannot be sent to fortran

#define DICT_TEMPLATE <int,PyObject*>
RpDict DICT_TEMPLATE fortObjDict;
int rapptureStarted = 0;

void rp_init()
{
    //
    PyObject* rpClass = NULL;
    int retVal = 0;

    // initialize the interpreter
    Py_Initialize();
    rpClass = importRappture();

    if (rpClass) {
        // store the handle in the dictionary
        retVal = storeObject(rpClass);
        rapptureStarted = 1;
    }
}

int rp_lib(const char* filePath, int filePath_len)
{
    PyObject* lib = NULL;
    PyObject* rpClass = NULL;
    int handle = -1;
    char* inFilePath = NULL;

    if (!rapptureStarted) {
        rp_init();
    }

    inFilePath = null_terminate((char*)filePath, filePath_len);

    // error checking to make sure inText is valid???

    // grab the rappture class from the dictionary
    rpClass = getObject(0);

    if (rpClass) {


        // create a RapptureIO object and store in dictionary
        lib = createRapptureObj(rpClass, inFilePath);

        if (lib) {
            handle = storeObject(lib);
        }
    }

    if (inFilePath) {
        free(inFilePath);
        inFilePath = NULL;
    }

    return handle;
}

int rp_lib_element_obj(int* handle,     /* integer handle of library */
                       char* path,     /* null terminated path */
                       int path_len)
{
    int newObjHandle = -1;

    const char* flavor = "object";
    char* inPath = NULL;

    PyObject* lib = NULL;
    PyObject* retObj = NULL;

    inPath = null_terminate(path,path_len);

    // error checking to make sure inText is valid???

    if (fortObjDict.size() > 0) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);
            if (lib) {
                retObj = (PyObject*) rpElement(lib, inPath, flavor);

                if (retObj) {

                    newObjHandle = storeObject(retObj);
                    // Py_DECREF(retObj);
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

void rp_lib_element_comp(int* handle, /* integer handle of library */
                         char* path,      /* null terminated path */
                         char* retText,   /* return buffer for fortran*/
                         int path_len,
                         int retText_len /* length of return text buffer */
                         )
{
    char* inPath = NULL;

    inPath = null_terminate(path,path_len);
    if (path_len) {
        path_len = strlen(inPath) + 1;
    }
    else {
        path_len = 0;
    }

    // error checking to make sure inText is valid???

    rp_lib_element_str(handle,inPath,"component",retText,path_len,10,retText_len);

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

}

void rp_lib_element_id(int* handle, /* integer handle of library */
                       char* path,      /* null terminated path */
                       char* retText,   /* return buffer for fortran*/
                       int path_len,
                       int retText_len /* length of return text buffer */
                       )
{
    char* inPath = NULL;
    inPath = null_terminate(path,path_len);
    if (inPath) {
        path_len = strlen(inPath) + 1;
    }
    else {
        path_len = 0;
    }

    rp_lib_element_str(handle,inPath,"id",retText,path_len,3,retText_len);
    if (inPath) {
        free(inPath);
        inPath = NULL;
    }


}

void rp_lib_element_type(int* handle, /* integer handle of library */
                         char* path,      /* null terminated path */
                         char* retText,   /* return buffer for fortran*/
                         int path_len,
                         int retText_len /* length of return text buffer */
                         )
{
    char* inPath = NULL;
    inPath = null_terminate(path,path_len);
    if (inPath) {
        path_len = strlen(inPath) + 1;
    }
    else {
        path_len = 0;
    }
    rp_lib_element_str(handle,path,"type",retText,path_len,5,retText_len);
    if (inPath) {
        free(inPath);
        inPath = NULL;
    }


}

void rp_lib_element_str(int* handle, /* integer handle of library */
                        char* path,      /* null terminated path */
                        char* flavor,    /* null terminated flavor */
                        char* retText,   /* return buffer for fortran*/
                        int path_len,
                        int flavor_len,
                        int retText_len /* length of return text buffer */
                        )
{
    // int length_in = 0;
    // int length_out = 0;
    // int i = 0;
    const char* xmlText = NULL;

    PyObject* lib = NULL;

    char* retObj = NULL;

    char* inPath = NULL;
    char* inFlavor = NULL;

    inPath = null_terminate(path,path_len);
    inFlavor = null_terminate(flavor,flavor_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

            if (lib) {
                if(objType(inFlavor) == 1) {

                    retObj = (char*) rpElement(lib, inPath, inFlavor);
                    if (retObj) {
                        xmlText = retObj;

                        fortranify(xmlText, retText, retText_len);

                        /*
                        length_in = strlen(xmlText);
                        length_out = retText_len;

                        strncpy(retText, xmlText, length_out);

                        // fortran-ify the string
                        if (length_in < length_out) {
                              for (i = length_in; i < length_out; i++) {
                                  retText[i] = ' ';
                              }
                        }
                        *(retText+length_out-1) = ' ';
                        */

                        free(retObj);
                        retObj = NULL;
                    }
                    else {

                    }
                }

            }
        }
    }

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inFlavor) {
        free(inFlavor);
        inFlavor = NULL;
    }

}

int rp_lib_child_num(int* handle,
                     char* path,
                     int path_len)
{
    int numChildren = 0;

    PyObject* lib = NULL;
    PyObject* list = NULL;
    char* inPath = NULL;

    inPath = null_terminate(path,path_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

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


int rp_lib_child_comp(int* handle,    /* integer handle of library */
                      char* path,     /* DOM path of requested object */
                      char* type,     /* specific name of element */
                      int* childNum,  /* child number for iteration */
                      char* retText,  /* buffer to store return text */
                      int path_len,   /* length of path */
                      int type_len,   /* length of type */
                      int retText_len /* length of return text buffer */
                      )
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

    retVal = rp_lib_child_str(handle,
                              inPath,
                              "component",
                              inType,
                              childNum,
                              retText,
                              path_len,
                              10,
                              type_len,
                              retText_len);

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

int rp_lib_child_id(int* handle,    /* integer handle of library */
                    char* path,     /* DOM path of requested object */
                    char* type,     /* specific name of element */
                    int* childNum,  /* child number for iteration */
                    char* retText,  /* buffer to store return text */
                    int path_len,   /* length of path */
                    int type_len,   /* length of type */
                    int retText_len /* length of return text buffer */
                    )
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

    retVal = rp_lib_child_str(handle,
                              inPath,
                              "id",
                              inType,
                              childNum,
                              retText,
                              path_len,
                              3,
                              type_len,
                              retText_len);

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

int rp_lib_child_type(int* handle,    /* integer handle of library */
                      char* path,     /* DOM path of requested object */
                      char* type,     /* specific name of element */
                      int* childNum,  /* child number for iteration */
                      char* retText,  /* buffer to store return text */
                      int path_len,   /* length of path */
                      int type_len,   /* length of type */
                      int retText_len /* length of return text buffer */
                      )
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

    retVal = rp_lib_child_str(handle,
                              inPath,
                              "type",
                              inType,
                              childNum,
                              retText,
                              path_len,
                              5,
                              type_len,
                              retText_len);

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

int rp_lib_child_str(int* handle,    /* integer handle of library */
                     char* path,     /* DOM path of requested object */
                     char* flavor,   /* information you want returned*/
                     char* type,     /* specific name of element */
                     int* childNum,  /* child number for iteration */
                     char* retText,  /* buffer to store return text */
                     int path_len,   /* length of path */
                     int flavor_len, /* length of flavor */
                     int type_len,   /* length of type */
                     int retText_len /* length of return text buffer */
                     )
{
    int retVal = 0;
    // int i = 0;
    // int length_in = 0;

    PyObject* lib = NULL;
    PyObject* list = NULL;
    PyObject* list_item = NULL;

    char* xmlChild = NULL;
    char* inPath = NULL;
    char* inType = NULL;
    char* inFlavor = NULL;

    inPath = null_terminate(path,path_len);
    inFlavor = null_terminate(flavor,flavor_len);
    inType = null_terminate(type,type_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);
            if (lib) {
                if (objType(inFlavor) == 1) {

                    list = rpChildren_f(lib, inPath, inFlavor);
                    if (list) {
                        retVal = PyList_Size(list);
                        // check to make sure our node index is within bounds
                        if (((*childNum)-1) < retVal) {
                            // get the requested node
                            list_item = PyList_GetItem(list,(*childNum)-1);
                            // make node string, borrowed object
                            xmlChild = PyString_AsString(list_item);
                            Py_DECREF(list);
                            // printf("xmlChild = :%s:\n",xmlChild);
                            if (xmlChild) {
                                fortranify(xmlChild, retText, retText_len);

                                /*
                                strncpy(retText, xmlChild, retText_len);
                                length_in = strlen(xmlChild);

                                // fortran-ify the string
                                if (length_in < retText_len) {
                                      for (i = length_in; i < retText_len; i++) {
                                          retText[i] = ' ';
                                      }
                                }
                                // *(retText+retText_len-1) = ' ';
                                */
                            }
                        }
                    }
                }
            }
        }
    }

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inFlavor) {
        free(inFlavor);
        inFlavor = NULL;
    }

    if (inType) {
        free(inType);
        inType = NULL;
    }

    return retVal;
}

int rp_lib_child_obj(int* handle,
                     char* path,
                     char* type,
                     int path_len,
                     int type_len)
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
            lib = getObject(*handle);

            if (lib) {
                list = rpChildren_f(lib, inPath, "object");
                if (list) {
                    // store the whole list,
                    // need to make a way to read the nodes
                    newObjHandle = storeObject(list);
                    // Py_DECREF(list);
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


void rp_lib_get(int* handle, /* integer handle of library */
                char* path,      /* null terminated path */
                char* retText,   /* return text buffer for fortran*/
                int path_len,
                int retText_len /* length of return text buffer */
                )
{
    // int length_in = 0;
    // int length_out = 0;
    // int i = 0;
    const char* xmlText = NULL;

    PyObject* lib = NULL;

    char* inPath = NULL;

    inPath = null_terminate(path,path_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

            if (lib) {
                // retObj is a borrowed object
                // whose contents must not be modified
                xmlText = rpGet(lib, inPath);

                if (xmlText) {

                    fortranify(xmlText, retText, retText_len);

                    /*
                    length_in = strlen(xmlText);
                    length_out = retText_len;

                    strncpy(retText, xmlText, length_out);

                    // fortran-ify the string
                    if (length_in < length_out) {
                          for (i = length_in; i < length_out; i++) {
                              retText[i] = ' ';
                          }
                    }
                    *(retText+length_out-1) = ' ';
                    */
                }
            }
        }
    }

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }
}

double rp_lib_get_double(int* handle,   /* integer handle of library */
                         char* path,    /* null terminated path */
                         int path_len)
{
    double retVal = 0.0;
    const char* xmlText = NULL;

    PyObject* lib = NULL;

    char* inPath = NULL;

    inPath = null_terminate(path,path_len);

    path_len = strlen(inPath) + 1;

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

            if (lib) {
                // retObj is a borrowed object
                // whose contents must not be modified
                xmlText = rpGet(lib, inPath);

                if (xmlText) {
                    retVal = atof(xmlText);
                }
            }
        }
    }

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    return retVal;
}


void rp_lib_put_str(int* handle,
                    char* path,
                    char* value,
                    int* append,
                    int path_len,
                    int value_len)
{
    char* inPath = NULL;
    char* inValue = NULL;

    inPath = null_terminate(path,path_len);
    inValue = null_terminate(value,value_len);

    if (inPath) {
        path_len = strlen(inPath) + 1;
    }
    else {
        path_len = 0;
    }

    if (inValue){
        value_len = strlen(inValue) + 1;
    }
    else {
        value_len = 0;
    }

    rp_lib_put_id_str(handle,inPath,inValue,NULL,append,path_len,value_len,0);

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inValue) {
        free(inValue);
        inValue = NULL;
    }
}


void rp_lib_put_id_str(int* handle,
                       char* path,
                       char* value,
                       char* id,
                       int* append,
                       int path_len,
                       int value_len,
                       int id_len)
{
    PyObject* lib = NULL;

    char* inPath = NULL;
    char* inValue = NULL;
    char* inId = NULL;

    inPath = null_terminate(path,path_len);
    inValue = null_terminate(value,value_len);
    inId = null_terminate(id,id_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

            if (lib) {
                rpPut(lib, inPath, inValue, inId, *append);
            }
        }
    }

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

    if (inValue) {
        free(inValue);
        inValue = NULL;
    }

    if (inId) {
        free(inId);
        inId = NULL;
    }
}

void rp_lib_put_obj(int* handle,
                    char* path,
                    int* valHandle,
                    int* append,
                    int path_len)
{
    char* inPath = NULL;

    inPath = null_terminate(path,path_len);

    if (inPath) {
        path_len = strlen(inPath) + 1;
    }

    rp_lib_put_id_obj(handle,inPath,valHandle,NULL,append,path_len,0);

    if (inPath) {
        free(inPath);
        inPath = NULL;
    }

}

void rp_lib_put_id_obj(int* handle,
                       char* path,
                       int* valHandle,
                       char* id,
                       int* append,
                       int path_len,
                       int id_len)
{
    PyObject* lib = NULL;
    PyObject* value = NULL;

    char* inPath = NULL;
    char* inId = NULL;

    inPath = null_terminate(path,path_len);
    inId = null_terminate(id,id_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);
            value = getObject(*valHandle);

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

int rp_lib_remove(int* handle, char* path, int path_len)
{
    int newObjHandle = -1;

    PyObject* lib = NULL;
    PyObject* removedObj = NULL;

    char* inPath = NULL;

    inPath = null_terminate(path,path_len);

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

            if (lib) {
                removedObj = rpRemove(lib, inPath);

                if (removedObj) {
                    newObjHandle = storeObject(removedObj);
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

int rp_lib_xml_len(int* handle)
{
    int length = -1;
    char* xmlText = NULL;

    PyObject* lib = NULL;

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

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

void rp_lib_xml(int* handle, char* retText, int retText_len)
{
    // int length_in = 0;
    // int length_out = 0;
    // int i = 0;
    char* xmlText = NULL;

    PyObject* lib = NULL;

    if (rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

            if (lib) {
                xmlText = rpXml(lib);

                if (xmlText) {

                    fortranify(xmlText, retText, retText_len);

                    /*
                    length_in = strlen(xmlText);
                    length_out = retText_len;

                    strncpy(retText, xmlText, length_out);

                    // fortran-ify the string
                    if (length_in < length_out) {
                          for (i = length_in; i < length_out; i++) {
                              retText[i] = ' ';
                          }
                    }
                    *(retText+length_out-1) = ' ';
                    */

                    free(xmlText);
                }
            }
        }
    }
}

int rp_lib_write_xml(int* handle, char* outFile, int outFile_len)
{
    int retVal = -1;
    PyObject* lib = NULL;
    FILE* outFH = NULL;
    char* inOutFile = NULL;
    char* xmlText = NULL;

    inOutFile = null_terminate(outFile, outFile_len);

    if (inOutFile && (inOutFile != "") ) {
        outFH = fopen(inOutFile, "w");
    }

    if (outFH && rapptureStarted) {
        if ((handle) && (*handle != 0)) {
            lib = getObject(*handle);

            if (lib) {
                xmlText = rpXml(lib);
                if (xmlText) {
                    retVal = fputs(xmlText,outFH);
                    free(xmlText);
                }
            }
        }
    }

    if (outFH) {
        fclose(outFH);
        outFH = NULL;
    }

    if (inOutFile) {
        free(inOutFile);
        inOutFile = NULL;
    }

    return retVal;
}

void rp_quit()
{

    // clean up python's memory
    // shut down the python interpreter
    // clean up the dictionary

    RpDictEntry DICT_TEMPLATE *hPtr;
    // RpDictIterator DICT_TEMPLATE iter((RpDict&)*this);
    RpDictIterator DICT_TEMPLATE iter(fortObjDict);

    hPtr = iter.first();

    while (hPtr) {
        Py_DECREF(*(hPtr->getValue()));
        hPtr->erase();
        hPtr = iter.next();
    }

    if (fortObjDict.size()) {
        // probably want to change the warning sometime
        // printf("\nWARNING: internal dictionary is not empty..deleting\n");
    }

    rapptureStarted = 0;
    // Py_Finalize();
}

void rp_result(int* handle)
{
    PyObject* lib = NULL;

    if (rapptureStarted && handle && *handle != 0) {
        lib = getObject(*handle);
        if (lib) {
            rpResult(lib);
        }
    }
    rp_quit();
}

int objType( char* flavor)
{
    if (flavor == NULL) {
        // return a PyObject*
        return 0;
    }
    else if ((*flavor == 'o') && (strncmp(flavor,"object", 6) == 0)) {
        // return a PyObject*
        return 0;
    }
    else if (((*flavor == 't') && (strncmp(flavor,"type", 4) == 0)) ||
             ((*flavor == 'i') && (strncmp(flavor,"id", 2) == 0)) ||
             ((*flavor == 'c') && (strncmp(flavor,"component", 9) == 0)))
    {
        // return a char*
        // convert the result to c style strings
        return 1;
    }
    else {
        // unrecognized format
        return -1;
    }
}

int storeObject(PyObject* objectName)
{
    int retVal = -1;
    int dictKey = fortObjDict.size();
    int newEntry = 0;

    if (objectName) {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.
        fortObjDict.set(dictKey,objectName, &newEntry);
    }

    retVal = dictKey;
    return retVal;
}

PyObject* getObject(int objKey)
{
    PyObject* retVal = *(fortObjDict.find(objKey).getValue());

    if (retVal == *(fortObjDict.getNullEntry().getValue())) {
        retVal = NULL;
    }

   return retVal;
}
