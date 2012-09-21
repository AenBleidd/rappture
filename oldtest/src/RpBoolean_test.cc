/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpBoolean.h"

int test_setPath(RpBoolean* myBoolean);
int test_setDefaultValue(RpBoolean* myBoolean);
int test_setCurrentValue(RpBoolean* myBoolean);
int test_setLabel(RpBoolean* myBoolean);
int test_setDesc(RpBoolean* myBoolean);

int test_setPath(RpBoolean* myBoolean)
{
    std::cout << "Path " << myBoolean->getPath() << std::endl;
    myBoolean->setPath("input.(ambiant)");
    std::cout << "Path " << myBoolean->getPath() << std::endl;

    return 1;
}


int test_setDefaultValue(RpBoolean* myBoolean)
{
    std::cout << "defaultVal " << myBoolean->getDefaultValue() << std::endl;
    myBoolean->setDefaultValue("new default value");
    std::cout << "defaultVal " << myBoolean->getDefaultValue() << std::endl;

    return 1;
}

int test_setCurrentValue(RpBoolean* myBoolean)
{
    std::cout << "currentVal " << myBoolean->getCurrentValue() << std::endl;
    myBoolean->setCurrentValue("new current value");
    std::cout << "currentVal " << myBoolean->getCurrentValue() << std::endl;

    return 1;
}

int test_setLabel(RpBoolean* myBoolean)
{
    std::cout << "label " << myBoolean->getLabel() << std::endl;
    myBoolean->setLabel("newLabel");
    std::cout << "label " << myBoolean->getLabel() << std::endl;

    return 1;
}

int test_setDesc(RpBoolean* myBoolean)
{
    std::cout << "desc " << myBoolean->getDesc() << std::endl;
    myBoolean->setDesc("new description");
    std::cout << "desc " << myBoolean->getDesc() << std::endl;

    return 1;
}

int main ()
{

    RpBoolean* T1 = new RpBoolean("input.(ambient).(temperature)","default-string");
    RpBoolean* T2 = new RpBoolean("input.(ambient).(temperature)",
                                "default-string",
                                "boolean2's label",
                                "boolean2's desc"
                                );
    

    std::cout << "T1 run" << std::endl;
    test_setPath(T1);
    test_setDefaultValue(T1);
    test_setCurrentValue(T1);
    test_setLabel(T1);
    test_setDesc(T1);

    std::cout << std::endl;

    std::cout << "T2 run" << std::endl;
    test_setPath(T2);
    test_setDefaultValue(T2);
    test_setCurrentValue(T2);
    test_setLabel(T2);
    test_setDesc(T2);

    std::cout << std::endl;
    std::cout << std::endl;

    return 0;
}
