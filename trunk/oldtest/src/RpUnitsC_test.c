/*
 *----------------------------------------------------------------------
 * TEST: Cee's interface to RpUnits.
 *
 * Basic units conversion tests for the RpUnits portion of Rappture
 * written in Cee.
 *======================================================================
 * AUTHOR:  Derrick Kearney, Purdue University
 * Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *======================================================================
 */
#include "RpUnitsCInterface.h"
#include <stdio.h>

double fahrenheit2centigrade (double F);
double centigrade2fahrenheit (double C);
double centigrade2kelvin (double C);
double kelvin2centigrade (double K);

double angstrom2meter (double angstrom)
{
    return angstrom*1.0e-10;
}

double meter2angstrom (double meters)
{
    return meters*1.0e10;
}

double fahrenheit2centigrade (double F)
{
    return ((F-32.0)/(9.0/5.0));
}

double centigrade2fahrenheit (double C)
{
    return ((C*(9.0/5.0))+32.0);
}

double centigrade2kelvin (double C)
{
    return (C+273.15);
}

double kelvin2centigrade (double K)
{
    return (K-273.15);
}

int main()
{

    const RpUnits* meters = rpDefineUnit("m",NULL);

    const RpUnits* centimeters = NULL;
    const RpUnits* nanometers = NULL;
    const RpUnits* cm_basis = NULL;

    const RpUnits* angstrom = rpDefineUnit("A",NULL);
    const RpUnits* fahrenheit = rpDefineUnit("F",NULL);
    const RpUnits* celcius = rpDefineUnit("C",NULL);
    const RpUnits* kelvin = rpDefineUnit("K",NULL);

    rpDefineConv(angstrom, meters, angstrom2meter, meter2angstrom);
    rpDefineConv(fahrenheit, celcius, fahrenheit2centigrade, centigrade2fahrenheit);
    rpDefineConv(celcius, kelvin, centigrade2kelvin, kelvin2centigrade);


    double cm_exp = 0;
    double nm_conv = 0;
    double value = 0;

    int result = 0;
    int showUnits = 0;

    const char* nm_conv_str = NULL;
    const char* retStr = NULL;

    rpMakeMetric(meters);
    centimeters = rpFind("cm");

    if (meters) {
        printf("meters sign is :%s:\n",rpGetUnitsName(meters));
    }

    if (centimeters) {
        cm_exp = rpGetExponent(centimeters);
        cm_basis = rpGetBasis(centimeters);

        printf("centimeters sign is :%s:\n",rpGetUnits(centimeters));
        printf("cm_exp is :%f:\n",cm_exp);

        if (cm_basis) {
            printf("cm_basis sign is :%s:\n",rpGetUnitsName(cm_basis));
        }
        else {
            printf("cm_basis is NULL\n");
        }

    }

    nanometers = rpFind("nm");

    if (nanometers) {

        nm_conv = rpConvert_ObjDbl(nanometers,meters,1.0e9,&result);
        printf("1.0e9 nm = %f m\tresult = %d\n",nm_conv,result);

        nm_conv = rpConvert_ObjDbl(nanometers,meters,1.0e9, NULL);
        printf("1.0e9 nm = %f m\n",nm_conv);

        showUnits = 1;
        nm_conv_str = rpConvert_ObjStr(nanometers,meters,1.588e9,showUnits,NULL);
        printf("1.588e9 nm = %s\n",nm_conv_str);

        showUnits = 0;
        nm_conv_str = rpConvert_ObjStr(nanometers,meters,1.588e9,showUnits,NULL);
        printf("1.588e9 nm = %s\n",nm_conv_str);
    }
    else {
        printf("nanometers is NULL\n");
    }

    if (meters && angstrom && centimeters) {
        value = rpConvert_ObjDbl(angstrom,meters,1.0,&result);
        printf("1 angstrom = %e meters\n",value);

        value = rpConvert_ObjDbl(centimeters,angstrom,1e-8,&result);
        printf("1.0e-8 centimeter = %f angstroms\n",value);
    }
    else {
        printf("meters or angstrom or centimeters is NULL\n");
    }


    if (fahrenheit && celcius) {
        value = rpConvert_ObjDbl(fahrenheit,celcius,72,&result);
        printf("72 degrees fahrenheit = %f degrees celcius\n",value);

        value = rpConvert_ObjDbl(celcius,fahrenheit,value,&result);
        printf("22.222 degrees celcius = %f degrees fahrenheit\n",value);
    }
    else {
        printf("fahrenheit or celcius is NULL\n");
    }

    if (celcius && kelvin) {
        value = rpConvert_ObjDbl(celcius,kelvin,20,&result);
        printf("20 degrees celcius = %f kelvin\n",value);

        value = rpConvert_ObjDbl(kelvin,celcius,300,&result);
        printf("300 kelvin = %f degrees celcius\n", value);
    }
    else {
        printf("celcius or kelvin is NULL\n");
    }

    printf("====== adding all preset units ======\n");
    rpAddPresets("all");

    printf("====== TESTING STATIC CONVERT FXNS ======\n");

    retStr = rpConvert("72F","C",1,&result);
    printf("72F = %s\tresult = %d\n", retStr,result);

    retStr = rpConvert("300K","F",1,&result);
    printf("300K = %s\tresult = %d\n", retStr,result);

    retStr = rpConvert("1eV","J",0,&result);
    printf("1eV = %s (no units)\tresult = %d\n", retStr,result);

    retStr = rpConvertStr("300K","R",1,&result);
    printf("300K = %s\tresult = %d\n", retStr,result);

    retStr = rpConvertStr("5m","ft",1,&result);
    printf("5m = %s\tresult = %d\n", retStr,result);

    value = rpConvertDbl("5000mV","V",&result);
    printf("5V = %f (double value)\n", value);


    return 0;
}
