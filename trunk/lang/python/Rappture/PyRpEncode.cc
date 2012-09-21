
/*
 * Rappture Encode Python Interface
 *
 * ======================================================================
 *  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <Python.h>
#include <RpEncode.h>

static PyObject *ErrorObject;

PyDoc_STRVAR(RpEncode_isbinary_doc,
"isbinary(data)\n\
\n\
Check to see if there are binary characters\n\
in the provided data.");

static PyObject*
RpEncode_isbinary(PyObject *self, PyObject *args, PyObject *keywds)
{
    const char* data = NULL;
    int dlen = 0;
    PyObject* rv = NULL;

    static char *kwlist[] = {
        (char *)"data", 
        NULL
    };

    if (PyTuple_Size(args) != 1) {
        PyErr_SetString(PyExc_TypeError,"isbinary() takes exactly 1 argument");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#",
            kwlist, &data, &dlen)) {
        PyErr_SetString(PyExc_TypeError,"isbinary() takes exactly 1 argument");
        return NULL;
    }

    rv = PyFloat_FromDouble(Rappture::encoding::isBinary(data,dlen));
    return rv;
}

PyDoc_STRVAR(RpEncode_encode_doc,
"encode(data,flags)\n\
\n\
Encode the provided data with gzip compression and base64 \n\
according to the provided flags");

static PyObject*
RpEncode_encode(PyObject *self, PyObject *args, PyObject *keywds)
{
    const char* data = NULL;
    int dlen = 0;
    int flags = 0;
    PyObject* rv = NULL;
    Rappture::Outcome err;

    static char *kwlist[] = {
        (char *)"data",
        (char *)"flags",
        NULL
    };

    if (PyTuple_Size(args) != 2) {
        PyErr_SetString(PyExc_TypeError,"encode() takes exactly 2 arguments");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#i",
            kwlist, &data, &dlen, &flags)) {
        PyErr_SetString(PyExc_TypeError,"encode() takes exactly 2 arguments");
        return NULL;
    }

    Rappture::Buffer buf(data, dlen);
    if (!Rappture::encoding::encode(err, buf, flags)) {
        std::string outStr;

        outStr = err.remark();
        outStr += "\n";
        outStr += err.context();
        PyErr_SetString(PyExc_RuntimeError, outStr.c_str());
        buf.clear();
        return NULL;
    }
    rv = PyString_FromStringAndSize(buf.bytes(),buf.size());
    return rv;
}

PyDoc_STRVAR(RpEncode_decode_doc,
"decode(data,flags)\n\
\n\
Decode the provided data with gzip compression and base64 \n\
according to the provided flags");

static PyObject*
RpEncode_decode(PyObject *self, PyObject *args, PyObject *keywds)
{
    const char* data = NULL;
    int dlen = 0;
    int flags = 0;
    PyObject* rv = NULL;
    Rappture::Outcome err;

    static char *kwlist[] = {
        (char *)"data",
        (char *)"flags",
        NULL
    };

    if (PyTuple_Size(args) != 2) {
        PyErr_SetString(PyExc_TypeError,"decode() takes exactly 2 arguments");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#i",
            kwlist, &data, &dlen, &flags)) {
        PyErr_SetString(PyExc_TypeError,"decode() takes exactly 2 arguments");
        return NULL;
    }

    Rappture::Buffer buf(data, dlen);
    if (!Rappture::encoding::decode(err, buf, flags)) {
        std::string outStr;

        outStr = err.remark();
        outStr += "\n";
        outStr += err.context();
        PyErr_SetString(PyExc_RuntimeError,outStr.c_str());
        return NULL;
    }
    rv = PyString_FromStringAndSize(buf.bytes(),buf.size());
    return rv;
}

/* ---------- */


/* List of functions defined in the module */

static PyMethodDef RpEncode_Methods[] = {

    {"isbinary", (PyCFunction)RpEncode_isbinary, METH_VARARGS|METH_KEYWORDS,
        RpEncode_isbinary_doc},
    {"encode", (PyCFunction)RpEncode_encode, METH_VARARGS|METH_KEYWORDS,
        RpEncode_encode_doc},
    {"decode", (PyCFunction)RpEncode_decode, METH_VARARGS|METH_KEYWORDS,
        RpEncode_decode_doc},

    {NULL,        NULL}        /* sentinel */
};

PyDoc_STRVAR(module_doc, "Rappture Encode Module for Python.");

/* Initialization function for the module */

PyMODINIT_FUNC
initencoding(void)
{
    PyObject *m;

    /* Create the module and add the functions */
    m = Py_InitModule3("encoding", RpEncode_Methods, module_doc);

    if (ErrorObject == NULL) {
        ErrorObject = PyErr_NewException((char *)"Rappture.encoding.error", 
                NULL, NULL);
        if (ErrorObject == NULL) {
            return;
        }
    }
    Py_INCREF(ErrorObject);
    PyModule_AddObject(m, "error", ErrorObject);

    return;
}
