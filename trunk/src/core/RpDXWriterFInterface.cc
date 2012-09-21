/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture DXWriter Bindings Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <cstring>
#include <fstream>
#include "RpDXWriterFInterface.h"
#include "RpFortranCommon.h"
#include "RpBindingsDict.h"
#include "RpDXWriterFStubs.c"

#define RP_OK 0
#define RP_ERROR 1

/**********************************************************************/
// FUNCTION: rp_dxwriter()
/// Open a new Rappture::DXWriter object and return its handle.
/**
 */

int rp_dxwriter() {

    Rappture::DXWriter* dxwriter = NULL;
    int handle = -1;

    // create a Rappture::DXWriter object and store in dictionary
    dxwriter = new Rappture::DXWriter();

    if (dxwriter) {
        handle = storeObject_Void(dxwriter);
    }

    return handle;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_origin()
/// change the origin of the dx object.
/**
 */
int rp_dxwriter_origin(int* handle,     /* integer handle of dxwriter */
                        double *o         /* array of origin values per axis */
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            dxwriter->origin(o);
            retVal = RP_OK;
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_delta()
/// change the delta of the dx object.
/**
 */
int rp_dxwriter_delta(int* handle,     /* integer handle of dxwriter */
                        double *d         /* array of delta values per axis */
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            dxwriter->delta(d);
            retVal = RP_OK;
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_counts()
/// change the counts of the position grid in the dx object.
/**
 */
int rp_dxwriter_counts(int* handle,     /* integer handle of dxwriter */
                        size_t *p         /* array of counts per axis */
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            dxwriter->counts(p);
            retVal = RP_OK;
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_append()
/// append data to the dxwriter object.
/**
 */
int rp_dxwriter_append(int* handle,     /* integer handle of dxwriter */
                        double *value         /* single data value*/
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            dxwriter->append(value);
            retVal = RP_OK;
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_data()
/// append data to the dxwriter object.
/**
 */
int rp_dxwriter_data(int* handle,     /* integer handle of dxwriter */
                        double *value,        /* array of data values */
                        size_t *nmemb        /* number of members in array value*/
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            dxwriter->data(value,*nmemb);
            retVal = RP_OK;
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_pos()
/// append position to the dxwriter object.
/**
 */
int rp_dxwriter_pos(int* handle,     /* integer handle of dxwriter */
                        double *position,     /* array of positions */
                        size_t *nmemb        /* number of members in array value*/
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            dxwriter->pos(position,*nmemb);
            retVal = RP_OK;
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_write()
/// write the dx file to disk.
/**
 */
int rp_dxwriter_write(int* handle,     /* integer handle of dxwriter */
                        char *fname,     /* filename to hold dx file text */
                        size_t str_len /* size of str */
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            dxwriter->write((const char*)fname);
            retVal = RP_OK;
            /*
            if (dxwriter->size() > (str_len)) {
                dxwriter->write((const char*)fname);
                retVal = RP_OK;
            } else {
                // string too small
                retVal = RP_ERROR;
            }
            */
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_size()
/// return the size of the dx file text
/**
 */
int rp_dxwriter_size(int* handle,    /* integer handle of dxwriter */
                        size_t* s    /* return value */
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            *s = dxwriter->size();
            retVal = RP_OK;
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_rank()
/// change the rank of the dx file text
/**
 */
int rp_dxwriter_rank(int* handle,    /* integer handle of dxwriter */
                        size_t* rank /* return value */
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            *rank = dxwriter->rank();
            retVal = RP_OK;
        }
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: rp_dxwriter_shape()
/// change the shape of the dx file text
/**
 */
int rp_dxwriter_shape(int* handle,    /* integer handle of dxwriter */
                        size_t* shape      /* return value */
                        )
{
    size_t retVal = RP_ERROR;
    Rappture::DXWriter* dxwriter = NULL;

    if ((handle) && (*handle != 0)) {
        dxwriter = (Rappture::DXWriter*) getObject_Void(*handle);
        if (dxwriter) {
            *shape = dxwriter->shape();
            retVal = RP_OK;
        }
    }

    return retVal;
}
