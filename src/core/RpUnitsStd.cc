#include <RpUnitsStd.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************
 * METRIC CONVERSIONS
 ****************************************/


double centi2base (double centi, double power)
{
   return centi*pow(1e-2,power);
}

double milli2base (double milli, double power)
{
    return milli*pow(1e-3,power);
}

double micro2base (double micro, double power)
{
    return micro*pow(1e-6,power);
}

double nano2base (double nano, double power)
{
    return nano*pow(1e-9,power);
}

double pico2base (double pico, double power)
{
    return pico*pow(1e-12,power);
}

double femto2base (double femto, double power)
{
    return femto*pow(1e-15,power);
}

double atto2base (double atto, double power)
{
    return atto*pow(1e-18,power);
}

double kilo2base (double kilo, double power)
{
    return kilo*pow(1e3,power);
}

double mega2base (double mega, double power)
{
    return mega*pow(1e6,power);
}

double giga2base (double giga, double power)
{
    return giga*pow(1e9,power);
}

double tera2base (double tera, double power)
{
    return tera*pow(1e12,power);
}

double peta2base (double peta, double power)
{
    return peta*pow(1e15,power);
}

double base2centi (double base, double power)
{
    return base*pow(1e2,power);
}

double base2milli (double base, double power)
{
    return base*pow(1e3,power);
}

double base2micro (double base, double power)
{
    return base*pow(1e6,power);
}

double base2nano (double base, double power)
{
    return base*pow(1e9,power);
}

double base2pico (double base, double power)
{
    return base*pow(1e12,power);
}

double base2femto (double base, double power)
{
    return base*pow(1e15,power);
}

double base2atto (double base, double power)
{
    return base*pow(1e18,power);
}

double base2kilo (double base, double power)
{
    return base*pow(1e-3,power);
}

double base2mega (double base, double power)
{
    return base*pow(1e-6,power);
}

double base2giga (double base, double power)
{
    return base*pow(1e-9,power);
}

double base2tera (double base, double power)
{
    return base*pow(1e-12,power);
}

double base2peta (double base, double power)
{
    return base*pow(1e-15,power);
}

/****************************************
 * METRIC TO NON-METRIC CONVERSIONS
 * LENGTH CONVERSIONS
 * http://www.nodc.noaa.gov/dsdt/ucg/
 ****************************************/

double angstrom2meter (double angstrom, double power)
{
        return angstrom*(pow(1.0e-10,power));
}

double meter2angstrom (double meters, double power)
{
        return meters*(pow(1.0e10,power));
}

double meter2inch (double m, double power)
{
        return (m*(pow(39.37008,power)));
}

double inch2meter (double in, double power)
{
        return (in/(pow(39.37008,power)));
}

double meter2feet (double m, double power)
{
        return (m*(pow(3.280840,power)));
}

double feet2meter (double ft, double power)
{
        return (ft/(pow(3.280840,power)));
}

double meter2yard (double m, double power)
{
        return (m*(pow(1.093613,power)));
}

double yard2meter (double yd, double power)
{
        return (yd/(pow(1.093613,power)));
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

double cubicMeter2usGallon (double m3, double none)
{
        return (m3*264.1721);
}

double usGallon2cubicMeter (double g, double none)
{
        return (g/264.1721);
}

double cubicFeet2usGallon (double ft3, double none)
{
        return (ft3*7.48051);
}

double usGallon2cubicFeet (double g, double none)
{
        return (g/7.48051);
}


#ifdef __cplusplus
}
#endif
