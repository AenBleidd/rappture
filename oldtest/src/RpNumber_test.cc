/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpNumber.h"

int test_getPath(RpNumber* myNumber);
int test_getUnits(RpNumber* myNumber);
int test_getDefaultValue(RpNumber* myNumber);
int test_getCurrentValue(RpNumber* myNumber);
int test_getMin(RpNumber* myNumber);
int test_getMax(RpNumber* myNumber);
int test_getLabel(RpNumber* myNumber);
int test_getDesc(RpNumber* myNumber);
int test_convert(RpNumber* myNumber);

int test_getPath(RpNumber* myNumber)
{
    std::cout << "Path " << myNumber->getPath() << std::endl;

    return 1;
}

int test_getUnits(RpNumber* myNumber)
{
    std::cout << "units " << myNumber->getUnits() << std::endl;

    return 1;
}

int test_getDefaultValue(RpNumber* myNumber)
{
    std::cout << "defaultVal " << myNumber->getDefaultValue() << std::endl;

    return 1;
}

int test_getCurrentValue(RpNumber* myNumber)
{
    std::cout << "currentVal " << myNumber->getCurrentValue() << std::endl;

    return 1;
}

int test_getMin(RpNumber* myNumber)
{
    std::cout << "min " << myNumber->getMin() << std::endl;

    return 1;
}

int test_getMax(RpNumber* myNumber)
{
    std::cout << "max " << myNumber->getMax() << std::endl;

    return 1;
}

int test_getLabel(RpNumber* myNumber)
{
    std::cout << "label " << myNumber->getLabel() << std::endl;

    return 1;
}

int test_getDesc(RpNumber* myNumber)
{
    std::cout << "desc " << myNumber->getDesc() << std::endl;

    return 1;
}

int test_convert(RpNumber* myNumber)
{
    int result = 0;
    double newVal = myNumber->convert("C",1,&result);

    if (result) {
        std::cout << "newVal = " << newVal << std::endl;
    }
    else {
        std::cout << "newVal has a bad result" << std::endl;
    }

    return result;
}

void define_some_units()
{
    const RpUnits * meters = RpUnits::define("m", NULL);
    RpUnits::makeMetric(meters);
    RpUnits::define("V", NULL);
    RpUnits::define("s", NULL);
    RpUnits::define("cm2/Vs", NULL);
    const RpUnits* angstrom = RpUnits::define("A", NULL);
    RpUnits::define(angstrom, meters, angstrom2meter, meter2angstrom);
    const RpUnits* fahrenheit = RpUnits::define("F", NULL);
    const RpUnits* celcius = RpUnits::define("C", NULL);
    const RpUnits* kelvin = RpUnits::define("K", NULL);
    RpUnits::define(fahrenheit, celcius, fahrenheit2centigrade, centigrade2fahrenheit);
    RpUnits::define(celcius, kelvin, centigrade2kelvin, kelvin2centigrade);
}


int main ()
{

    define_some_units();

    RpNumber* T1 = new RpNumber("input.(ambient).(temperature)","K",300.00);
    RpNumber* T2 = new RpNumber("input.(ambient).(temperature)",
                                "K",
                                300.00,
                                0.00,
                                1000.00,
                                "Temperature",
                                "Ambient Temperature, value between 0 and 1000"
                                );
    

    test_getPath(T1);
    test_getUnits(T1);
    test_getDefaultValue(T1);
    test_getCurrentValue(T1);
    test_getMin(T1);
    test_getMax(T1);
    test_getLabel(T1);
    test_getDesc(T1);
    test_convert(T1);
    test_getCurrentValue(T1);
    test_getUnits(T1);

    std::cout << std::endl;

    test_getPath(T2);
    test_getUnits(T2);
    test_getDefaultValue(T2);
    test_getCurrentValue(T2);
    test_getMin(T2);
    test_getMax(T2);
    test_getLabel(T2);
    test_getDesc(T2);
    test_convert(T2);

    std::cout << std::endl;

    return 0;
}
