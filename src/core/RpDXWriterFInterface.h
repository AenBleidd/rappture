/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture DXWriter Bindings Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RAPPTURE_DXWRITER_F_H
#define _RAPPTURE_DXWRITER_F_H

#ifdef __cplusplus
    #include "RpDXWriter.h"
    #include "RpDXWriterFStubs.h"

    extern "C" {
#endif // ifdef __cplusplus

int rp_dxwriter();

int rp_dxwriter_origin(int *handle, double *o);

int rp_dxwriter_delta(int *handle, double *d);

int rp_dxwriter_counts(int *handle, size_t *p);

int rp_dxwriter_append(int *handle, double *value);

int rp_dxwriter_data(int *handle, double *value, size_t *nmemb);

int rp_dxwriter_pos(int *handle, double *position, size_t *nmemb);

int rp_dxwriter_write(int *handle, char *fname, size_t str_len);

int rp_dxwriter_size(int *handle, size_t *sz);

int rp_dxwriter_rank(int *handle, size_t *rank);

int rp_dxwriter_shape(int *handle, size_t *shape);

/**********************************************************/

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // ifndef _RAPPTURE_DXWRITER_F_H
