
/*
 * Rappture Utils Python Interface
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
#include <rappture.h>

static PyObject *ErrorObject;

PyDoc_STRVAR(RpUtils_progress_doc,
"progress (percent,message)\n\
\n\
Send progress messages to Rappture for updating the\n\
graphical user interface's progress bar.");

static PyObject*
RpUtils_progress(PyObject *self, PyObject *args, PyObject *keywds)
{
    double pct;
    char *msg;
    static char *kwlist[] = { 
	(char *)"percent", (char *)"message", NULL 
    };
    if (PyTuple_Size(args) != 2) {
        PyErr_Format(PyExc_ValueError,
		"progress() takes exactly 2 arguments, got %ld", 
		PyTuple_Size(args));
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ds", kwlist, 
		&pct, &msg)) {
        return NULL;
    }
    if (Rappture::Utils::progress((int)pct, msg) != 0) {
        PyErr_SetString(PyExc_RuntimeError, "Error while writing to stdout");
	return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

/* ---------- */


/* List of functions defined in the module */

static PyMethodDef RpUtils_Methods[] = {

    {"progress", (PyCFunction)RpUtils_progress, METH_VARARGS|METH_KEYWORDS,
        RpUtils_progress_doc},

    {NULL,        NULL}        /* sentinel */
};

PyDoc_STRVAR(module_doc, "Rappture Utils Module for Python.");

/* Initialization function for the module */

PyMODINIT_FUNC
initUtils(void)
{
    PyObject *m;

    /* Create the module and add the functions */
    m = Py_InitModule3("Utils", RpUtils_Methods, module_doc);

    if (ErrorObject == NULL) {
        ErrorObject = PyErr_NewException((char *)"Rappture.Utils.error", 
		NULL, NULL);
        if (ErrorObject == NULL)
            return;
    }
    Py_INCREF(ErrorObject);
    PyModule_AddObject(m, "error", ErrorObject);

    return;
}
