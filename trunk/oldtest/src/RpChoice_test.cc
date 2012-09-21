/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpChoice.h"

int test_setPath(RpChoice* myChoice);
int test_setDefaultValue(RpChoice* myChoice);
int test_setCurrentValue(RpChoice* myChoice);
int test_setOption(RpChoice* myChoice);
int test_deleteOption(RpChoice* myChoice);

int test_setPath(RpChoice* myChoice)
{
    std::cout << "Path " << myChoice->getPath() << std::endl;
    myChoice->setPath("input.(ambiant)");
    std::cout << "Path " << myChoice->getPath() << std::endl;

    return 1;
}

int test_setDefaultValue(RpChoice* myChoice)
{
    std::cout << "defaultVal " << myChoice->getDefaultValue() << std::endl;
    myChoice->setDefaultValue("new default value");
    std::cout << "defaultVal " << myChoice->getDefaultValue() << std::endl;

    return 1;
}

int test_setCurrentValue(RpChoice* myChoice)
{
    std::cout << "currentVal " << myChoice->getCurrentValue() << std::endl;
    myChoice->setCurrentValue("new current value");
    std::cout << "currentVal " << myChoice->getCurrentValue() << std::endl;

    return 1;
}

int test_setOption(RpChoice* myChoice)
{
    unsigned int optListSize = myChoice->getOptListSize();
    myChoice->setOption("newOpt");
    std::string tmpOpt= myChoice->getFirstOption();
    std::string optList;

    std::cout << "begin setOption optListSize = " << optListSize << std::endl;

    while (tmpOpt != "") {
        optList += tmpOpt + " ";
        tmpOpt= myChoice->getNextOption();
    }

    std::cout << "setOption List = " << optList << std::endl;
    std::cout << "end setOption optListSize = " << myChoice->getOptListSize() << std::endl;

    return 1;
}

int test_deleteOption(RpChoice* myChoice)
{
    std::string tmpOpt= myChoice->getFirstOption();

    std::cout << "begin deleteOption optListSize = " << myChoice->getOptListSize() << std::endl;
    myChoice->deleteOption("newOpt");
    std::cout << "end deleteOption optListSize = " << myChoice->getOptListSize() << std::endl;

    return 1;
}

int main ()
{

    RpChoice* T1 = new RpChoice("input.(ambient).(temperature)",
                                "default-string",
                                "opt1,opt2,opt3");
    RpChoice* T2 = new RpChoice("input.(ambient).(temperature)",
                                "default-string",
                                "string3's label",
                                "string3's desc",
                                "opt4,opt5,opt6");
    

    std::cout << "T1 run" << std::endl;
    test_setPath(T1);
    test_setDefaultValue(T1);
    test_setCurrentValue(T1);
    test_setOption(T1);
    test_deleteOption(T1);

    std::cout << std::endl;

    std::cout << "T2 run" << std::endl;
    test_setPath(T2);
    test_setDefaultValue(T2);
    test_setCurrentValue(T2);
    test_setOption(T2);
    test_deleteOption(T2);

    std::cout << std::endl;
    std::cout << std::endl;

    return 0;
}
