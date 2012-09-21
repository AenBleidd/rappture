/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Utils Stub Functions
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifdef __cplusplus
    extern "C" {
#endif


/**********************************************************/

int
rp_utils_progress_ ( int* percent, char* text, int text_len ) {
    return rp_utils_progress (percent,text,text_len);
}

int
rp_utils_progress__ ( int* percent, char* text, int text_len ) {
    return rp_utils_progress (percent,text,text_len);
}

int
RP_UTILS_PROGRESS ( int* percent, char* text, int text_len ) {
    return rp_utils_progress (percent,text,text_len);
}

/**********************************************************/
/**********************************************************/

#ifdef __cplusplus
    }
#endif
