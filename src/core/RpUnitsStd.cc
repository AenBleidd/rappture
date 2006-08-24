/*
 * ======================================================================
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <RpUnitsStd.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************
 * METRIC CONVERSIONS
 ****************************************/


double centi2base (double centi)
{
   return centi*1e-2;
}

double milli2base (double milli)
{
    return milli*1e-3;
}

double micro2base (double micro)
{
    return micro*1e-6;
}

double nano2base (double nano)
{
    return nano*1e-9;
}

double pico2base (double pico)
{
    return pico*1e-12;
}

double femto2base (double femto)
{
    return femto*1e-15;
}

double atto2base (double atto)
{
    return atto*1e-18;
}

double kilo2base (double kilo)
{
    return kilo*1e3;
}

double mega2base (double mega)
{
    return mega*1e6;
}

double giga2base (double giga)
{
    return giga*1e9;
}

double tera2base (double tera)
{
    return tera*1e12;
}

double peta2base (double peta)
{
    return peta*1e15;
}

double base2centi (double base)
{
    return base*1e2;
}

double base2milli (double base)
{
    return base*1e3;
}

double base2micro (double base)
{
    return base*1e6;
}

double base2nano (double base)
{
    return base*1e9;
}

double base2pico (double base)
{
    return base*1e12;
}

double base2femto (double base)
{
    return base*1e15;
}

double base2atto (double base)
{
    return base*1e18;
}

double base2kilo (double base)
{
    return base*1e-3;
}

double base2mega (double base)
{
    return base*1e-6;
}

double base2giga (double base)
{
    return base*1e-9;
}

double base2tera (double base)
{
    return base*1e-12;
}

double base2peta (double base)
{
    return base*1e-15;
}

/****************************************
 * METRIC TO NON-METRIC CONVERSIONS
 * LENGTH CONVERSIONS
 * http://www.nodc.noaa.gov/dsdt/ucg/
 ****************************************/

double angstrom2meter (double angstrom)
{
    return angstrom*(1.0e-10);
}

double meter2angstrom (double meter)
{
    return meter*(1.0e10);
}

double meter2inch (double meter)
{
    return meter*(39.37008);
}

double inch2meter (double in)
{
    return (in/(39.37008));
}

double meter2feet (double meter)
{
    return (meter*(3.280840));
}

double feet2meter (double ft)
{
    return (ft/(3.280840));
}

double meter2yard (double meter)
{
    return (meter*(1.093613));
}

double yard2meter (double yd)
{
    return (yd/(1.093613));
}

/****************************************
 * TEMPERATURE CONVERSIONS
 ****************************************/

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

double rankine2kelvin (double R)
{
    return ((9.0/5.0)*R);
}

double kelvin2rankine (double K)
{
    return ((5.0/9.0)*K);
}

double fahrenheit2kelvin (double F)
{
    return ((F+459.67)*(5.0/9.0));
}

double kelvin2fahrenheit (double K)
{
    return (((9.0/5.0)*K)-459.67);
}

/****************************************
 * ENERGY CONVERSIONS
 ****************************************/

double electronVolt2joule (double eV)
{
    return (eV*1.602177e-19);
}

double joule2electronVolt (double J)
{
    return (J/1.602177e-19);
}

/****************************************
 * MISC VOLUME CONVERSIONS
 ****************************************/

double cubicMeter2usGallon (double m3)
{
    return (m3*264.1721);
}

double usGallon2cubicMeter (double gal)
{
    return (gal/264.1721);
}

double cubicFeet2usGallon (double ft3)
{
    return (ft3*7.48051);
}

double usGallon2cubicFeet (double gal)
{
    return (gal/7.48051);
}

/****************************************
 * ANGLE CONVERSIONS
 * http://www.metrication.com/
 ****************************************/

double rad2deg (double rad)
{
    return (rad*(180.00/M_PI));
}

double deg2rad (double deg)
{
    return (deg*(M_PI/180.00));
}

double rad2grad (double rad)
{
    return (rad*(200.00/M_PI));
}

double grad2rad (double grad)
{
    return (grad*(M_PI/200.00));
}

double deg2grad (double deg)
{
    return (deg*(10.00/9.00));
}

double grad2deg (double grad)
{
    return (grad*(9.00/10.00));
}

/****************************************
 * TIME CONVERSIONS
 ****************************************/

double sec2min (double sec)
{
    return (sec/60.00);
}

double min2sec (double min)
{
    return (min*60.00);
}

double sec2hour (double sec)
{
    return (sec/3600.00);
}

double hour2sec (double hour)
{
    return (hour*3600.00);
}

double sec2day (double sec)
{
    return (sec/86400.00);
}

double day2sec (double day)
{
    return (day*86400.00);
}



#ifdef __cplusplus
}
#endif
