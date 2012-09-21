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

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

int rp_dxwriter_();
int rp_dxwriter__();
int RP_DXWRITER();

int rp_dxwriter_origin_(int *handle, double *o);
int rp_dxwriter_origin__(int *handle, double *o);
int RP_DXWRITER_ORIGIN(int *handle, double *o);

int rp_dxwriter_delta_(int *handle, double *d);
int rp_dxwriter_delta__(int *handle, double *d);
int RP_DXWRITER_DELTA(int *handle, double *d);

int rp_dxwriter_counts_(int *handle, size_t *p);
int rp_dxwriter_counts__(int *handle, size_t *p);
int RP_DXWRITER_COUNTS(int *handle, size_t *p);

int rp_dxwriter_append_(int *handle, double *value);
int rp_dxwriter_append__(int *handle, double *value);
int RP_DXWRITER_APPEND(int *handle, double *value);

int rp_dxwriter_data_(int *handle, double *value, size_t *nmemb);
int rp_dxwriter_data__(int *handle, double *value, size_t *nmemb);
int RP_DXWRITER_DATA(int *handle, double *value, size_t *nmemb);

int rp_dxwriter_pos_(int *handle, double *position, size_t *nmemb);
int rp_dxwriter_pos__(int *handle, double *position, size_t *nmemb);
int RP_DXWRITER_POS(int *handle, double *position, size_t *nmemb);

int rp_dxwriter_write_(int *handle, char *fname, size_t str_len);
int rp_dxwriter_write__(int *handle, char *fname, size_t str_len);
int RP_DXWRITER_WRITE(int *handle, char *fname, size_t str_len);

int rp_dxwriter_size_(int *handle, size_t *sz);
int rp_dxwriter_size__(int *handle, size_t *sz);
int RP_DXWRITER_SIZE(int *handle, size_t *sz);

int rp_dxwriter_rank_(int *handle, size_t *rank);
int rp_dxwriter_rank__(int *handle, size_t *rank);
int RP_DXWRITER_RANK(int *handle, size_t *rank);

int rp_dxwriter_shape_(int *handle, size_t *shape);
int rp_dxwriter_shape__(int *handle, size_t *shape);
int RP_DXWRITER_SHAPE(int *handle, size_t *shape);

/**********************************************************/

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus
