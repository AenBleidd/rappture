#include "RpFortranCommon.h"

#ifndef _RpUNITS_F_H
#define _RpUNITS_F_H

#ifdef COMPNAME_ADD1UNDERSCORE
#   define rp_define_unit       rp_define_unit_
#   define rp_find              rp_find_
#   define rp_make_metric       rp_make_metric_
#   define rp_get_units         rp_get_units_
#   define rp_get_units_name    rp_get_units_name_
#   define rp_get_exponent      rp_get_exponent_
#   define rp_get_basis         rp_get_basis_
#   define rp_units_convert_dbl rp_units_convert_dbl_
#   define rp_units_convert_str rp_units_convert_str_
#   define rp_units_add_presets rp_units_add_presets_
#elif defined(COMPNAME_ADD2UNDERSCORE)
#   define rp_define_unit       rp_define_unit__
#   define rp_find              rp_find__
#   define rp_make_metric       rp_make_metric__
#   define rp_get_units         rp_get_units__
#   define rp_get_units_name    rp_get_units_name__
#   define rp_get_exponent      rp_get_exponent__
#   define rp_get_basis         rp_get_basis__
#   define rp_units_convert_dbl rp_units_convert_dbl__
#   define rp_units_convert_str rp_units_convert_str__
#   define rp_units_add_presets rp_units_add_presets__
#elif defined(COMPNAME_NOCHANGE)
#   define rp_define_unit       rp_define_unit
#   define rp_find              rp_find
#   define rp_make_metric       rp_make_metric
#   define rp_get_units         rp_get_units
#   define rp_get_units_name    rp_get_units_name
#   define rp_get_exponent      rp_get_exponent
#   define rp_get_basis         rp_get_basis
#   define rp_units_convert_dbl rp_units_convert_dbl
#   define rp_units_convert_str rp_units_convert_str
#   define rp_units_add_presets rp_units_add_presets
#elif defined(COMPNAME_UPPERCASE)
#   define rp_define_unit       RP_DEFINE_UNIT
#   define rp_find              RP_FIND
#   define rp_make_metric       RP_MAKE_METRIC
#   define rp_get_units         RP_GET_UNITS
#   define rp_get_units_name    RP_GET_UNITS_NAME 
#   define rp_get_exponent      RP_GET_EXPONENT
#   define rp_get_basis         RP_GET_BASIS
#   define rp_units_convert_dbl RP_UNITS_CONVERT_DBL
#   define rp_units_convert_str RP_UNITS_CONVERT_STR
#   define rp_units_add_presets RP_UNITS_ADD_PRESETS
#endif

#ifdef __cplusplus 
extern "C" {
#endif

int rp_define_unit(char* unitName, int* basisName, int unitName_len);

int rp_find(char* searchName, int searchName_len);

int rp_make_metric(int* basis);

int rp_get_units(int* unitRefVal, char* retText, int retText_len);

int rp_get_units_name(int* unitRefVal, char* retText, int retText_len);

int rp_get_exponent(int* unitRefVal, double retExponent);

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
