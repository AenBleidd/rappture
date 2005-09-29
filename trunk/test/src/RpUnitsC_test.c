//----------------------------------------------------------------------
// TEST: Cee's interface to RpUnits.
//
// Basic units conversion tests for the RpUnits portion of Rappture
// written in Cee.
//======================================================================
// AUTHOR:  Derrick Kearney, Purdue University
// Copyright (c) 2004-2005
// Purdue Research Foundation, West Lafayette, IN
//======================================================================
//
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

    RpUnits* meters = defineUnit("m",NULL);

    RpUnits* centimeters = NULL;
    RpUnits* nanometers = NULL;
    RpUnits* cm_basis = NULL;

    RpUnits* angstrom = defineUnit("A",NULL);
    RpUnits* fahrenheit = defineUnit("F",NULL);
    RpUnits* celcius = defineUnit("C",NULL);
    RpUnits* kelvin = defineUnit("K",NULL);

    defineConv(angstrom, meters, angstrom2meter, meter2angstrom);
    defineConv(fahrenheit, celcius, fahrenheit2centigrade, centigrade2fahrenheit);
    defineConv(celcius, kelvin, centigrade2kelvin, kelvin2centigrade);
    
    
    double cm_exp = 0;
    double nm_conv = 0;
    double value = 0;
    
    int result = 0;
    int showUnits = 0;

    const char* nm_conv_str = NULL;
    const char* retStr = NULL;
    
    makeMetric(meters);
    centimeters = find("cm");
    
    if (meters) {
        printf("meters sign is :%s:\n",getUnitsName(meters));
    }

    if (centimeters) {
        cm_exp = getExponent(centimeters);
        cm_basis = getBasis(centimeters);

        printf("centimeters sign is :%s:\n",getUnits(centimeters));
        printf("cm_exp is :%f:\n",cm_exp);

        if (cm_basis) {
            printf("cm_basis sign is :%s:\n",getUnitsName(cm_basis));
        }
        else {
            printf("cm_basis is NULL\n");
        }

    }

    nanometers = find("nm");

    if (nanometers) {

        nm_conv = convert_obj_double_result(nanometers,meters,1.0e9,&result);
        printf("1.0e9 nm = %f m\tresult = %d\n",nm_conv,result);

        nm_conv = convert_obj_double(nanometers,meters,1.0e9);
        printf("1.0e9 nm = %f m\n",nm_conv);

        showUnits = 1;
        nm_conv_str = convert_obj_str(nanometers,meters,1.588e9,showUnits);
        printf("1.588e9 nm = %s\n",nm_conv_str);

        showUnits = 0;
        nm_conv_str = convert_obj_str(nanometers,meters,1.588e9,showUnits);
        printf("1.588e9 nm = %s\n",nm_conv_str);
    }
    else {
        printf("nanometers is NULL\n");
    }

    if (meters && angstrom && centimeters) {
        value = convert_obj_double_result(angstrom,meters,1.0,&result);
        printf("1 angstrom = %e meters\n",value);

        value = convert_obj_double_result(centimeters,angstrom,1e-8,&result);
        printf("1.0e-8 centimeter = %f angstroms\n",value);
    }
    else {
        printf("meters or angstrom or centimeters is NULL\n");
    }


    if (fahrenheit && celcius) {
        value = convert_obj_double_result(fahrenheit,celcius,72,&result);
        printf("72 degrees fahrenheit = %f degrees celcius\n",value);
    
        value = convert_obj_double_result(celcius,fahrenheit,value,&result);
        printf("22.222 degrees celcius = %f degrees fahrenheit\n",value);
    }
    else {
        printf("fahrenheit or celcius is NULL\n");
    }
        
    if (celcius && kelvin) {
        value = convert_obj_double_result(celcius,kelvin,20,&result);
        printf("20 degrees celcius = %f kelvin\n",value);

        value = convert_obj_double_result(kelvin,celcius,300,&result);
        printf("300 kelvin = %f degrees celcius\n", value);
    }
    else {
        printf("celcius or kelvin is NULL\n");
    }

    printf("====== adding all preset units ======\n");
    add_presets("all");

    printf("====== TESTING STATIC CONVERT FXNS ======\n");

    retStr = convert("72F","C",1,&result);
    printf("72F = %s\tresult = %d\n", retStr,result);

    retStr = convert("300K","F",1,&result);
    printf("300K = %s\tresult = %d\n", retStr,result);

    retStr = convert("1eV","J",0,&result);
    printf("1eV = %s (no units)\tresult = %d\n", retStr,result);

    retStr = convert_str("300K","R",1,&result);
    printf("300K = %s\tresult = %d\n", retStr,result);

    retStr = convert_str("5m","ft",1,&result);
    printf("5m = %s\tresult = %d\n", retStr,result);

    value = convert_dbl("5000mV","V",&result);
    printf("5V = %f (double value)\n", value);

    
    return 0;
}
