// #include <stdio.h>
// #include <assert.h>
// #include <python2.3/Python.h> // "/usr/include/python2.3/Python.h" //  
#include "python2.4/Python.h" // "/usr/include/python2.3/Python.h" //  

/*
#ifdef _cplusplus 
extern "C" {
#endif    
*/

PyObject* importRappture    ();
PyObject* createRapptureObj (PyObject* rpObj, const char* path);

void*       rpElement   (PyObject* lib, const char* path, const char* flavor);
void**      rpChildren  (PyObject* lib, const char* path, const char* flavor);
PyObject*   rpChildren_f  (PyObject* lib, const char* path, const char* flavor);
const char* rpGet       (PyObject* lib, const char* path);
void        rpPut       (PyObject* lib, 
                            const char* path, 
                            const char* value, 
                            const char* id, 
                            int append );
void        rpPutObj    (PyObject* lib,
                            const char* path,
                            PyObject* value,
                            const char* id,
                            int append );

PyObject*   rpRemove    (PyObject* lib, const char* path);
const char* rpXml       (PyObject* lib);

/*
#ifdef _cplusplus 
}
#endif    
*/
