/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <Python.h>
#include <RpUnits.h>

static PyObject *ErrorObject;

typedef struct {
    PyObject_HEAD
    const RpUnits* rp_unit;
} RpUnitsObject;

static void
RpUnitsObject_dealloc(RpUnitsObject *self)
{
    if (self) {
        if (self->rp_unit){
            // cant call this right now because destructor is private
            // delete(self->rp_unit);
        }
        PyObject_Del(self);
    }
}

static PyObject*
RpUnitsObject_getUnits(RpUnitsObject *self)
{
    PyObject* rv = NULL;

    if (self->rp_unit){ 
        rv = PyString_FromString(self->rp_unit->getUnits().c_str());
    }

    return rv;
}

static PyObject*
RpUnitsObject_getUnitsName(RpUnitsObject *self)
{
    PyObject* rv = NULL;

    if (self->rp_unit){
        rv = PyString_FromString(self->rp_unit->getUnitsName().c_str());
    }

    return rv;
}

static PyObject*
RpUnitsObject_getExponent(RpUnitsObject *self)
{
    PyObject* rv = NULL;

    if (self->rp_unit){
        rv = PyFloat_FromDouble(self->rp_unit->getExponent());
    }

    return rv;
}

static PyObject * RpUnitsObject_convert(RpUnitsObject *self, PyObject *args);

static PyObject *
RpUnitsObject_makeBasis(RpUnitsObject *self, PyObject *args)
{
    PyObject* rv = NULL;
    double inVal = 0;
    double outVal = 0;
    int result = 0;

    if (PyTuple_Size(args) > 0) {
        PyArg_ParseTuple(args, "d", &inVal);
    }
    else {
        PyErr_SetString(PyExc_AttributeError, "incorrect input arguments");
        return NULL;
    }

    if (self->rp_unit){
        outVal = self->rp_unit->makeBasis(inVal, &result);
        if (result) {
            rv = PyFloat_FromDouble(outVal);
        }
        else {
            PyErr_SetString(PyExc_StandardError, "could not convert to basis");
        }
    }
    else {
        PyErr_SetString(PyExc_AttributeError, "rp_unit is NULL");
    }

    return rv;
}

static PyMethodDef RpUnitsObject_methods[] = {
    {"getUnits", (PyCFunction)RpUnitsObject_getUnits, METH_NOARGS,
     "Return the base name of the RpUnitsObject" },
    {"getUnitsName", (PyCFunction)RpUnitsObject_getUnitsName, METH_NOARGS,
     "Return the whole (base and exponent) name of the RpUnitsObject" },
    {"getExponent", (PyCFunction)RpUnitsObject_getExponent, METH_NOARGS,
     "Return the exponent of the RpUnitsObject" },
    {"convert", (PyCFunction)RpUnitsObject_convert, METH_VARARGS,
     "convert a value from one RpUnits Object to another" },
    {"makeBasis", (PyCFunction)RpUnitsObject_makeBasis, METH_VARARGS,
     "return the basis value of the value provided" },

    {NULL}  /* Sentinel */
};

static PyTypeObject RpUnitsObjectType = {
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyObject_HEAD_INIT(NULL)
    0,                                    /*ob_size*/
    "RpUnits.RpUnitsObject",              /*tp_name*/
    sizeof(RpUnitsObject),                /*tp_basicsize*/
    0,                                    /*tp_itemsize*/
    /* methods */
    (destructor)RpUnitsObject_dealloc,    /*tp_dealloc*/
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
    "Rappture.Units Object",              /*tp_doc*/
    0,                                    /*tp_traverse*/
    0,                                    /*tp_clear*/
    0,                                    /*tp_richcompare*/
    0,                                    /*tp_weaklistoffset*/
    0,                                    /*tp_iter*/
    0,                                    /*tp_iternext*/
    RpUnitsObject_methods,                /* tp_methods */
    0,                                    /*tp_members*/
    0,                                    /*tp_getset*/
    0,                                    /*tp_base*/
    0,                                    /*tp_dict*/
    0,                                    /*tp_descr_get*/
    0,                                    /*tp_descr_set*/
    0,                                    /*tp_dictoffset*/
    0,                                    /*tp_init*/
    0,                                    /*tp_alloc*/
    // RpUnitsObject_new,                 /*tp_new*/
    0,                                    /*tp_new*/
    0,                                    /*tp_free*/
    0,                                    /*tp_is_gc*/
};

static RpUnitsObject*
newRpUnitsObject(PyObject *arg)
{
    RpUnitsObject* self;
    self = PyObject_New(RpUnitsObject, &RpUnitsObjectType);
    if (self == NULL)
        return NULL;
    self->rp_unit = NULL;
    return self;
}

static PyObject *
RpUnitsObject_convert(RpUnitsObject *self, PyObject *args)
{
    PyObject* rv = NULL;
    // PyObject* toUnits = NULL;
    RpUnitsObject* toUnits = NULL;
    PyObject* inVal = NULL;
    PyObject* argList = NULL;
    PyObject* outVal = NULL;
    int result = 0;
    int argTupleSize = PyTuple_Size(args);

    if (argTupleSize > 0) {
        PyArg_ParseTuple(args, "O!O", &RpUnitsObjectType, &toUnits, &inVal);
        argList = PyTuple_New(1);
        PyTuple_SetItem(argList, 0, inVal);
        /*
         * need to make it so user can give any number of variables in arglist
         * because the new argList is sent to the python conversion fxn where it
         * will be parsed in python when c++ calls the conv fxn.
        PyArg_ParseTuple(args, "O!", &RpUnitsObjectType, &toUnits);
        if (argTupleSize > 1) {
            argTupleSize--;
            inVal = PyTuple_New(argTupleSize);
            while (argTupleSize)
            inVal = PyTuple_GetSlice(args,0,argTupleSize);
        }
        else {
            // convertion function needs no arguments???
            inVal = PyTuple_New(0);
        }
        */
    }
    else {
        PyErr_SetString(PyExc_AttributeError, "Not enough arguments");
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (self->rp_unit){ 
        outVal = (PyObject*) self->rp_unit->convert(toUnits->rp_unit,
                                                    // (void*)&inVal,
                                                    (void*)argList,
                                                    &result );
        if (result) {
            rv = outVal;
        }
    }

    return rv;
}

/* --------------------------------------------------------------------- */

PyDoc_STRVAR(RpUnits_define_doc, 
"define(name, basis) -> RpUnitsObject \n\
\n\
define RpUnits Object where 'name' is the string name of the units \n\
and 'basis' is an RpUnitsObject pointer for the basis of the unit \n\
being created");

static PyObject *
RpUnits_define(PyObject *self, PyObject *args)
{
    RpUnitsObject* newRpUnit;
    RpUnitsObject* basis = NULL;
    char* unitsName;

    if (PyTuple_Size(args) > 0) {
        PyArg_ParseTuple(args, "s|O!", &unitsName, &RpUnitsObjectType,&basis);
    }
    else {
        PyErr_SetString(PyExc_AttributeError, "Not enough arguments");
        Py_INCREF(Py_None);
        return Py_None;
    }

    newRpUnit = newRpUnitsObject(args);

    if (newRpUnit == NULL)
        return NULL;


    if (basis && basis->rp_unit) {
        newRpUnit->rp_unit = RpUnits::define(unitsName, basis->rp_unit);
    }
    else {
        newRpUnit->rp_unit = RpUnits::define(unitsName, NULL);
    }

    if (newRpUnit->rp_unit == NULL) {
        // rp_unit was not allocated. 
        RpUnitsObject_dealloc(newRpUnit);
        PyErr_SetString(PyExc_AttributeError, "allocating rp_unit failed");
        return NULL;
    }

    return (PyObject *)newRpUnit;
}

void* PyCallback (void* fxnPtr, void* args)
{
    PyObject* retVal = NULL;

    if ((PyObject*)fxnPtr != NULL) {
        retVal = PyObject_CallObject((PyObject*)fxnPtr,(PyObject*)args);
    }

    return (void*) retVal;

}

PyDoc_STRVAR(RpUnits_defineConv_doc, 
"defineConv(fromUnits,toUnits, forwConvFxn, backConvFxn) -> RpUnitsObject\n\
\n\
define RpUnits Object where 'name' is the string name of the units \n\
and 'basis' is an RpUnitsObject pointer for the basis of the unit \n\
being created");

static PyObject *
RpUnits_defineConv(PyObject *self, PyObject *args)
{
    RpUnitsObject* fromUnit = NULL;
    RpUnitsObject* toUnit = NULL;
    PyObject* forwConvFxnStr = NULL;
    PyObject* backConvFxnStr = NULL;

    RpUnitsObject* newRpUnit = NULL;
    const RpUnits* newConv = NULL;

    if (PyTuple_Size(args) > 0) {
        PyArg_ParseTuple(args, "O!O!O!O!",&RpUnitsObjectType, &fromUnit,
                                          &RpUnitsObjectType, &toUnit,
                                          &PyFunction_Type, &forwConvFxnStr,
                                          &PyFunction_Type, &backConvFxnStr);
    }
    else {
        PyErr_SetString(PyExc_AttributeError, "incorrect input arguments");
        return NULL;
    }

    // check to make sure fromUnit and toUnit are populated
    if ( (fromUnit == NULL) ||
         (toUnit == NULL) ) {
        PyErr_SetString(PyExc_AttributeError,
                "could not retrieve fromUnit or toUnit from argument list");
    }

    // check to make sure forwConvFxnStr and backConvFxnStr are populated
    if ( (forwConvFxnStr == NULL) || (backConvFxnStr == NULL) ) {
        PyErr_SetString(PyExc_AttributeError, 
                "could not retrieve conversion function argument");
        return NULL;
    }

    // make sure we get callable functions and non-null RpUnit Objects
    if ( PyCallable_Check(forwConvFxnStr) &&
         PyCallable_Check(backConvFxnStr) &&
         fromUnit->rp_unit &&
         toUnit->rp_unit) {

        Py_INCREF(forwConvFxnStr);
        Py_INCREF(backConvFxnStr);
        newConv = RpUnits::define(  fromUnit->rp_unit,
                                    toUnit->rp_unit,
                                    PyCallback,
                                    (void*)forwConvFxnStr,
                                    PyCallback,
                                    (void*)backConvFxnStr );
    }
    else {
        PyErr_SetString(PyExc_AttributeError, 
                "could not retrieve conversion function argument");
        return NULL;
    }

    if (newConv) {
        newRpUnit = newRpUnitsObject(args);

        if (newRpUnit) {
            newRpUnit->rp_unit = newConv;
        }
    }

    return (PyObject *)newRpUnit;
}

PyDoc_STRVAR(RpUnits_find_doc,
"find(name) -> RpUnitsObject \n\
\n\
search the dictionary of created RpUnits for a unit matching \n\
the string 'name'");

static PyObject *
RpUnits_find(PyObject *self, PyObject *args)
{
    char* searchUnits = NULL;
    const RpUnits* foundUnits = NULL;
    RpUnitsObject* returnUnits = NULL;

    if (PyTuple_Size(args) > 0) {
        PyArg_ParseTuple(args, "s", &searchUnits);
    }
    else {
        PyErr_SetString(PyExc_AttributeError, "incorrect input arguments");
        return NULL;
    }

    foundUnits = RpUnits::find(searchUnits);

    if (foundUnits) {
        returnUnits = newRpUnitsObject(args);

        if (returnUnits == NULL)
            return NULL;

        returnUnits->rp_unit = foundUnits;
    }

    return (PyObject*) returnUnits;

}

PyDoc_STRVAR(RpUnits_convert_doc,
"convert (fromVal, to, units) -> PyString \n\
\n\
Convert the value 'fromVal', to the units listed in 'to', \n\
and return the value as a string. The string 'fromVal' must have a \n\
numeric value with units attached to the end. If 'units' is set to \n\
'off' then the returned string will not have units. The default \n\
behavior is to show units.");

static PyObject*
RpUnits_convert(PyObject *self, PyObject *args, PyObject *keywds)
{
    // RpUnitsObject* units = NULL;
    char* fromVal = NULL;
    char* to = NULL;
    char* units = NULL;
    // char* tmpRetStr = NULL;
    std::string fromVal_S = "";
    std::string to_S = "";
    std::string tmpUnits_S = "";
    int unitsVal = 1;
    int result = 0;
    std::string retStr = "";
    PyObject* retVal = NULL;
    PyObject* tmpPyStr = NULL;

    static char *kwlist[] = {
        (char *)"fromVal",
        (char *)"to",
        (char *)"units",
        NULL
    };

    if (PyTuple_Size(args) > 0) {
        // PyArg_ParseTuple(args, "ss|s", &fromVal, &to, &units);
        if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|s", 
                    kwlist, &fromVal, &to, &units)) {
            return NULL;
        }
    }
    else {
        return NULL;
    }

    fromVal_S = std::string(fromVal);
    to_S = std::string(to);
    if (units) {
        tmpUnits_S = std::string(units);
        if(tmpUnits_S.compare("off") == 0) {
            unitsVal = 0;
        }
        else {
            unitsVal = 1;
        }
    }

    retStr = RpUnits::convert(fromVal_S,to_S,unitsVal,&result);

    if ( (!retStr.empty()) && (result == 0) ) {
        if (unitsVal) {
            retVal = PyString_FromString(retStr.c_str());
        }
        else {
            // convert to a double and return that if
            // the units were turned off
            tmpPyStr = PyString_FromString(retStr.c_str());
            if (tmpPyStr) {
                    Py_INCREF(tmpPyStr);
                    retVal = PyFloat_FromString(tmpPyStr,NULL);
                    Py_DECREF(tmpPyStr);
            }
        }
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return retVal;
}

/* ---------- */


/* List of functions defined in the module */

static PyMethodDef RpUnits_Methods[] = {

    {"define", RpUnits_define, METH_VARARGS,
        RpUnits_define_doc},

    {"defineConv", RpUnits_defineConv, METH_VARARGS,
        RpUnits_defineConv_doc},

    {"find", RpUnits_find, METH_VARARGS,
        RpUnits_find_doc},

    {"convert", (PyCFunction)RpUnits_convert, METH_VARARGS|METH_KEYWORDS,
        RpUnits_convert_doc},

    {NULL,        NULL}        /* sentinel */
};

PyDoc_STRVAR(module_doc, "Rappture Units Module for Python.");

/* Initialization function for the module */

PyMODINIT_FUNC
initUnits(void)
{
    PyObject *m;

    /* Finalize the type object including setting type of the new type
     * object; doing it here is required for portability to Windows
     * without requiring C++. */
    if (PyType_Ready(&RpUnitsObjectType) < 0)
        return;

    /* Create the module and add the functions */
    m = Py_InitModule3("Units", RpUnits_Methods, module_doc);

    /* Add some symbolic constants to the module */
    if (ErrorObject == NULL) {
        ErrorObject = PyErr_NewException((char *)"RpUnits.error", NULL, NULL);
        if (ErrorObject == NULL)
            return;
    }
    Py_INCREF(ErrorObject);
    PyModule_AddObject(m, "error", ErrorObject);

    // add some standard units definitions and conversions.
    // RpUnits::addPresets("all");
}
