/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpVariable.h"

int test_getPath(RpVariable* myVariable);
int test_getDefaultValue(RpVariable* myVariable);
int test_getCurrentValue(RpVariable* myVariable);
int test_getLabel(RpVariable* myVariable);
int test_getDesc(RpVariable* myVariable);

int test_getPath(RpVariable* myVariable)
{
    std::cout << "Path " << myVariable->getPath() << std::endl;

    return 1;
}

int test_getDefaultValue(RpVariable* myVariable)
{
    std::cout << "defaultVal " << *((double*) myVariable->getDefaultValue()) << std::endl;

    return 1;
}

int test_getCurrentValue(RpVariable* myVariable)
{
    std::cout << "currentVal " << myVariable->getCurrentValue() << std::endl;

    return 1;
}

int test_getLabel(RpVariable* myVariable)
{
    std::cout << "label " << myVariable->getLabel() << std::endl;

    return 1;
}

int test_getDesc(RpVariable* myVariable)
{
    std::cout << "desc " << myVariable->getDesc() << std::endl;

    return 1;
}



int main ()
{

    RpVariable* T1 = new RpVariable("input.(ambient).(temperature)",new double(300.00));
    /*
    RpVariable* T2 = new RpVariable("input.(ambient).(temperature)",
                                "K",
                                300.00,
                                0.00,
                                1000.00,
                                "Temperature",
                                "Ambient Temperature, value between 0 and 1000"
                                );
    */
    
    test_getPath(T1);
    test_getDefaultValue(T1);
//    test_getCurrentValue(T1);
    test_getLabel(T1);
    test_getDesc(T1);

    std::cout << std::endl;

    /*
    test_getPath(T2);
    test_getUnits(T2);
    test_getDefaultValue(T2);
    test_getCurrentValue(T2);
    test_getMin(T2);
    test_getMax(T2);
    test_getLabel(T2);
    test_getDesc(T2);
    */

    std::cout << std::endl;

    return 0;
}
