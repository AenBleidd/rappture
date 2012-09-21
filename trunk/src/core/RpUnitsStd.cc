/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <RpUnitsStd.h>
#include <math.h>

double invert (double inVal)
{
    return (1.0/inVal);
}

/****************************************
 * METRIC CONVERSIONS
 ****************************************/

double deci2base (double deci)
{
   return deci*1e-1;
}

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

double deca2base (double deca)
{
    return deca*1e1;
}

double hecto2base (double hecto)
{
    return hecto*1e2;
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

double exa2base (double exa)
{
    return exa*1e18;
}

double base2deci (double base)
{
    return base*1e1;
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

double base2deca (double base)
{
    return base*1e-1;
}

double base2hecto (double base)
{
    return base*1e-2;
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

double base2exa (double base)
{
    return base*1e-18;
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

double bohr2meter (double bohr)
{
    return bohr*(52.9177e-12);
}

double meter2bohr (double meter)
{
    return meter*(52.9177e12);
}

double meter2inch (double meter)
{
    return meter*(39.37008);
}

double inch2meter (double inch)
{
    return (inch/(39.37008));
}

double inch2feet (double inch)
{
    return (inch/(12.00));
}

double feet2inch (double ft)
{
    return (ft*(12.00));
}

double inch2yard (double inch)
{
    return (inch/(36.00));
}

double yard2inch (double yd)
{
    return (yd*(36.00));
}

double inch2mile (double inch)
{
    return (inch/(63360));
}

double mile2inch (double mi)
{
    return (mi*(63360));
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
    return ((5.0/9.0)*R);
}

double kelvin2rankine (double K)
{
    return ((9.0/5.0)*K);
}

double fahrenheit2kelvin (double F)
{
    return ((F+459.67)*(5.0/9.0));
}

double kelvin2fahrenheit (double K)
{
    return (((9.0/5.0)*K)-459.67);
}

double fahrenheit2rankine (double F)
{
    return (F+459.67);
}

double rankine2fahrenheit (double R)
{
    return (R-459.67);
}

double rankine2celcius (double R)
{
    return ((R*(5.0/9.0))-273.15);
}

double celcius2rankine (double C)
{
    return ((C + 273.15)*(9.0/5.0));
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

double cubicMeter2liter (double m3)
{
    return (m3*1e3);
}

double liter2cubicMeter (double L)
{
    return (L*1e-3);
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

/****************************************
 * PRESSURE CONVERSIONS
 * http://www.ilpi.com/msds/ref/pressureunits.html
 * http://en.wikipedia.org/wiki/Bar_%28unit%29
 ****************************************/

double bar2Pa (double bar)
{
    return (bar*100000.00);
}

double Pa2bar (double Pa)
{
    return (Pa/100000.00);
}

double bar2atm (double bar)
{
    return (bar*0.98692);
}

double atm2bar (double atm)
{
    return (atm/0.98692);
}

double bar2torr (double bar)
{
    return (bar*750.06);
}

double torr2bar (double torr)
{
    return (torr/750.06);
}

double bar2psi (double bar)
{
    return (bar*14.504);
}

double psi2bar (double psi)
{
    return (psi*0.0689476);
}

double Pa2atm (double Pa)
{
    return (Pa*9.8692e-6);
}

double atm2Pa (double atm)
{
    // need to check this conversion
    // tests fail
    return (atm*101325.024);
}

double Pa2torr (double Pa)
{
    return (Pa*7.5006e-3);
}

double torr2Pa (double torr)
{
    return (torr/7.5006e-3);
}

double Pa2psi (double Pa)
{
    return (Pa*145.04e-6);
}

double psi2Pa (double psi)
{
    // need to check this conversion
    // test fails because of truncation
    return (psi*6894.7625831);
}

double torr2atm (double torr)
{
    return (torr*1.3158e-3);
}

double atm2torr (double atm)
{
    return (atm*760);
}

double torr2psi (double torr)
{
    return (torr*19.337e-3);
}

double psi2torr (double psi)
{
    return (psi*51.71496);
}

double torr2mmHg (double torr)
{
    return (torr);
}

double mmHg2torr (double mmHg)
{
    return (mmHg);
}

double psi2atm (double psi)
{
    return (psi*68.046e-3);
}

double atm2psi (double atm)
{
    return (atm*14.696);
}

/****************************************
 * CONCENTRATION CONVERSIONS
 * http://en.wikipedia.org/wiki/PH
 ****************************************/

double pH2pOH (double pH)
{
    // This formula is valid exactly for
    // temperature = 298.15 K (25 °C) only,
    // but is acceptable for most lab calculations
    return (14.00 - pH);
}

double pOH2pH (double pOH)
{
    // This formula is valid exactly for
    // temperature = 298.15 K (25 °C) only,
    // but is acceptable for most lab calculations
    return (14.00 - pOH);
}

/****************************************
 * MAGNETIC CONVERSIONS
 * http://en.wikipedia.org/wiki/Tesla_(unit)
 * http://en.wikipedia.org/wiki/Gauss_(unit)
 * http://en.wikipedia.org/wiki/Maxwell_(unit)
 * http://en.wikipedia.org/wiki/Weber_(unit)
 ****************************************/

double tesla2gauss (double tesla)
{
    return (tesla*1e4);
}

double gauss2tesla (double gauss)
{
    return (gauss*1e-4);
}

double maxwell2weber (double maxwell)
{
    return (maxwell*1e-8);
}

double weber2maxwell (double weber)
{
    return (weber*1e8);
}

