#include "Python.h"
#include <stdlib.h>
#include <time.h>

// #include "Python.h"

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
char* rpGet       (PyObject* lib, const char* path);
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
char* rpXml       (PyObject* lib);

void        rpResult    (PyObject* lib);

/*
#ifdef _cplusplus 
}
#endif    
*/
