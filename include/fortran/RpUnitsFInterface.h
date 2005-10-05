/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Units Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#ifndef _RpUNITS_F_H
#define _RpUNITS_F_H

#include "RpFortranCommon.h"
#include "RpUnitsFStubs.h"

#ifdef __cplusplus
extern "C" {
#endif

int rp_define_unit(char* unitName, int* basisName, int unitName_len);

int rp_find(char* searchName, int searchName_len);

int rp_make_metric(int* basis);

int rp_get_units(int* unitRefVal, char* retText, int retText_len);

int rp_get_units_name(int* unitRefVal, char* retText, int retText_len);

int rp_get_exponent(int* unitRefVal, double* retExponent);

int rp_get_basis(int* unitRefVal);

int rp_units_convert_dbl (  char* fromVal,
                            char* toUnitsName,
                            double* convResult,
                            int fromVal_len,
                            int toUnitsName_len );

int rp_units_convert_str (  char* fromVal,
                            char* toUnitsName,
                            char* retText,
                            int fromVal_len,
                            int toUnitsName_len,
                            int retText_len     );

int rp_units_add_presets ( char* presetName, int presetName_len);

#ifdef __cplusplus
}
#endif

#endif
