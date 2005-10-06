/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Units Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct RpUnits RpUnits;

    // unit definition functions
    RpUnits* rpDefineUnit        ( const char* unitSymbol, RpUnits* basis );

    // conversion definition functions
    RpUnits* rpDefineConv        ( RpUnits* fromUnit,
                                   RpUnits* toUnit,
                                   double (*convForwFxnPtr)(double),
                                   double (*convBackFxnPtr)(double)    );

    // unit attribute access functions
    const char* rpGetUnits       ( RpUnits* unit );

    const char* rpGetUnitsName   ( RpUnits* unit );

    double rpGetExponent         ( RpUnits* unit );

    RpUnits* rpGetBasis          ( RpUnits* unit);

    RpUnits* rpFind              ( const char* unitSymbol);

    int rpMakeMetric             ( RpUnits* basis );

    // convert functions

    const char* rpConvert        ( const char* fromVal,
                                   const char* toUnitsName,
                                   int showUnits,
                                   int* result );

    const char* rpConvertStr     ( const char* fromVal,
                                   const char* toUnitsName,
                                   int showUnits,
                                   int* result );

    const char* rpConvert_ObjStr ( RpUnits* fromUnits,
                                   RpUnits* toUnits,
                                   double val,
                                   int showUnits,
                                   int* result );

    double rpConvertDbl          ( const char* fromVal,
                                   const char* toUnitsName,
                                   int* result );

    double rpConvert_ObjDbl      ( RpUnits* fromUnits,
                                   RpUnits* toUnits,
                                   double val,
                                   int* result );

    int rpAddPresets ( const char* presetName );

#ifdef __cplusplus
}
#endif
