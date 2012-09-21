/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Units Stub Function Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RpUNITS_F_STUBS_SRC
#define _RpUNITS_F_STUBS_SRC

#ifdef __cplusplus
extern "C" {
#endif

int
rp_define_unit_ (char* unitName, int* basisName, int unitName_len) {
    return rp_define_unit(unitName,basisName,unitName_len);
}

int
rp_define_unit__ (char* unitName, int* basisName, int unitName_len) {
    return rp_define_unit(unitName,basisName,unitName_len);
}

int
RP_DEFINE_UNIT (char* unitName, int* basisName, int unitName_len) {
    return rp_define_unit(unitName,basisName,unitName_len);
}

int
rp_find_ (char* searchName, int searchName_len) {
    return rp_find(searchName,searchName_len);
}

int
rp_find__ (char* searchName, int searchName_len) {
    return rp_find(searchName,searchName_len);
}

int
RP_FIND (char* searchName, int searchName_len) {
    return rp_find(searchName,searchName_len);
}

int
rp_get_units_ (int* unitRefVal, char* retText, int retText_len) {
    return rp_get_units(unitRefVal,retText,retText_len);
}

int
rp_get_units__ (int* unitRefVal, char* retText, int retText_len) {
    return rp_get_units(unitRefVal,retText,retText_len);
}

int
RP_GET_UNITS (int* unitRefVal, char* retText, int retText_len) {
    return rp_get_units(unitRefVal,retText,retText_len);
}

int
rp_get_units_name_     (int* unitRefVal, char* retText, int retText_len) {
    return rp_get_units_name(unitRefVal,retText,retText_len);
}

int
rp_get_units_name__    (int* unitRefVal, char* retText, int retText_len) {
    return rp_get_units_name(unitRefVal,retText,retText_len);
}

int
RP_GET_UNITS_NAME      (int* unitRefVal, char* retText, int retText_len) {
    return rp_get_units_name(unitRefVal,retText,retText_len);
}

int
rp_get_exponent_       (int* unitRefVal, double* retExponent) {
    return rp_get_exponent(unitRefVal,retExponent);
}

int
rp_get_exponent__      (int* unitRefVal, double* retExponent) {
    return rp_get_exponent(unitRefVal,retExponent);
}

int
RP_GET_EXPONENT        (int* unitRefVal, double* retExponent) {
    return rp_get_exponent(unitRefVal,retExponent);
}

int
rp_get_basis_          (int* unitRefVal) {
    return rp_get_basis(unitRefVal);
}

int
rp_get_basis__         (int* unitRefVal) {
    return rp_get_basis(unitRefVal);
}

int
RP_GET_BASIS           (int* unitRefVal) {
    return rp_get_basis(unitRefVal);
}

int
rp_units_convert_dbl_  (char* fromVal,
                            char* toUnitsName,
                            double* convResult,
                            int fromVal_len,
                            int toUnitsName_len ) {

    return rp_units_convert_dbl( fromVal, toUnitsName, convResult,
                                 fromVal_len, toUnitsName_len);
}

int
rp_units_convert_dbl__ (char* fromVal,
                            char* toUnitsName,
                            double* convResult,
                            int fromVal_len,
                            int toUnitsName_len ) {

    return rp_units_convert_dbl( fromVal, toUnitsName, convResult,
                                 fromVal_len, toUnitsName_len);
}

int
RP_UNITS_CONVERT_DBL   (char* fromVal,
                            char* toUnitsName,
                            double* convResult,
                            int fromVal_len,
                            int toUnitsName_len ) {

    return rp_units_convert_dbl( fromVal, toUnitsName, convResult,
                                 fromVal_len, toUnitsName_len);
}

int
rp_units_convert_str_  (char* fromVal,
                            char* toUnitsName,
                            char* retText,
                            int fromVal_len,
                            int toUnitsName_len,
                            int retText_len     ) {

    return rp_units_convert_str( fromVal,toUnitsName, retText, fromVal_len,
                                 toUnitsName_len, retText_len);
}

int
rp_units_convert_str__ (char* fromVal,
                            char* toUnitsName,
                            char* retText,
                            int fromVal_len,
                            int toUnitsName_len,
                            int retText_len     ) {

    return rp_units_convert_str( fromVal,toUnitsName, retText, fromVal_len,
                                 toUnitsName_len, retText_len);
}

int
RP_UNITS_CONVERT_STR   (char* fromVal,
                            char* toUnitsName,
                            char* retText,
                            int fromVal_len,
                            int toUnitsName_len,
                            int retText_len     ) {

    return rp_units_convert_str( fromVal,toUnitsName, retText, fromVal_len,
                                 toUnitsName_len, retText_len);
}

#ifdef __cplusplus
}
#endif

#endif
