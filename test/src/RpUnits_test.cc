#include "RpUnits.h"


void success() 
{
    printf ("Test Successful\n");
}

void fail() 
{
    printf ("Test FAILED !!!!!!!!!!!!!!!!!\n");
    exit(-1);
}


int main()
{
    double value = 0.0;
    // int failTest = 0;
    int result = 0;
    // std::list<double,RpUnits *> 

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
    RpUnits * meters = RpUnits::define("m", NULL);
    // (meters) ? success() : fail();
    // RpUnits * centimeter = RpUnits::define("cm", NULL);

    RpUnits::makeMetric(meters);
    RpUnits::define("V", NULL);
    RpUnits::define("s", NULL);

/*
    std::string srch_str = std::string("cm");

    RpUnits* meters2 = RpUnits::find("cm");
    if (meters2) { 
        std::cout << "meters2 exists" << std::endl;
        std::cout << "meters2 = :" << meters2->getUnitsName() <<":"<< std::endl;
    }

    std::cout << "complete"<< std::endl;

*/
    
    RpUnits* mobility = RpUnits::define("cm2/Vs", NULL);
    std::cout << "mobility = :" << mobility->getUnitsName() <<":"<< std::endl;

    RpUnits* mobility2 = RpUnits::find("cm2V-1s-1");
    if (mobility2) { 
        std::cout << "mobility2 exists" << std::endl;
        std::cout << "mobility2 = :" << mobility2->getUnits() <<":"<< std::endl;
        std::cout << "mobility2 = :" << mobility2->getUnitsName() <<":"<< std::endl;
    }
    else {
        std::cout << "mobility2 dn exists" << std::endl;
    }

    RpUnits* cmeters = RpUnits::find("cm");
    RpUnits* angstrom = RpUnits::define("A", NULL);
    RpUnits::define(angstrom, meters, angstrom2meter, meter2angstrom);

    value = angstrom->convert(meters,1.0,&result);
    std::cout << "1 angstrom = " << value << " meters" << std::endl;

    result = 0;
    value = cmeters->convert(angstrom,1e-8,&result);
    std::cout << "1.0e-8 centimeter = " << value << " angstroms" << std::endl;


    RpUnits* fahrenheit  = RpUnits::define("F", NULL);
    RpUnits* celcius  = RpUnits::define("C", NULL);
    RpUnits* kelvin  = RpUnits::define("K", NULL);
    
    RpUnits::define(fahrenheit, celcius, fahrenheit2centigrade, centigrade2fahrenheit);
    RpUnits::define(celcius, kelvin, centigrade2kelvin, kelvin2centigrade);
    
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

    RpUnits * millimeter = RpUnits::find("mm");
    RpUnits * micrometer = RpUnits::find("um");
    RpUnits * nanometer  = RpUnits::find("nm");
    RpUnits * picometer  = RpUnits::find("pm");
    RpUnits * femtometer = RpUnits::find("fm");
    RpUnits * attometer  = RpUnits::find("am");
    RpUnits * kilometer  = RpUnits::find("km");
    RpUnits * megameter  = RpUnits::find("Mm");
    RpUnits * gigameter  = RpUnits::find("Gm");
    RpUnits * terameter  = RpUnits::find("Tm");
    RpUnits * petameter  = RpUnits::find("Pm");

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


    return 0;
}
