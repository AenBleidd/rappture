/*
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpUnits.h"
#include <list>
#include <string>
#include <cassert>


void success() 
{
    printf ("Test Successful\n");
}

void fail() 
{
    printf ("Test FAILED !!!!!!!!!!!!!!!!!\n");
    exit(-1);
}

// compare_numbers()

int main()
{
    double value = 0.0;
    std::string strValue;
    int result = 0;

    //
    // use main to test class functionality
    // 

    // test define(units,basis)
    // this should create an object of type RpUnits
    // with the units of "m" and basis should be NULL 
    // which tells us that this object represents a fundamental system
    // of units
    //
    printf ("=============== TEST 1 ===============\n");

    const RpUnits* meters = RpUnits::find("m");
    const RpUnits* cmeters = RpUnits::find("cm");
    const RpUnits* angstrom = RpUnits::find("A");

    value = angstrom->convert(meters,1.0,&result);
    std::cout << "1 angstrom = " << value << " meters" << std::endl;
    std::cout << "result = " << result << std::endl;

    result = 0;
    value = cmeters->convert(angstrom,1e-8,&result);
    std::cout << "1.0e-8 centimeter = " << value << " angstroms" << std::endl;


    const RpUnits* fahrenheit  = RpUnits::find("F");
    const RpUnits* celcius  = RpUnits::find("C");
    const RpUnits* kelvin  = RpUnits::find("K");

    value = fahrenheit->convert(celcius,72,&result);
    std::cout << "72 degrees fahrenheit = " << value << " degrees celcius" << std::endl;

    value = celcius->convert(fahrenheit,value,&result);
    std::cout << "22.222 degrees celcius = " << value << " degrees fahrenheit" << std::endl;

    value = celcius->convert(kelvin,20,&result);
    std::cout << "20 degrees celcius = " << value << " kelvin" << std::endl;

    value = kelvin->convert(celcius,300,&result);
    std::cout << "300 kelvin = " << value << " degrees celcius" << std::endl;

/*
    // test getUnits() member function of objects of type RpUnits
    // this test should return the units of "m" for the oject
    // meters
    //
    printf ("=============== TEST 2 ===============\n");
    (meters->getUnits().c_str()) ? success() : fail();
    printf("meters units = :%s:\n", meters->getUnits().c_str());
    
    // test getBasis() functionality of objects of type RpUnits
    // create an object of type RpUnits and associate it with the 
    // meters RpUnits object as its basis.
    // print out what it reports its basis is.
    //
    printf ("=============== TEST 3 ===============\n");
    std::string t3units = "cm";
    // RpUnits * centimeters = RpUnits::define("cm", meters);
    RpUnits * centimeters = RpUnits::define(t3units, meters);
    (centimeters) ? success() : fail();
    printf("cm units = :%s:\n", centimeters->getUnits().c_str() );
    printf("cm basis = :%s:\n", centimeters->getBasis()->getUnits().c_str() );

    // test makeBasis()
    //
    printf ("=============== TEST 4.1 ===============\n");
    // value = centimeters->makeBasis(100.00);
    value = 100.00;
    failTest = centimeters->makeBasis(&value);
    (value == 1 && !failTest) ? success() : fail();
    printf("100 cm = :%f: meters\n", value);

    // test makeBasis() with a list of values, all the same units
    // 
    printf ("=============== TEST 4.2 ===============\n");
//    
//    double * valueArr = (double *) calloc(12,sizeof(double));
//    if (!valueArr) {
//        // complain that malloc failed
//        exit(-2);
//    }
//
//    valueArr = centimeters->makeBasis(100.00);
//
    // (value == 1) ? success() : fail();
    // printf("100 cm = :%f: meters\n", value);

    // test makeBasis() with a list of values, all different units
    // 
    printf ("=============== TEST 4.3 ===============\n");
    // value = centimeters->makeBasis(100.00);
    // (value == 1) ? success() : fail();
    // printf("100 cm = :%f: meters\n", value);

*/

    printf ("=============== TEST 4.4 ===============\n");

    const RpUnits * millimeter = RpUnits::find("mm");
    const RpUnits * micrometer = RpUnits::find("um");
    const RpUnits * nanometer  = RpUnits::find("nm");
    const RpUnits * picometer  = RpUnits::find("pm");
    const RpUnits * femtometer = RpUnits::find("fm");
    const RpUnits * attometer  = RpUnits::find("am");
    const RpUnits * kilometer  = RpUnits::find("km");
    const RpUnits * megameter  = RpUnits::find("Mm");
    const RpUnits * gigameter  = RpUnits::find("Gm");
    const RpUnits * terameter  = RpUnits::find("Tm");
    const RpUnits * petameter  = RpUnits::find("Pm");

    value = 1.0e+3;
    millimeter->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e3 mm = :%f: meters\n", value);

    value = 1.0e+6; 
    micrometer->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e6 um = :%f: meters\n", value);

    value = 1.0e+9;
    nanometer->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e9 nm = :%f: meters\n", value);

    value = 1.0e+12;
    picometer->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e12 pm = :%f: meters\n", value);

    value = 1.0e+15;
    femtometer->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e15 fm = :%f: meters\n", value);

    value = 1.0e+18;
    attometer->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e18 am = :%f: meters\n", value);

    value = 1.0e-3;
    kilometer->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e-3 km = :%f: meters\n", value);

    value = 1.0e-6;
    megameter->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e-6 Mm = :%f: meters\n", value);

    value = 1.0e-9;
    gigameter->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e-9 Gm = :%f: meters\n", value);

    value = 1.0e-12;
    terameter->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e-12 Gm = :%f: meters\n", value);

    value = 1.0e-15;
    petameter->makeBasis(&value);
    (value == 1) ? success() : fail();
    printf("1.0e-15 Pm = :%f: meters\n", value);

    value = 2;
    meters->makeBasis(&value);
    (value == 2) ? success() : fail();
    printf("2 m = :%f: meters\n", value);

    printf ("=============== TEST 5 ===============\n");

    strValue = RpUnits::convert("72F","C",1);
    std::cout << "result = " << result << std::endl;
    std::cout << "strValue convert(72F,C,1) = " << strValue << std::endl;

    strValue = RpUnits::convert("72F","C",0);
    std::cout << "result = " << result << std::endl;
    std::cout << "strValue convert(72F,C,0) = " << strValue << std::endl;

    strValue = RpUnits::convert("20C","K",1);
    std::cout << "result = " << result << std::endl;
    std::cout << "strValue convert(20C,K,1) = " << strValue << std::endl;

    strValue = RpUnits::convert("20C","K",0);
    std::cout << "result = " << result << std::endl;
    std::cout << "strValue convert(20C,K,1) = " << strValue << std::endl;

    strValue = RpUnits::convert("300K","C",1);
    std::cout << "result = " << result << std::endl;
    std::cout << "strValue convert(300K,C,1) = " << strValue << std::endl;

    strValue = RpUnits::convert("300K","C",0);
    std::cout << "result = " << result << std::endl;
    std::cout << "strValue convert(300K,C,0) = " << strValue << std::endl;




    const RpUnits* eV  = RpUnits::find("eV");
    const RpUnits* joules  = RpUnits::find("J");

    value = joules->convert(eV,1,&result);
    std::cout << "1 joule = " << value << " electronVolts" << std::endl;

    value = eV->convert(joules,1,&result);
    std::cout << "1 electronVolt = " << value << " joules" << std::endl;

    strValue = RpUnits::convert("10eV","J",1);
    std::cout << "strValue convert(10eV,J,1) = " << strValue << std::endl;

    strValue = RpUnits::convert("1eV","J",0);
    std::cout << "strValue convert(1eV,J,0) = " << strValue << std::endl;

    strValue = RpUnits::convert("10J","eV",1);
    std::cout << "strValue convert(10J,eV,1) = " << strValue << std::endl;

    strValue = RpUnits::convert("1J","eV",0);
    std::cout << "strValue convert(1J,eV,0) = " << strValue << std::endl;






    std::cout << "TESTING COPY CONSTRUCTOR" << std::endl;

    RpUnits *origRpUnits = RpUnits::define("obj2", NULL, "");
    RpUnits copyRpUnits = *origRpUnits;

    std::cout << "origRpUnits = " << origRpUnits->getUnitsName() << std::endl;
    std::cout << "copyRpUnits = " << copyRpUnits.getUnitsName() << std::endl;
    std::cout << "modifying origRpUnits" << std::endl;
    delete origRpUnits;
    origRpUnits = RpUnits::define("obj3",NULL, "");
    std::cout << "origRpUnits = " << origRpUnits->getUnitsName() << std::endl;
    std::cout << "copyRpUnits = " << copyRpUnits.getUnitsName() << std::endl;
    std::cout << "deleting origRpUnits" << std::endl;
    delete origRpUnits;
    std::cout << "copyRpUnits = " << copyRpUnits.getUnitsName() << std::endl;

    std::cout << "TESTING COPY ASSIGNMENT OPERATOR" << std::endl;

    RpUnits *testRpUnits = RpUnits::define("test2", NULL, "");
    copyRpUnits = *testRpUnits;

    std::cout << "testRpUnits = " << testRpUnits->getUnitsName() << std::endl;
    std::cout << "copyRpUnits = " << copyRpUnits.getUnitsName() << std::endl;
    std::cout << "modifying testRpUnits" << std::endl;
    delete testRpUnits;
    testRpUnits = RpUnits::define("test3",NULL, "");
    std::cout << "testRpUnits = " << testRpUnits->getUnitsName() << std::endl;
    std::cout << "copyRpUnits = " << copyRpUnits.getUnitsName() << std::endl;
    std::cout << "deleting testRpUnits" << std::endl;
    delete testRpUnits;
    std::cout << "copyRpUnits = " << copyRpUnits.getUnitsName() << std::endl;


    // test deleting a const object

    const RpUnits* myobj = RpUnits::define("myunit",NULL, "");
    delete myobj;

    // test /cm2
    // std::cout << "convert (3m3 -> cm3)     = " << RpUnits::convert("3m3","cm3",0) << std::endl;
    // std::cout << "convert (3/m3 -> /cm3)   = " << RpUnits::convert("3/m3","/cm3",0) << std::endl;
    // std::cout << "convert (300cm3 -> m3)   = " << RpUnits::convert("300cm3","m3",0) << std::endl;
    // std::cout << "convert (300/cm3 -> /m3) = " << RpUnits::convert("300/cm3","/m3",0) << std::endl;

    std::cout << "convert (3m3 -> cm3)     = " << RpUnits::convert("3m3","cm3",0) << std::endl;
    std::cout << "convert (3/m3 -> /cm3)   = " << RpUnits::convert("3/m3","/cm3",0) << std::endl;
    std::cout << "convert (300cm3 -> m3)   = " << RpUnits::convert("300cm3","m3",0) << std::endl;
    std::cout << "convert (300/cm3 -> /m3) = " << RpUnits::convert("300/cm3","/m3",0) << std::endl;
    std::cout << "convert (72F -> 22.22C) = " << RpUnits::convert("72F","C",0) << std::endl;
    std::cout << "convert (5J -> 3.12075e+28neV) = " << RpUnits::convert("5J","neV",0) << std::endl;
    std::cout << "convert (5J2 -> 1.9478e+56neV2) = " << RpUnits::convert("5J2","neV2",0) << std::endl;

    // testing complex units
    std::cout << "convert (1cm2/Vs -> 0.0001m2/Vs) = " << RpUnits::convert("1cm2/Vs","m2/Vs",0) << std::endl;
    std::cout << "convert (1cm2/Vs -> 0.1m2/kVs) = " << RpUnits::convert("1cm2/Vs","m2/kVs",0) << std::endl;
    std::cout << "convert (1cm2/Vs -> 1e-7m2/kVus) = " << RpUnits::convert("1cm2/Vs","m2/kVus",0) << std::endl;

    const RpUnits* myMeters = RpUnits::find("cm");
    std::list<std::string> convList = myMeters->getCompatible();
    std::list<std::string>::iterator convListIter;

    std::cout << myMeters->getUnitsName() << " is of type: " << myMeters->getType() << std::endl;
    std::cout << myMeters->getUnitsName() << " is compatible with: " << std::endl;
    convListIter = convList.begin();
    while (convListIter != convList.end()) {
        std::cout << *convListIter << std::endl;
        convListIter++;
    }

    std::cout << "convert(\"5.00mm\",\"\",1) = " << RpUnits::convert("5.00mm","",1) << std::endl;
    std::cout << "convert(\"5.00mm\",\"\",0) = " << RpUnits::convert("5.00mm","",0) << std::endl;

    // day hour min sec test
    assert((RpUnits::convert("5.00s","s",RpUnits::UNITS_ON)) == "5s");
    std::cout << "convert(\"5.00s\",\"s\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00s","s",RpUnits::UNITS_OFF)) == "5");
    std::cout << "convert(\"5.00s\",\"s\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00min","s",RpUnits::UNITS_ON)) == "300s");
    std::cout << "convert(\"5.00min\",\"s\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00min","s",RpUnits::UNITS_OFF)) == "300");
    std::cout << "convert(\"5.00min\",\"s\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00h","s",RpUnits::UNITS_ON)) == "18000s");
    std::cout << "convert(\"5.00h\",\"s\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00h","s",RpUnits::UNITS_OFF)) == "18000");
    std::cout << "convert(\"5.00h\",\"s\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00d","s",RpUnits::UNITS_ON)) == "432000s");
    std::cout << "convert(\"5.00d\",\"s\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00d","s",RpUnits::UNITS_OFF)) == "432000");
    std::cout << "convert(\"5.00d\",\"s\",UNITS_OFF) passed" << std::endl;

    assert((RpUnits::convert("5.00s","min",RpUnits::UNITS_ON)) == "0.0833333min");
    std::cout << "convert(\"5.00s\",\"min\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00s","min",RpUnits::UNITS_OFF)) == "0.0833333");
    std::cout << "convert(\"5.00s\",\"min\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00min","min",RpUnits::UNITS_ON)) == "5min");
    std::cout << "convert(\"5.00min\",\"min\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00min","min",RpUnits::UNITS_OFF)) == "5");
    std::cout << "convert(\"5.00min\",\"min\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00h","min",RpUnits::UNITS_ON)) == "300min");
    std::cout << "convert(\"5.00h\",\"min\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00h","min",RpUnits::UNITS_OFF)) == "300");
    std::cout << "convert(\"5.00h\",\"min\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00d","min",RpUnits::UNITS_ON)) == "7200min");
    std::cout << "convert(\"5.00d\",\"min\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00d","min",RpUnits::UNITS_OFF)) == "7200");
    std::cout << "convert(\"5.00d\",\"min\",UNITS_OFF) passed" << std::endl;

    assert((RpUnits::convert("5.00s","h",RpUnits::UNITS_ON)) == "0.00138889h");
    std::cout << "convert(\"5.00s\",\"h\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00s","h",RpUnits::UNITS_OFF)) == "0.00138889");
    std::cout << "convert(\"5.00s\",\"h\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00min","h",RpUnits::UNITS_ON)) == "0.0833333h");
    std::cout << "convert(\"5.00min\",\"h\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00min","h",RpUnits::UNITS_OFF)) == "0.0833333");
    std::cout << "convert(\"5.00min\",\"h\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00h","h",RpUnits::UNITS_ON)) == "5h");
    std::cout << "convert(\"5.00h\",\"h\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00h","h",RpUnits::UNITS_OFF)) == "5");
    std::cout << "convert(\"5.00h\",\"h\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00d","h",RpUnits::UNITS_ON)) == "120h");
    std::cout << "convert(\"5.00d\",\"h\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00d","h",RpUnits::UNITS_OFF)) == "120");
    std::cout << "convert(\"5.00d\",\"h\",UNITS_OFF) passed" << std::endl;

    assert((RpUnits::convert("5.00s","d",RpUnits::UNITS_ON)) == "5.78704e-05d");
    std::cout << "convert(\"5.00s\",\"d\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00s","d",RpUnits::UNITS_OFF)) == "5.78704e-05");
    std::cout << "convert(\"5.00s\",\"d\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00min","d",RpUnits::UNITS_ON)) == "0.00347222d");
    std::cout << "convert(\"5.00min\",\"d\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00min","d",RpUnits::UNITS_OFF)) == "0.00347222");
    std::cout << "convert(\"5.00min\",\"d\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00h","d",RpUnits::UNITS_ON)) == "0.208333d");
    std::cout << "convert(\"5.00h\",\"d\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00h","d",RpUnits::UNITS_OFF)) == "0.208333");
    std::cout << "convert(\"5.00h\",\"d\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00d","d",RpUnits::UNITS_ON)) == "5d");
    std::cout << "convert(\"5.00d\",\"d\",UNITS_ON)  passed" << std::endl;
    assert((RpUnits::convert("5.00d","d",RpUnits::UNITS_OFF)) == "5");
    std::cout << "convert(\"5.00d\",\"d\",UNITS_OFF) passed" << std::endl;



    assert((RpUnits::convert("5.00bar","Pa",RpUnits::UNITS_OFF)) == "500000");
    std::cout << "convert(\"5.00bar\",\"Pa\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00bar","Pa",RpUnits::UNITS_ON)) == "500000Pa");
    std::cout << "convert(\"5.00bar\",\"Pa\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00bar","atm",RpUnits::UNITS_OFF)) == "4.9346");
    std::cout << "convert(\"5.00bar\",\"atm\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00bar","atm",RpUnits::UNITS_ON)) == "4.9346atm");
    std::cout << "convert(\"5.00bar\",\"atm\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00bar","torr",RpUnits::UNITS_OFF)) == "3750.3");
    std::cout << "convert(\"5.00bar\",\"torr\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00bar","torr",RpUnits::UNITS_ON)) == "3750.3torr");
    std::cout << "convert(\"5.00bar\",\"torr\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00bar","psi",RpUnits::UNITS_OFF)) == "72.52");
    std::cout << "convert(\"5.00bar\",\"psi\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00bar","psi",RpUnits::UNITS_ON)) == "72.52psi");
    std::cout << "convert(\"5.00bar\",\"psi\",UNITS_ON) passed" << std::endl;




    assert((RpUnits::convert("5.00atm","bar",RpUnits::UNITS_OFF)) == "5.06627");
    std::cout << "convert(\"5.00atm\",\"bar\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00atm","bar",RpUnits::UNITS_ON)) == "5.06627bar");
    std::cout << "convert(\"5.00atm\",\"bar\",UNITS_ON) passed" << std::endl;
    /*
    assert((RpUnits::convert("5.00atm","Pa",RpUnits::UNITS_OFF)) == "506625.12");
    std::cout << "convert(\"5.00atm\",\"Pa\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00atm","Pa",RpUnits::UNITS_ON)) == "506625.12Pa");
    std::cout << "convert(\"5.00atm\",\"Pa\",UNITS_ON) passed" << std::endl;
    */
    assert((RpUnits::convert("5.00atm","torr",RpUnits::UNITS_OFF)) == "3800");
    std::cout << "convert(\"5.00atm\",\"torr\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00atm","torr",RpUnits::UNITS_ON)) == "3800torr");
    std::cout << "convert(\"5.00atm\",\"torr\",UNITS_ON) passed" << std::endl;




    assert((RpUnits::convert("5.00Pa","bar",RpUnits::UNITS_OFF)) == "5e-05");
    std::cout << "convert(\"5.00Pa\",\"bar\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00Pa","bar",RpUnits::UNITS_ON)) == "5e-05bar");
    std::cout << "convert(\"5.00Pa\",\"bar\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00Pa","atm",RpUnits::UNITS_OFF)) == "4.9346e-05");
    std::cout << "convert(\"5.00Pa\",\"atm\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00Pa","atm",RpUnits::UNITS_ON)) == "4.9346e-05atm");
    std::cout << "convert(\"5.00Pa\",\"atm\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00Pa","torr",RpUnits::UNITS_OFF)) == "0.037503");
    std::cout << "convert(\"5.00Pa\",\"torr\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00Pa","torr",RpUnits::UNITS_ON)) == "0.037503torr");
    std::cout << "convert(\"5.00Pa\",\"torr\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00Pa","psi",RpUnits::UNITS_OFF)) == "0.0007252");
    std::cout << "convert(\"5.00Pa\",\"psi\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00Pa","psi",RpUnits::UNITS_ON)) == "0.0007252psi");
    std::cout << "convert(\"5.00Pa\",\"psi\",UNITS_ON) passed" << std::endl;




    assert((RpUnits::convert("5.00torr","bar",RpUnits::UNITS_OFF)) == "0.00666613");
    std::cout << "convert(\"5.00torr\",\"bar\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00torr","bar",RpUnits::UNITS_ON)) == "0.00666613bar");
    std::cout << "convert(\"5.00torr\",\"bar\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00torr","Pa",RpUnits::UNITS_OFF)) == "666.613");
    std::cout << "convert(\"5.00torr\",\"Pa\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00torr","Pa",RpUnits::UNITS_ON)) == "666.613Pa");
    std::cout << "convert(\"5.00torr\",\"Pa\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00torr","atm",RpUnits::UNITS_OFF)) == "0.006579");
    std::cout << "convert(\"5.00torr\",\"atm\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00torr","atm",RpUnits::UNITS_ON)) == "0.006579atm");
    std::cout << "convert(\"5.00torr\",\"atm\",UNITS_ON) passed" << std::endl;
    assert((RpUnits::convert("5.00torr","psi",RpUnits::UNITS_OFF)) == "0.096685");
    std::cout << "convert(\"5.00torr\",\"psi\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00torr","psi",RpUnits::UNITS_ON)) == "0.096685psi");
    std::cout << "convert(\"5.00torr\",\"psi\",UNITS_ON) passed" << std::endl;


    assert((RpUnits::convert("5.00psi","bar",RpUnits::UNITS_OFF)) == "0.344738");
    std::cout << "convert(\"5.00psi\",\"bar\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00psi","bar",RpUnits::UNITS_ON)) == "0.344738bar");
    std::cout << "convert(\"5.00psi\",\"bar\",UNITS_ON) passed" << std::endl;
    /*
    assert((RpUnits::convert("5.00psi","Pa",RpUnits::UNITS_OFF)) == "34473.8129155");
    std::cout << "convert(\"5.00psi\",\"Pa\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00psi","Pa",RpUnits::UNITS_ON)) == "34473.8129155Pa");
    std::cout << "convert(\"5.00psi\",\"Pa\",UNITS_ON) passed" << std::endl;
    */
    assert((RpUnits::convert("5.00psi","torr",RpUnits::UNITS_OFF)) == "258.575");
    std::cout << "convert(\"5.00psi\",\"torr\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00psi","torr",RpUnits::UNITS_ON)) == "258.575torr");
    std::cout << "convert(\"5.00psi\",\"torr\",UNITS_ON) passed" << std::endl;




    assert((RpUnits::convert("5.00psi","kPa",RpUnits::UNITS_OFF)) == "34.4738");
    std::cout << "convert(\"5.00psi\",\"kPa\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00psi","kPa",RpUnits::UNITS_ON)) == "34.4738kPa");
    std::cout << "convert(\"5.00psi\",\"kPa\",UNITS_ON) passed" << std::endl;




    assert((RpUnits::convert("5.00torr","mmHg",RpUnits::UNITS_OFF)) == "5");
    std::cout << "convert(\"5.00torr\",\"mmHg\",UNITS_OFF) passed" << std::endl;
    assert((RpUnits::convert("5.00torr","mmHg",RpUnits::UNITS_ON)) == "5mmHg");
    std::cout << "convert(\"5.00torr\",\"mmHg\",UNITS_ON) passed" << std::endl;

    return 0;

}
