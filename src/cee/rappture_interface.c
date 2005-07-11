#include "../include/rappture_interface.h"

/**********************************************************************/
// FUNCTION: PyObject* importRappture()
/// fxn to mimic/implement the python call "import Rappture"
/**
 * Clients use this function to import the Rappture module into
 *          the Python Interpretor.
 *
 * Notes: It is the client's responsibility to increment and 
 *          decrement the reference count of the returned object
 *          as needed. 
 *
 * Returns pointer (PyObject*) to the Rappture.library class 
 *          if successful, or NULL if something goes wrong.
 */


PyObject* importRappture() 
{
    
    // it is the callers responsibility to Py_DECREF the returned object 
    // if it exists.

    PyObject* pName     = NULL;
    PyObject* pModule   = NULL;
    PyObject* mClass    = NULL;
    PyObject* retVal    = NULL;    /* return object */

    // create a string object holding the name of the module
    pName = PyString_FromString("Rappture");

    if (pName) {
        pModule = PyImport_Import(pName);
        if (pModule) {
            /* fetch module.class */
            mClass = PyObject_GetAttrString(pModule, "library"); 
            Py_DECREF(pModule);

            if (mClass) {
                // try to instantiate the class
                if (PyCallable_Check(mClass)) {

                    retVal = mClass;
                    
                }
                else {
                    // mClass not callable
                    // something went wrong
                    // python will probably raise error
                }

                // not sure if we need to increase and decrease
                // increase because we are returning a ref to the object
                // decrease because this function no longer needs the ref.
                // 
                // the decrease is left as an exercise to the calling fxn.
                // 
                // Py_DECREF(mClass);
            }

            // Py_DECREF(pModule);
        }
        else {
            // pModule was not created
        }

        Py_DECREF(pName);
    }
    else {
        // pName was not able to be created as a PyObject string
    }       

    return retVal;

}  

/**********************************************************************/
// FUNCTION: PyObject* createRapptureObj(PyObject* rpObj, const char* path)
/// fxn to mimic/implement the python call "lib = Rappture.library(".")"
/// fxn instatiates a Rappture Object and returns it to the caller.
/**
 * Clients use this function to create a Rappture object which can 
 *          access the Rappture member functions.
 *
 * Notes: It is the client's responsibility to increment and 
 *          decrement the reference count of the returned object
 *          as needed. 
 *        
 *        Neither of the two arguments (rpObj,path) are optional.
 *          if either value is NULL, NULL will be returned.
 *
 * Returns pointer (PyObject*) to an instantiated Rappture object
 *          if successful, or NULL if something goes wrong.
 */

PyObject* createRapptureObj (PyObject* rpObj, const char* path)
{
    PyObject* args      = NULL;      /* args tuple */
    PyObject* stringarg = NULL;      /* string of arguments */
    PyObject* lib       = NULL;      /* results from fxn call */
    PyObject* retVal    = NULL;      /* fxn return value */


    if (rpObj) {
        if (path) {
            // setup our arguments in a Python tuple
            args = PyTuple_New(1);
            stringarg = PyString_FromString(path);
            PyTuple_SetItem(args, 0, stringarg);
                    
            // call the class ... lib = Rappture.library("...")
            lib = PyObject_CallObject(rpObj, args);
            
            // following line could cause a segfault if used in place of above
            // maybe path could == NULL or bad memory
            // lib = PyObject_CallFunction(rpObj,"(s)", path);

            if (lib) {
                // return the Rappture instantiation.
                retVal = lib;
            }
            else {
                // lib was not successfully created
            }

            Py_DECREF(stringarg);
            Py_DECREF(args);
        }
        else {
            // path was null
        }
    } 
    else {
        // rpObj was null
    }

    return retVal;

}

/**********************************************************************/
// FUNCTION: void* rpElement(PyObject* lib, const char* path, const char* flavor)
/// fxn to mimic/implement the python call "element = lib.element("..")"
/// fxn calls the element() member function of the Rappture.library
///     class and returns the text to the caller
/**
 * Clients use this function to call the element() member function of the 
 *          Rappture.library class and get the text of the Rappture 
 *          object lib.
 *
 * Notes:   If (flavor == "object") || (flavor == NULL),
 *          1) it is the client's responsibility to increment and 
 *             decrement the reference count of the returned object
 *             as needed.
 *          2) the return object will need to be cast as a PyObject* 
 *             to use it.
 *
 *          If (flavor == type) || (flavor == id) || (flavor == component),
 *          1) the return object will need to be cast as a char*
 *             to use it.
 *        
 *        The Arguments lib and pathare not optional.
 *          If either value is NULL, NULL will be returned.
 *
 * Returns pointer (char*) to the c style string representing the
 *          xml text of the Rappture object if successful, or 
 *          NULL if something goes wrong.
 */

void* rpElement(PyObject* lib, const char* path, const char* flavor)
{
    PyObject* mbr_fxn   = NULL;      /* pointer to fxn of class lib */
    PyObject* rslt      = NULL;      /* results from fxn call */
    void* retVal        = NULL;      /* fxn return value */

    if (lib && path) {
        
        // retrieve the lib.element() function
        mbr_fxn = PyObject_GetAttrString(lib, "element");

        if (mbr_fxn) {

            if (PyCallable_Check(mbr_fxn)) {
                
                // call the lib.element() function with arguments
                if (flavor) {
                    rslt = PyObject_CallFunction(mbr_fxn,"(ss)", path, flavor);
                }
                else {
                    rslt = PyObject_CallFunction(mbr_fxn,"(s)", path);
                }

                if (rslt) {

                    if (flavor == NULL) {
                        // return a PyObject*
                        retVal = (void*) rslt;
                    }
                    else if((*flavor == 'o')&&(strncmp(flavor,"object", 6)==0)){
                        // return a PyObject*
                        retVal = (void*) rslt;
                    }
                    else if ( 
                      ((*flavor == 't')&&(strncmp(flavor,"type", 4) == 0)) 
                    ||((*flavor == 'i')&&(strncmp(flavor,"id", 2) == 0))
                    ||((*flavor == 'c')&&(strncmp(flavor,"component", 9) == 0)))
                    {
                        // return a char*
                        // convert the result to c style strings
                        retVal = (void*) PyString_AsString(rslt);
                        Py_DECREF(rslt);
                    }
                    else {
                        // unrecognized format
                    }
                    
                    // Py_DECREF(rslt);

                } // rslt was null
                
            } // mbr_fxn was not callable
            
            Py_DECREF(mbr_fxn);
        } // mbr_fxn was null
    }
    else {
        // lib or path was set to NULL
    }

    return retVal;
}


/**********************************************************************/
// FUNCTION: void** rpChildren(PyObject* lib, const char* path, const char* flavor)
/// fxn to mimic/implement the python call "xmlText = lib.children("..")"
/// fxn calls the element() member function of the Rappture.library
///     class and returns the text to the caller
/**
 * Clients use this function to call the xml() member function of the 
 *          Rappture.library class and get the text of the Rappture 
 *          object lib.
 *
 * Notes:   If (flavor == "object") || (flavor == NULL),
 *          1) it is the client's responsibility to increment and 
 *             decrement the reference count of the returned object
 *             as needed.
 *          2) the return object will need to be cast as a PyObject* 
 *             to use it.
 *
 *          If (flavor == "type")||(flavor == "id")||(flavor == "component"),
 *          1) the return object will need to be case as a char*
 *             to use it.
 *        
 *        The Arguments lib and pathare not optional.
 *          If either value is NULL, NULL will be returned.
 *
 * Returns pointer (char*) to the c style string representing the
 *          xml text of the Rappture object if successful, or 
 *          NULL if something goes wrong.
 */

void** rpChildren(PyObject* lib, const char* path, const char* flavor)
{
    PyObject* mbr_fxn   = NULL;      /* pointer to fxn of class lib */
    PyObject* rslt      = NULL;      /* results from fxn call */
    PyObject* list_item = NULL;      /* item from list of returned children */
//    void* retVal        = NULL;      /* fxn return value */
    int list_size       = 0;
    int index           = 0;
    char** rslt_arr_c   = NULL;
    void** rslt_arr  = NULL;

    if (lib && path) {
        
        // retrieve the lib.children() function
        mbr_fxn = PyObject_GetAttrString(lib, "children");

        if (mbr_fxn) {

            if (PyCallable_Check(mbr_fxn)) {
                
                // call the lib.element() function with arguments
                if (flavor) {
                    rslt = PyObject_CallFunction(mbr_fxn,"(ss)", path, flavor);
                }
                else {
                    rslt = PyObject_CallFunction(mbr_fxn,"(s)", path);
                }

                if (rslt) {

                    if (flavor == NULL) {
                        // return a PyObject*
                        // retVal = (void*) rslt;
                        // 
                        // the PyObject* points to a list, so we need to
                        // convert this to an array of char*'s

                        list_size = PyList_Size(rslt);
                        rslt_arr = (void**) calloc(list_size+1,sizeof(void*));
                        if (rslt_arr) {
                            while (index < list_size) {
                                // list_item is a borrowed reference
                                list_item = PyList_GetItem(rslt,index);
                                rslt_arr[index] = (void*) (list_item);
                                index++;
                            }
                            // *(rslt_arr[index]) = (void*)NULL;
                        }
                    }
                    else if((*flavor == 'o')&&(strncmp(flavor,"object", 6)==0)){
                        // return a PyObject*
                        // retVal = (void*) rslt;
                        // 
                        // the PyObject* points to a list, so we need to
                        // convert this to an array of char*'s

                        list_size = PyList_Size(rslt);
                        rslt_arr = (void**) calloc(list_size+1,sizeof(void*));
                        if (rslt_arr) {
                            while (index < list_size) {
                                // list_item is a borrowed reference
                                list_item = PyList_GetItem(rslt,index);
                                rslt_arr[index] = (void*) (list_item);
                                index++;
                            }
                            // *(rslt_arr[index]) = NULL;
                        }
                    }
                    else if ( 
                      ((*flavor == 't')&&(strncmp(flavor,"type", 4) == 0)) 
                    ||((*flavor == 'i')&&(strncmp(flavor,"id", 2) == 0))
                    ||((*flavor == 'c')&&(strncmp(flavor,"component", 9) == 0)))
                    {
                        // the PyObject* points to a list, so we need to
                        // convert this to an array of char*'s

                        list_size = PyList_Size(rslt);
                        rslt_arr_c = (char**) calloc(list_size+1,sizeof(char*));
                        if (rslt_arr_c) {
                            while (index < list_size) {
                                // list_item is a borrowed reference
                                list_item = PyList_GetItem(rslt,index);
                                // we cannot deallocate the results of 
                                // PyString_AsString()
                                rslt_arr_c[index] = PyString_AsString(list_item);
                                index++;
                                
                            }
                            // *(rslt_arr[index]) = NULL;
                            rslt_arr = (void**) rslt_arr_c;
                        }
                        // return a char*
                        // convert the result to c style strings
                        // retVal = (void*) PyString_AsString(rslt);
                        Py_DECREF(rslt);
                    }
                    else {
                        // unrecognized format
                    }
                    
                    // Py_DECREF(rslt);

                } // rslt was null
                
            } // mbr_fxn was not callable
            
            Py_DECREF(mbr_fxn);
        } // mbr_fxn was null
    }
    else {
        // lib or path was set to NULL
    }

    // return retVal;
    return (void**) rslt_arr;
}

/**********************************************************************/
// FUNCTION: PyObject* rpChildren_f(PyObject* lib, const char* path, const char* flavor)
/// fxn to mimic/implement the python call "xmlText = lib.children("..")"
/// fxn calls the element() member function of the Rappture.library
///     class and returns the text to the caller
/**
 * Clients use this function to call the xml() member function of the 
 *          Rappture.library class and get the text of the Rappture 
 *          object lib.
 *
 * Notes:   If (flavor == "object") || (flavor == NULL),
 *          1) it is the client's responsibility to increment and 
 *             decrement the reference count of the returned object
 *             as needed.
 *          2) the return object will need to be cast as a PyObject* 
 *             to use it.
 *
 *          If (flavor == "type")||(flavor == "id")||(flavor == "component"),
 *          1) the return object will need to be case as a char*
 *             to use it.
 *        
 *        The Arguments lib and pathare not optional.
 *          If either value is NULL, NULL will be returned.
 *
 * Returns pointer (char*) to the c style string representing the
 *          xml text of the Rappture object if successful, or 
 *          NULL if something goes wrong.
 */

PyObject* rpChildren_f(PyObject* lib, const char* path, const char* flavor)
{
    PyObject* mbr_fxn   = NULL;      /* pointer to fxn of class lib */
    PyObject* rslt      = NULL;      /* results from fxn call */
//    PyObject* list_item = NULL;      /* item from list of returned children */
//    void* retVal        = NULL;      /* fxn return value */
//    int list_size       = 0;
//    int index           = 0;
//    char** rslt_arr_c   = NULL;
//    void** rslt_arr  = NULL;

//    char* xmlChild = NULL;

    if (lib && path) {
        
        // retrieve the lib.children() function
        mbr_fxn = PyObject_GetAttrString(lib, "children");

        if (mbr_fxn) {

            if (PyCallable_Check(mbr_fxn)) {
                
                // call the lib.children() function with arguments
                if (flavor) {
                    rslt = PyObject_CallFunction(mbr_fxn,"(ss)", path, flavor);
                }
                else {
                    rslt = PyObject_CallFunction(mbr_fxn,"(s)", path);
                }

                /*
                if (rslt) {
                    list_size = PyList_Size(rslt);
                    while (index < list_size) {
                        // list_item is a borrowed reference
                        list_item = PyList_GetItem(rslt,index);
                        // we cannot deallocate the results of 
                        // PyString_AsString()
                        // printf("list item = :%x:\n", list_item);
                        xmlChild = PyString_AsString(list_item);
                        printf("xml = :%s:\n",xmlChild);
                        index++;
                    }
                }
                */

                /*
                if (rslt) {
                    list_item = PyList_GetItem(rslt,0);
                    xmlChild = PyString_AsString(list_item);
                    printf("xml0 = :%s:\n",xmlChild);
                    list_item = PyList_GetItem(rslt,1);
                    xmlChild = PyString_AsString(list_item);
                    printf("xml1 = :%s:\n",xmlChild);
                } // rslt was null
                */
                
            } // mbr_fxn was not callable
            
            Py_DECREF(mbr_fxn);
        } // mbr_fxn was null
    }
    else {
        // lib or path was set to NULL
    }

    return rslt;
}

/**********************************************************************/
// FUNCTION: const char* rpGet(PyObject* lib, const char* path)
/// fxn to mimic/implement the python call "xmlText = lib.get('...')"
/// fxn calls the get() member function of the Rappture.library class
///     and returns the text to the caller
/**
 * Clients use this function to call the get() member function of the 
 *          Rappture.library class and get the text of the Rappture 
 *          object lib.
 *
 * Notes: The argument (lib) is not optional. If it is NULL, 
 *          NULL will be returned.
 *
 * Returns pointer (const char*) to the c style string representing the
 *          xml text provided by the path of the Rappture object if 
 *          successful, or NULL if something goes wrong.
 *
 *          The return value's contents should not be changed because 
 *          it is a pointer being borrowed from python's buffer space
 *          (hence the _const_)
 */
const char* rpGet(PyObject* lib, const char* path)
{
    PyObject* mbr_fxn   = NULL;      /* pointer to fxn of class lib */
    PyObject* rslt      = NULL;      /* results from fxn call */
    char* retVal        = NULL;      /* return value */

    if (lib) {
        // retrieve the lib.get() function
        mbr_fxn = PyObject_GetAttrString(lib, "get");

        if (mbr_fxn) {

            if (PyCallable_Check(mbr_fxn)) {
                
                // call the lib.get() function
                rslt = PyObject_CallFunction(mbr_fxn,"(s)", path);
                if (rslt) {
                    // convert the result to c style strings
                    retVal = PyString_AsString(rslt);
                    // Py_DECREF(rslt);
                }
                
            }
            Py_DECREF(mbr_fxn);
        }
    }
    else {
        // lib was set to NULL
    }

    return (const char*) retVal;
    
}

/**********************************************************************/
// FUNCTION: void rpPut( PyObject* lib, 
//                       const char* path, 
//                       const char* value, 
//                       const char* id, 
//                       int append)
/// fxn to mimic/implement the python call "lib.put('...')"
/// fxn calls the put() member function of the Rappture.library class
///     and creates user specified fields within xml document
/**
 * Clients use this function to call the put() member function of the 
 *          Rappture.library class and place custom text into the 
 *          Rappture object lib.
 *
 * Notes: The argument (lib) is not optional. If it is NULL, 
 *          NULL will be returned.
 *
 * Returns nothing.
 */
void rpPut( PyObject* lib, 
            const char* path, 
            const char* value, 
            const char* id, 
            int append )
{
    PyObject* mbr_fxn   = NULL;      /* pointer to fxn of class lib */

    if (lib) {
        // retrieve the lib.xml() function
        mbr_fxn = PyObject_GetAttrString(lib, "put");

        if (mbr_fxn) {

            if (PyCallable_Check(mbr_fxn)) {
                PyObject_CallFunction(mbr_fxn,"(sssi)",path,value,id,append);
            }
            Py_DECREF(mbr_fxn);
        }
    }
    else {
        // lib was set to NULL
    }

}

/**********************************************************************/
// FUNCTION: void rpPutObj( PyObject* lib, 
//                       const char* path, 
//                       const char* value, 
//                       const char* id, 
//                       int append)
/// fxn to mimic/implement the python call "lib.put('...')"
/// fxn calls the put() member function of the Rappture.library class
///     and creates user specified fields within xml document
/**
 * Clients use this function to call the put() member function of the 
 *          Rappture.library class and place custom text into the 
 *          Rappture object lib.
 *
 * Notes: The argument (lib) is not optional. If it is NULL, 
 *          NULL will be returned.
 *
 * Returns nothing.
 */
void rpPutObj( PyObject* lib, 
            const char* path, 
            PyObject* value, 
            const char* id, 
            int append )
{
    PyObject* mbr_fxn   = NULL;      /* pointer to fxn of class lib */

    if (lib) {
        // retrieve the lib.xml() function
        mbr_fxn = PyObject_GetAttrString(lib, "put");

        if (mbr_fxn) {

            if (PyCallable_Check(mbr_fxn)) {
                PyObject_CallFunction(mbr_fxn,"(sOsi)",path,value,id,append);
                // PyObject_CallFunctionObjArgs(mbr_fxn,path,value,id,append,NULL);
            }
            Py_DECREF(mbr_fxn);
        }
    }
    else {
        // lib was set to NULL
    }

}

/**********************************************************************/
// FUNCTION: PyObject* rpRemove(PyObject* lib, const char* path)
/// fxn to mimic/implement the python call "lib.remove('...')"
/// fxn calls the remove() member function of the Rappture.library class
///     and removes the elements listed, if they exist
/**
 * Clients use this function to call the remove() member function of the 
 *          Rappture.library class and remove the elements of the 
 *          Rappture object lib.
 *
 * Notes: The argument (lib) is not optional. If it is NULL, 
 *          NULL will be returned.
 *
 * Returns nothin.
 *
 */
PyObject* rpRemove(PyObject* lib, const char* path)
{
    PyObject* mbr_fxn   = NULL;      /* pointer to fxn of class lib */
    PyObject* rslt      = NULL;      /* results from fxn call */
    
//    PyObject* args = NULL;
//    PyObject* stringarg = NULL;


    if (lib) {
        // retrieve the lib.remove() function
        mbr_fxn = PyObject_GetAttrString(lib, "remove");

        if (mbr_fxn) {

            if (PyCallable_Check(mbr_fxn)) {

                // setup our arguments in a Python tuple
                // args = PyTuple_New(1);
                // stringarg = PyString_FromString(path);
                // PyTuple_SetItem(args, 0, stringarg);

                // call the class ... lib = Rappture.library("...")
                // rslt = PyObject_CallObject(mbr_fxn, args);

                
                // call the lib.xml() function with no arguments
                rslt = PyObject_CallFunction(mbr_fxn,"(s)", path);
                // it is caller's responsibility to adjust reference count
                // Py_DECREF(rslt);
                
            }
            Py_DECREF(mbr_fxn);
        }
    }
    else {
        // lib was set to NULL
    }

    return rslt;
    
}

/**********************************************************************/
// FUNCTION: const char* rpXml(PyObject* lib)
/// fxn to mimic/implement the python call "xmlText = lib.xml()"
/// fxn calls the xml() member function of the Rappture.library class
///     and returns the text to the caller
/**
 * Clients use this function to call the xml() member function of the 
 *          Rappture.library class and get the text of the Rappture 
 *          object lib.
 *
 * Notes: The argument (lib) is not optional. If it is NULL, 
 *          NULL will be returned.
 *
 * Returns pointer (const char*) to the c style string representing the
 *          xml text of the Rappture object if successful, or 
 *          NULL if something goes wrong.
 *
 *          The return value's contents should not be changed because 
 *          it is a pointer being borrowed from python's buffer space
 *          (hence the _const_)
 */


const char* rpXml(PyObject* lib)
{
    PyObject* mbr_fxn   = NULL;      /* pointer to fxn of class lib */
    PyObject* rslt      = NULL;      /* results from fxn call */
    char* retVal        = NULL;      /* return value */

    if (lib) {
        // retrieve the lib.xml() function
        mbr_fxn = PyObject_GetAttrString(lib, "xml");

        if (mbr_fxn) {

            if (PyCallable_Check(mbr_fxn)) {
                
                // call the lib.xml() function
                rslt = PyObject_CallFunction(mbr_fxn,NULL);
                if (rslt) {
                    // convert the result to c style strings
                    retVal = PyString_AsString(rslt);
                    Py_DECREF(rslt);
                }
                
            }
            Py_DECREF(mbr_fxn);
        }
    }
    else {
        // lib was set to NULL
    }

    return (const char*) retVal;
    
}
