/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Units Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RpUNITS_C_H
#define _RpUNITS_C_H

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

    typedef struct RpUnits RpUnits;

    // unit definition functions
    const RpUnits* rpDefineUnit  ( const char* unitSymbol,
                                   const RpUnits* basis );

    // conversion definition functions
    const RpUnits* rpDefineConv  ( const RpUnits* fromUnit,
                                   const RpUnits* toUnit,
                                   double (*convForwFxnPtr)(double),
                                   double (*convBackFxnPtr)(double)    );

    // unit attribute access functions
    const char* rpGetUnits       ( const RpUnits* unit );

    const char* rpGetUnitsName   ( const RpUnits* unit );

    double rpGetExponent         ( const RpUnits* unit );

    const RpUnits* rpGetBasis    ( const RpUnits* unit);

    const RpUnits* rpFind        ( const char* unitSymbol);

    // convert functions

    const char* rpConvert        ( const char* fromVal,
                                   const char* toUnitsName,
                                   int showUnits,
                                   int* result );

    const char* rpConvertStr     ( const char* fromVal,
                                   const char* toUnitsName,
                                   int showUnits,
                                   int* result );

    const char* rpConvert_ObjStr ( const RpUnits* fromUnits,
                                   const RpUnits* toUnits,
                                   double val,
                                   int showUnits,
                                   int* result );

    double rpConvertDbl          ( const char* fromVal,
                                   const char* toUnitsName,
                                   int* result );

    double rpConvert_ObjDbl      ( const RpUnits* fromUnits,
                                   const RpUnits* toUnits,
                                   double val,
                                   int* result );

    int rpAddPresets ( const char* presetName );

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // ifndef _RpUNITS_C_H
