/*
 * ======================================================================
 *  AUTHOR: Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpString.h"

int test_setPath(RpString* myString);
int test_setDefaultValue(RpString* myString);
int test_setCurrentValue(RpString* myString);
int test_setSize(RpString* myString);
int test_setHeight(RpString* myString);
int test_setWidth(RpString* myString);
int test_setLabel(RpString* myString);
int test_setDesc(RpString* myString);

int test_setPath(RpString* myString)
{
    std::cout << "Path " << myString->getPath() << std::endl;
    myString->setPath("input.(ambiant)");
    std::cout << "Path " << myString->getPath() << std::endl;

    return 1;
}

int test_setSize(RpString* myString)
{
    std::cout << "Size " << myString->getSize() << std::endl;
    myString->setSize("5x3");
    std::cout << "Size " << myString->getSize() << std::endl;

    return 1;
}

int test_setDefaultValue(RpString* myString)
{
    std::cout << "defaultVal " << myString->getDefaultValue() << std::endl;
    myString->setDefaultValue("new default value");
    std::cout << "defaultVal " << myString->getDefaultValue() << std::endl;

    return 1;
}

int test_setCurrentValue(RpString* myString)
{
    std::cout << "currentVal " << myString->getCurrentValue() << std::endl;
    myString->setCurrentValue("new current value");
    std::cout << "currentVal " << myString->getCurrentValue() << std::endl;

    return 1;
}

int test_setHeight(RpString* myString)
{
    std::cout << "Height " << myString->getHeight() << std::endl;
    myString->setHeight(2);
    std::cout << "Height " << myString->getHeight() << std::endl;

    return 1;
}

int test_setWidth(RpString* myString)
{
    std::cout << "width " << myString->getWidth() << std::endl;
    myString->setWidth(1);
    std::cout << "width " << myString->getWidth() << std::endl;

    return 1;
}

int test_setLabel(RpString* myString)
{
    std::cout << "label " << myString->getLabel() << std::endl;
    myString->setLabel("newLabel");
    std::cout << "label " << myString->getLabel() << std::endl;

    return 1;
}

int test_setDesc(RpString* myString)
{
    std::cout << "desc " << myString->getDesc() << std::endl;
    myString->setDesc("new description");
    std::cout << "desc " << myString->getDesc() << std::endl;

    return 1;
}

int main ()
{

    RpString* T1 = new RpString("input.(ambient).(temperature)","default-string");
    RpString* T2 = new RpString("input.(ambient).(temperature)",
                                "default-string",
                                "15x20");
    RpString* T3 = new RpString("input.(ambient).(temperature)",
                                "default-string",
                                "100x200",
                                "string3's label",
                                "string3's desc"
                                );
    

    std::cout << "T1 run" << std::endl;
    test_setPath(T1);
    test_setDefaultValue(T1);
    test_setCurrentValue(T1);
    test_setSize(T1);
    test_setHeight(T1);
    test_setWidth(T1);
    test_setLabel(T1);
    test_setDesc(T1);

    std::cout << std::endl;

    std::cout << "T2 run" << std::endl;
    test_setPath(T2);
    test_setDefaultValue(T2);
    test_setCurrentValue(T2);
    test_setSize(T2);
    test_setHeight(T2);
    test_setWidth(T2);
    test_setLabel(T2);
    test_setDesc(T2);

    std::cout << std::endl;

    std::cout << "T3 run" << std::endl;
    test_setPath(T3);
    test_setDefaultValue(T3);
    test_setCurrentValue(T3);
    test_setSize(T3);
    test_setHeight(T3);
    test_setWidth(T3);
    test_setLabel(T3);
    test_setDesc(T3);

    std::cout << std::endl;

    return 0;
}
