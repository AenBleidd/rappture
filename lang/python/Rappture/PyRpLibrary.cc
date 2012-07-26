
/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2008  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <Python.h>
#include <RpLibrary.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0

#define RP_OK	0
#define RP_ERROR 1

#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif

/*
 * As suggested by PEP353
 * http://www.python.org/dev/peps/pep-0353/
 */

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

static PyObject *ErrorObject;
static RpLibrary * RpLibraryObject_AsLibrary(PyObject *lib);
static PyObject * RpLibraryObject_FromLibrary(RpLibrary *lib);
static int boolAsInt(const char *inVal, int *outVal);
static int boolIntFromPyObject (PyObject *inPyObj, const char *defaultVal,
	const char *argName, int *boolVal);
static int getArgCount(PyObject *args, PyObject *keywds, int *argc);

typedef struct {
    PyObject_HEAD
    RpLibrary *lib;
} RpLibraryObject;

/*
 * Check to see if this object is a Rappture.library object.
 * This does not check to see if the object is correctly
 * initialized. To check for correct object type and
 * correct initialization, use RpLibraryObject_IsValid.
 */
static int
RpLibraryObject_Check(PyObject *objPtr)
{
    if (objPtr == NULL) {
	return FALSE;
    }
    if (strcmp(objPtr->ob_type->tp_name, "Rappture.library") != 0) {
	return FALSE;
    }
    return TRUE;
}

/*
 * Check to see if this object is a Rappture.library object
 * and is correctly initialized
 *
 * This function calls RpLibraryObject_Check internally, and is used
 * in most of the internal functions to check if an object being passed
 * in by the user is a valid Rappture.library object
 */
static int
RpLibraryObject_IsValid(PyObject *objPtr)
{
    if (objPtr == NULL) {
	return FALSE;			// Passed a NULL pointer. Should
					// probably segfault.
    }
    if (!RpLibraryObject_Check(objPtr)) {
	return FALSE;			// Not a rappture library object type.
    }
    if (((RpLibraryObject *)objPtr)->lib == NULL) {
	return FALSE;			// Is a rappture library object, but
					// doesn't contain a valid library.
    }
    return TRUE;
}

static PyObject *
RpLibraryObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    RpLibraryObject *self = NULL;

    self = (RpLibraryObject *)type->tp_alloc(type,0);
    if (self != NULL) {
        self->lib = NULL;
    }
    else {
        PyErr_SetString(PyExc_RuntimeError,
		"trouble creating new RpLibraryObject");
    }
    return (PyObject *)self;
}

static int
RpLibraryObject_init(RpLibraryObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *inObj = NULL;
    char *filename = NULL;
    int retVal = 1;

    if (!PyArg_ParseTuple(args, "|O", &inObj)) {
        PyErr_Format(PyExc_TypeError,
            "library() takes at most 1 argument, a file name or a Rappture Library Object");
        return -1;
    }

    if (inObj != NULL) {
        if (PyString_Check(inObj)) {
            filename = PyString_AsString(inObj);
            if (filename == NULL) {
                PyErr_Format(PyExc_ValueError,"a file name is required");
            }
            self->lib = new RpLibrary(std::string(filename));
        }
        else if (RpLibraryObject_IsValid(inObj)) {
            self->lib = new RpLibrary( *(RpLibraryObject_AsLibrary(inObj)) );
        }
        else if (RpLibraryObject_Check(inObj)) {
            self->lib = new RpLibrary();
        }
        else {
            PyErr_Format(PyExc_TypeError,"unrecognized object type");
            return -1;
        }
    }
    else {
        self->lib = new RpLibrary();
    }

    return retVal;
}

static void
RpLibraryObject_dealloc(RpLibraryObject *self)
{
    if (self) {
        if (self->lib){
            delete(self->lib);
        }
        self->ob_type->tp_free((PyObject *) self);
    }
}

PyDoc_STRVAR(RpLibraryObject_copy_doc,
"copy (topath, frompath [, fromobj=$self]) -> None, copies data\n\
\n\
Copy an xml element to \'topath\' of the current this object\n\
from \'frompath\' of the Rappture Library Object \'fromobj\'.\n\
\n\
\'topath\' is the path (in this object) where the data should\n\
be copied to\n\
\'frompath\' is an path (in fromobj) where the data should be\n\
copied from\n\
\'fromobj\' is the Rappture Library Object to copy the data from\n\
if fromobj is not set, the data is copied from the same object\n\
the data will be copied to (this object).\n\
\n\
");

static PyObject *
RpLibraryObject_copy(RpLibraryObject *self, PyObject *args, PyObject *keywds)
{
    char *topath = (char *)"";
    char *frompath = (char *)"";
    int argc = 0;
    PyObject *fromobj = (PyObject *) self;

    static char *kwlist[] = {
        (char *)"topath",
        (char *)"frompath",
        (char *)"fromobj",
        NULL
    };

    if (self->lib == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
            "self is uninitialized Rappture Library Object");
        return NULL;
    }

    if (getArgCount(args,keywds,&argc) != RP_OK) {
        // trouble ensues
        // error message was set in getArgCount()
        return NULL;
    }

    if (argc > 3) {
        // tested with CopyTests.testArguments_TooManyArgs()
        PyErr_Format(PyExc_TypeError,
            "copy() takes at most 3 arguments (%i given)",argc);
        return NULL;
    }

    if (argc < 2) {
        // tested with CopyTests.testArguments_NotEnoughArgs()
        PyErr_Format(PyExc_TypeError,
            "copy() takes at least 2 arguments (%i given)",argc);
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|O",
            kwlist, &topath, &frompath, &fromobj)) {
        // tested with CopyTests.testArguments_ArgsWrongType2()
        PyErr_Format(PyExc_TypeError,
            "copy() takes 2 sting and 1 Rappture Library Object as arguments");
        return NULL;
    }

    if (!RpLibraryObject_IsValid(fromobj)) {
        // tested with CopyTests.testArguments_ArgsWrongType3()
        PyErr_SetString(PyExc_RuntimeError,
            "incorrectly initialized Rappture Library Object");
        return NULL;
    }

    self->lib->copy(std::string(topath),
        RpLibraryObject_AsLibrary(fromobj),
        std::string(frompath));

    Py_RETURN_NONE;
}

PyDoc_STRVAR(RpLibraryObject_element_doc,
"element ([path=\'\'][, as=\'object\']) -> returns string or Rappture Library Object\n\
\n\
Clients use this to query a particular element within the \n\
entire data structure.  The path is a string of the form \n\
\"structure.box(source).corner\".  This example represents \n\
the tag <corner> within a tag <box id=\"source\"> within a \n\
a tag <structure>, which must be found at the top level \n\
within this document. \n\
\n\
By default, this method returns an object representing the \n\
DOM node referenced by the path.  This is changed by setting \n\
the \"as\" argument to \"id\" (for name of the tail element), \n\
to \"type\" (for the type of the tail element), to \"component\" \n\
(for the component name \"type(id)\"), or to \"object\" \n\
for the default (an object representing the tail element).\n\
");

static PyObject *
RpLibraryObject_element(RpLibraryObject *self, PyObject *args, PyObject *keywds)
{
    char* path = (char *)"";
    char* as = (char *)"object";
    RpLibrary* retlib = NULL;
    PyObject* retVal = NULL;
    int argc = 0;
    int status = 0;

    static char *kwlist[] = {
        (char *)"path",
        (char *)"as",
        NULL
    };

    if (self->lib == NULL) {
        PyErr_Format(PyExc_RuntimeError,
            "self is uninitialized Rappture Library Object");
        return retVal;
    }

    if (getArgCount(args,keywds,&argc) != RP_OK) {
        // trouble ensues
        // error message was set in getArgCount()
        return NULL;
    }

    if (argc > 2) {
        // tested with ElementTests.testArguments_TooManyArgs()
        PyErr_Format(PyExc_TypeError,
            "element() takes at most 2 arguments (%i given)", argc);
        return retVal;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|ss",
                kwlist, &path, &as)) {
        /* incorrect input values */
        // tested with ElementTests.testArguments_ArgsWrongType2()
        PyErr_Format(PyExc_TypeError,"element ([path=\'\'][, as=\'object\'])");
        return retVal;
    }

    retlib = self->lib->element(std::string(path));

    if (retlib != NULL) {
        if (   (as == NULL)
            || ((*as == 'o') && (strcmp("object",as) == 0)) ) {
            // tested with ElementTests.testArguments_PathArg()
            retVal = RpLibraryObject_FromLibrary(retlib);
        }
        else if ((*as == 'c') && (strcmp("component",as) == 0)) {
            // tested with ElementTests.testArguments_TwoArgs()
            retVal = PyString_FromString(retlib->nodeComp().c_str());
        }
        else if ((*as == 'i') && (strcmp("id",as) == 0)) {
            // tested with ElementTests.testArguments_AsId()
            retVal = PyString_FromString(retlib->nodeId().c_str());
        }
        else if ((*as == 't') && (strcmp("type",as) == 0)) {
            // tested with ElementTests.testArguments_AsKeywordArgs()
            retVal = PyString_FromString(retlib->nodeType().c_str());
        }
        else if ((*as == 'p') && (strcmp("path",as) == 0)) {
            // tested with ElementTests.testArguments_TwoKeywordArgs()
            retVal = PyString_FromString(retlib->nodePath().c_str());
        }
        else {
            // tested with ElementTests.testArguments_UnrecognizedAs()
            PyErr_Format(PyExc_ValueError,
                "element() \'as\' arg must be \'object\' or \'component\' or \'id\' or \'type\' or \'path\'");
        }
    }

    return (PyObject *)retVal;
}


PyDoc_STRVAR(RpLibraryObject_get_doc,
"get ([path=\'\'][, decode=\'True\']) -> returns data at \'path\' as string\n\
\n\
Clients use this to query the value of a node.  If the path\n\
is not specified, it returns the value associated with the\n\
root node.  Otherwise, it returns the value for the element\n\
specified by the path.\n\
");

static PyObject *
RpLibraryObject_get(RpLibraryObject *self, PyObject *args, PyObject *keywds)
{
    char* path = (char *)"";

    PyObject* decode = NULL;
    int decodeVal = FALSE;

    PyObject* retVal = NULL;
    std::string retValStr = "";
    Rappture::Buffer retValBuf;

    int argc = 0;
    int status = 0;

    static char *kwlist[] = {
        (char *)"path",
        (char *)"decode",
        NULL
    };

    if (self->lib == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
            "self uninitialized Rappture Library Object");
        return NULL;
    }

    if (getArgCount(args,keywds,&argc) != RP_OK) {
        // trouble ensues
        // error message was set in getArgCount()
        return NULL;
    }

    if (argc > 2) {
        // tested with GetTests.testArguments_TooManyArgs()
        PyErr_Format(PyExc_TypeError,
            "get() takes at most 2 arguments (%i given)",argc);
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|sO",
            kwlist, &path, &decode)) {
        // tested with GetTests.testArguments_ArgsWrongType2()
        PyErr_Format(PyExc_TypeError,"get ([path=\'\'][, decode=\'True\'])");
        return NULL;
    }
    if (boolIntFromPyObject(decode,"yes", "decode", &decodeVal) != RP_OK) {
        // tested with GetTests.testArgumentsDecodeError()
        return NULL;
    }

    if (decodeVal) {
        // tested with GetTests.testArgumentsDecode()
        // tested with GetTests.testArgumentsDecodeYes()
        // tested with GetTests.testArgumentsDecodeTrue)
        // tested with GetTests.testArgumentsDecodeOn()
        // tested with GetTests.testArgumentsDecode1Str()
        // tested with GetTests.testArgumentsDecode1Int()
        retValStr = self->lib->get(std::string(path));
        retVal = PyString_FromStringAndSize(retValStr.c_str(),retValStr.size());
    }
    else {
        // tested with GetTests.testArgumentsNoDecodeNo()
        // tested with GetTests.testArgumentsNoDecodeFalse()
        // tested with GetTests.testArgumentsNoDecodeOff()
        // tested with GetTests.testArgumentsNoDecode0Str()
        // tested with GetTests.testArgumentsNoDecode0Int()
        retValBuf = self->lib->getData(std::string(path));
        retVal = PyString_FromStringAndSize(retValBuf.bytes(),retValBuf.size());
    }
    return (PyObject *)retVal;
}

PyDoc_STRVAR(RpLibraryObject_parent_doc,
"parent ([path=\'\'][, as=\'object\']) -> returns string or Rappture Library Object\n\
\n\
Clients use this to query the parent of a particular element \n\
This is just like the \'element()\' method, but it returns the parent \n\
of the element instead of the element itself. \n\
\n\
By default, this method returns an object representing the \n\
DOM node referenced by the path.  This is changed by setting \n\
the \"as\" argument to \"id\" (for name of the tail element), \n\
to \"type\" (for the type of the tail element), to \"component\" \n\
(for the component name \"type(id)\"), or to \"object\" \n\
for the default (an object representing the tail element).\n\
");

static PyObject *
RpLibraryObject_parent(RpLibraryObject *self, PyObject *args, PyObject *keywds)
{
    char* path = (char *)"";
    char* as = (char *)"object";
    RpLibrary* retlib = NULL;
    PyObject* retVal = NULL;
    int argc = 0;
    int status = 0;

    static char *kwlist[] = {
        (char *)"path",
        (char *)"as",
        NULL
    };

    if (getArgCount(args,keywds,&argc) != RP_OK) {
        // trouble ensues
        // error message was set in getArgCount()
        return NULL;
    }

    if (argc > 3) {
        PyErr_SetString(PyExc_TypeError,"parent() takes at most 2 arguments");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|ss",
            kwlist, &path, &as)) {
        /* incorrect input values */
        PyErr_SetString(PyExc_TypeError,
			"parent ([path=\'\'][, as=\'object\'])");
        return NULL;
    }
    if (self->lib) {
        retlib = self->lib->parent(std::string(path));
    } else {
        PyErr_SetString(PyExc_RuntimeError,
            "incorrectly initialized Rappture Library Object");
        return NULL;
    }

    if (retlib != NULL) {
        if ((as == NULL) || ((*as == 'o') && (strcmp("object",as) == 0))) {
            retVal = RpLibraryObject_FromLibrary(retlib);
        } else if ((*as == 'c') && (strcmp("component",as) == 0)) {
            retVal = PyString_FromString(retlib->nodeComp().c_str());
        } else if ((*as == 'i') && (strcmp("id",as) == 0)) {
            retVal = PyString_FromString(retlib->nodeId().c_str());
        } else if ((*as == 't') && (strcmp("type",as) == 0)) {
            retVal = PyString_FromString(retlib->nodeType().c_str());
        } else if ((*as == 'p') && (strcmp("path",as) == 0)) {
            retVal = PyString_FromString(retlib->nodePath().c_str());
        } else {
            PyErr_Format(PyExc_ValueError,
                "parent() \'as\' arg must be \'object\' or \'component\' or \'id\' or \'type\' or \'path\'");
        }
    }
    return (PyObject *)retVal;
}

PyDoc_STRVAR(RpLibraryObject_put_doc,
"put (path=\'\', value=\'\'[,id=None][,append=False][,type=\'string\'][,compress=False]) -> None\n\
\n\
Clients use this to set the value of a node.  If the path\n\
is not specified, it sets the value for the root node.\n\
Otherwise, it sets the value for the element specified\n\
by the path.  If the value is a string, then it is treated\n\
as the text within the tag at the tail of the path.  If it\n\
is a DOM node or a library, then it is inserted into the\n\
tree at the specified path.\n\
\n\
The optional id input has no effect and is available for\n\
backwards compatibility.\n\
\n\
If the optional append flag is specified, then the value\n\
is appended to the current value. Otherwise, the value\n\
replaces the current value.\n\
\n\
The optional type flag specifies whether the value should be\n\
treated as string data (type=\"string\"), or the name of a\n\
file (type=\"file\"). The default behavior is to treat the\n\
value as string data.\n\
\n\
If the optional compress flag is specified, then the append\n\
flag will be evaluated upon the data at path and the result\n\
will be compressed.\n\
");

static PyObject *
RpLibraryObject_put(RpLibraryObject *self, PyObject *args, PyObject *keywds)
{
    char *path = (char *)"";

    PyObject *value = NULL;
    PyObject *valueStrObj = NULL;
    char *value_char = NULL;
    Py_ssize_t value_len = 0;

    char *id = NULL;

    PyObject* append = NULL;
    int appendInt = FALSE;

    char *type = (char *)"string";

    PyObject* compress = NULL;
    int compressInt = FALSE;

    int argc = 0;
    int status = 0;

    static char *kwlist[] = {
        (char *)"path",
        (char *)"value",
        (char *)"id",
        (char *)"append",
        (char *)"type",
        (char *)"compress",
        NULL
    };

    if (self->lib == NULL) {
        PyErr_Format(PyExc_RuntimeError,
            "self uninitialized Rappture Library Object");
        return NULL;
    }

    status = getArgCount(args,keywds,&argc);
    if (status != 0) {
        // trouble ensues
        // error message was set in getArgCount()
        return NULL;
    }

    if (argc > 6) {
        // tested with PutTests.testArguments_TooManyArgs()
        PyErr_Format(PyExc_TypeError,
            "put() takes at most 6 arguments (%i given)",argc);
        return NULL;
    }

    if (argc < 2) {
        // tested with PutTests.testArguments_NotEnoughArgs()
        PyErr_Format(PyExc_TypeError,
            "put() takes at least 2 arguments (%i given)",argc);
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sO|sOsO",
            kwlist, &path, &value, &id, &append, &type, &compress)) {
        // tested with PutTests.testArguments_ArgsWrongType2()
        PyErr_Format(PyExc_TypeError, "incorrect arguments: put(path=\'\',value=\'\'[,id=None][,append=False][,type=\'string\'][,compress=False])");
        return NULL;
    }

    if (value == NULL) {
        PyErr_Format(PyExc_ValueError, "put()'s \'value\' arg is required");
        return NULL;
    }

    valueStrObj = PyObject_Str(value);

    status = boolIntFromPyObject(append,"no","append",&appendInt);
    if (status != 0) {
        // error message set inside of boolIntFromPyObject
        // tested with PutTests.testArguments_AppendValError()
        return NULL;
    }
    status = boolIntFromPyObject(compress,"no","compress",&compressInt);
    if (status != 0) {
        // error message set inside of boolIntFromPyObject
        // tested with PutTests.testArguments_CompressValError()
        return NULL;
    }

    if (RpLibraryObject_IsValid(value)) {
        // tested with PutTests.testPutObject()
        self->lib->put( std::string(path),
                        RpLibraryObject_AsLibrary(value), "", appendInt);
    }
    else if (valueStrObj != NULL) {

        if (PyString_AsStringAndSize(valueStrObj, &value_char, &value_len) == -1) {
            // user passed in an improper string
            // exception raised within fxn
            return NULL;
        }

        if (   ( type == NULL )
            || ( (*type == 's') && (strcmp("string", type) == 0) ) ) {
            if (compress == 0) {
                // tested with PutTests.testArgumentsCheckAppend()
                // tested with PutTests.testArgumentsCheckAppendYes()
                self->lib->put( std::string(path),
                                std::string(value_char), "", appendInt);
            }
            else {
                // tested with PutTests.testArgumentsCheckTypeStringCompressYes()
                self->lib->putData( std::string(path),
                                    value_char, value_len, appendInt);
            }
        }
        else if ( (*type == 'f') && (strcmp("file",type) == 0) ) {
            // tested with PutTests.testArgumentsCheckTypeFile()
            // tested with PutTests.testArgumentsCheckTypeFileCompressTrue()
            // tested with PutTests.testArgumentsCheckTypeFileCompressTrueNoDecode()
            self->lib->putFile( std::string(path),
                                std::string(PyString_AsString(valueStrObj)),
                                compressInt,appendInt);
        }
        else {
            // tested with PutTests.testArgumentsCheckTypeError()
            PyErr_Format(PyExc_ValueError,
                "\'type\' arg must be \'string\' or \'file\'");
            return NULL;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "put()'s \'value\' arg must be a string or Rappture Library Object");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(RpLibraryObject_xml_doc,
"xml () -> returns xml string\n\
\n\
Clients use this to query the XML representation for this\n\
object.\n\
");

static PyObject*
RpLibraryObject_xml(RpLibraryObject *self)
{
    PyObject *retStr = NULL;

    if (self->lib == NULL) {
        PyErr_Format(PyExc_RuntimeError,
            "self uninitialized Rappture Library Object");
    }

    retStr = PyString_FromString(self->lib->xml().c_str());

    return (PyObject *)retStr;
}

PyDoc_STRVAR(RpLibraryObject_result_doc,
"result ([status=0]) -> None, send results back to graphical user interface\n\
\n\
Use this function to report the result of a Rappture simulation.\n\
Pass in the optional exit status. Default is 0\n\
");

static PyObject*
RpLibraryObject_result(RpLibraryObject *self, PyObject *args, PyObject *keywds)
{
    int argc = 0;
    int status = 0;

    static char *kwlist[] = {
	(char *)"status", 
	NULL
    };

    if (self->lib == NULL) {
        PyErr_Format(PyExc_RuntimeError,
            "self uninitialized Rappture Library Object");
        return NULL;
    }

    status = getArgCount(args,keywds,&argc);
    if (status != 0) {
        // trouble ensues
        // error message was set in getArgCount()
        return NULL;
    }

    if (argc > 2) {
        PyErr_Format(PyExc_TypeError,
            "result() takes at most 1 argument (%i given)",argc);
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|i", kwlist, &status)) {
        // tested with ResultTests.testArguments_InvalidStatusArg()
        PyErr_Format(PyExc_TypeError, "an integer is required");
        return NULL;
    }

    self->lib->put("tool.version.rappture.language","python");
    self->lib->result(status);

    Py_RETURN_NONE;
}

static PyMethodDef RpLibraryObject_methods[] = {

    {"copy", (PyCFunction)RpLibraryObject_copy, METH_VARARGS|METH_KEYWORDS,
        RpLibraryObject_copy_doc},

/*
    {"children", (PyCFunction)RpLibraryObject_children, METH_VARARGS|METH_KEYWORDS,
        RpLibraryObject_children_doc},
*/

    {"element", (PyCFunction)RpLibraryObject_element, METH_VARARGS|METH_KEYWORDS,
        RpLibraryObject_element_doc},

    {"get", (PyCFunction)RpLibraryObject_get, METH_VARARGS|METH_KEYWORDS,
        RpLibraryObject_get_doc},

/*
    {"parent", (PyCFunction)RpLibraryObject_parent, METH_VARARGS|METH_KEYWORDS,
        RpLibraryObject_put_doc},
*/

    {"put", (PyCFunction)RpLibraryObject_put, METH_VARARGS|METH_KEYWORDS,
        RpLibraryObject_put_doc},

/*
    {"remove", (PyCFunction)RpLibraryObject_remove, METH_VARARGS|METH_KEYWORDS,
        RpLibraryObject_remove_doc},
*/

    {"xml", (PyCFunction)RpLibraryObject_xml, METH_NOARGS,
        RpLibraryObject_xml_doc},

    {"result", (PyCFunction)RpLibraryObject_result, METH_VARARGS|METH_KEYWORDS,
        RpLibraryObject_result_doc},

    {NULL,        NULL}        /* sentinel */
};

static PyTypeObject RpLibraryObjectType = {
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyObject_HEAD_INIT(NULL)
    0,                                    /*ob_size*/
    "Rappture.library",                   /*tp_name*/
    sizeof(RpLibraryObject),              /*tp_basicsize*/
    0,                                    /*tp_itemsize*/
    /* methods */
    (destructor)RpLibraryObject_dealloc,  /*tp_dealloc*/
    0,                                    /*tp_print*/
    0,                                    /*tp_getattr*/
    0,                                    /*tp_setattr*/
    0,                                    /*tp_compare*/
    0,                                    /*tp_repr*/
    0,                                    /*tp_as_number*/
    0,                                    /*tp_as_sequence*/
    0,                                    /*tp_as_mapping*/
    0,                                    /*tp_hash*/
    0,                                    /*tp_call*/
    0,                                    /*tp_str*/
    0,                                    /*tp_getattro*/
    0,                                    /*tp_setattro*/
    0,                                    /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /*tp_flags*/
    "Rappture.library Object",            /*tp_doc*/
    0,                                    /*tp_traverse*/
    0,                                    /*tp_clear*/
    0,                                    /*tp_richcompare*/
    0,                                    /*tp_weaklistoffset*/
    0,                                    /*tp_iter*/
    0,                                    /*tp_iternext*/
    RpLibraryObject_methods,              /* tp_methods */
    0,                                    /*tp_members*/
    0,                                    /*tp_getset*/
    0,                                    /*tp_base*/
    0,                                    /*tp_dict*/
    0,                                    /*tp_descr_get*/
    0,                                    /*tp_descr_set*/
    0,                                    /*tp_dictoffset*/
    (initproc)RpLibraryObject_init,       /*tp_init*/
    0,                                    /*tp_alloc*/
    RpLibraryObject_new,                  /*tp_new*/
    0,                                    /*tp_new*/
};

/*
 * used to create a RpLibrary object from a PyObject
 */
static RpLibrary *
RpLibraryObject_AsLibrary(PyObject *lib)
{
    RpLibrary *retval = NULL;

    if (lib != NULL) {
        if (RpLibraryObject_IsValid(lib)) {
            retval = ((RpLibraryObject *)lib)->lib;
        }
    }
    return retval;
}

/*
 * used to create a PyObject from a RpLibrary object
 */
PyObject *
RpLibraryObject_FromLibrary(RpLibrary *lib)
{
    RpLibraryObject *self = NULL;

    if (lib != NULL) {
        self = PyObject_New(RpLibraryObject, &RpLibraryObjectType);
        if (self != NULL) {
            self->lib = lib;
        }
        else {
            // raise error
            PyErr_SetString(PyExc_RuntimeError,
                            "trouble creating new RpLibraryObject");
        }
    }

    return (PyObject *)self;
}

/*
 *  int boolAsInt (inVal, outVal)
 *
 *  represent a boolean string as an integer.
 *
 *  outVal set to 1 if the boolean value inVal could
 *  be associated with any of the following strings:
 *  "yes", "on", "true", "1".
 *
 *  outVal set to 0 if the boolean value inVal could
 *  be associated with any of the following strings:
 *  "no", "off", "false", "0".
 *
 *  returns a status integer to tell if the operation
 *  was successful (0) of if there was an error (!0)
 *
 *  note: string comparisons are case insensitive.
 */

static int
boolAsInt(const char *string, int *resultPtr)
{
    int len, i;
    char c;

    if ((string == NULL) || (resultPtr == NULL) ) {
        PyErr_Format(PyExc_TypeError,
		     "incorrect use of boolAsInt(inVal,outVal)");
        return RP_ERROR;
    }
    c = tolower(string[0]);
    if ((c == 'y') && (strcasecmp(string, "yes") == 0)) {
	*resultPtr = TRUE;
    } else if ((c == 'n') && (strcasecmp(string, "no") == 0)) {
	*resultPtr = FALSE;
    } else if ((c == 'o') && (strcasecmp(string, "on") == 0)) {
	*resultPtr = TRUE;
    } else if ((c == 'o') && (strcasecmp(string, "off") == 0)) {
	*resultPtr = FALSE;
    } else if ((c == 't') && (strcasecmp(string, "true") == 0)) {
	*resultPtr = TRUE;
    } else if ((c == 'f') && (strcasecmp(string, "false") == 0)) {
	*resultPtr = FALSE;
    } else if ((c == '1') && (strcasecmp(string, "1") == 0)) {
	*resultPtr = TRUE;
    } else if ((c == '0') && (strcasecmp(string, "0") == 0)) {
	*resultPtr = FALSE;
    } else {
        PyErr_Format(PyExc_ValueError,
            "unrecognized input: %s: should be one of: \'yes\',\'true\',\'on\',\'1\',1,True,\'no\',\'false\',\'off\',\'0\',0,False", string);
        return RP_ERROR;
    }
    return RP_OK;
}

static int
boolIntFromPyObject (PyObject *objPtr, const char *defValue, 
		     const char *argName, int *resultPtr)
{
    int value;

    if ((defValue == NULL) || (argName == NULL) || (resultPtr == NULL)) {
        // incorrect use of function
        PyErr_Format(PyExc_ValueError,
            "boolIntFromPyObject(): defValue or argName or resultPtr is NULL");
        return RP_ERROR;
    }
    if (objPtr == NULL) {
        return boolAsInt(defValue, resultPtr);
    }
    if (PyBool_Check(objPtr)) {
	value = PyObject_IsTrue(objPtr);
	if (value < 0) {
             PyErr_Format(PyExc_ValueError, 
		"boolIntFromPyObject(): bad boolean object");
	    return RP_ERROR;
	}
    } else if (PyLong_Check(objPtr)) {
	long l;

	l = PyLong_AsLong(objPtr);
	value = (l == 0) ? FALSE : TRUE;
    } else if (PyFloat_Check(objPtr)) {
	double d;

	d = PyFloat_AS_DOUBLE(objPtr);
	value = (d == 0.0) ? FALSE : TRUE;
    } else if (PyString_Check(objPtr)) {
	const char *string;
	
        string = PyString_AsString(objPtr);
        if (string == NULL) {
            PyErr_Format(PyExc_TypeError,
                "bad value: %s: cannot convert to string",argName);
            return RP_ERROR;
        }
        return boolAsInt(string, resultPtr);
    }
    *resultPtr = value;
    return RP_OK;
}

static int 
getArgCount (PyObject *args, PyObject *keywds, int *argc)
{
    int args_cnt = 0;
    int keywds_cnt = 0;

    if (argc == NULL) {
        // incorrect use of function
        // argc cannot be null
        PyErr_Format(PyExc_ValueError,"getArgCount(): argc is NULL");
        return RP_ERROR;
    }
    if (args != NULL) {
        if (!PyTuple_Check(args)) {
            PyErr_Format(PyExc_TypeError,
                "getArgCount(): \'args\' should be a PyTuple");
            return RP_ERROR;
        }
        args_cnt = PyTuple_Size(args);
    }
    if (keywds != NULL) {
        if (!PyDict_Check(keywds)) {
            PyErr_Format(PyExc_TypeError,
                "getArgCount(): \'keywds\' should be a PyDict");
            return RP_ERROR;
        }
        keywds_cnt = PyDict_Size(keywds);
    }
    *argc = args_cnt + keywds_cnt;
    return RP_OK;
}

/* ---------- */

/* List of functions defined in the module */

static PyMethodDef Library_Methods[] = {
    {NULL,        NULL}        /* sentinel */
};

PyDoc_STRVAR(module_doc, "Rappture Library Module for Python.");

/* Initialization function for the module */

PyMODINIT_FUNC
initlibrary(void)
{
    PyObject *m;

    /* Finalize the type object including setting type of the new type
     * object; doing it here is required for portability to Windows
     * without requiring C++. */
    if (PyType_Ready(&RpLibraryObjectType) < 0)
        return;

    /* Create the module and add the functions */
    m = Py_InitModule3("library", Library_Methods, module_doc);

    /* Add some symbolic constants to the module */
    if (ErrorObject == NULL) {
        ErrorObject = PyErr_NewException((char *)"RpLibrary.error", NULL, NULL);
        if (ErrorObject == NULL)
            return;
    }

    Py_INCREF(&RpLibraryObjectType);
    PyModule_AddObject(m, "library", (PyObject*) &RpLibraryObjectType);

    Py_XINCREF(ErrorObject);
    PyModule_AddObject(m, "error", ErrorObject);

}
