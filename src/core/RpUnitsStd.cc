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
 ****************************************/

double angstrom2meter (double angstrom)
{
        return angstrom*1.0e-10;
}

double meter2angstrom (double meters)
{
        return meters*1.0e10;
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


#ifdef __cplusplus
}
#endif
