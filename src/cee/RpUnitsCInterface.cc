#include "../include/RpUnits.h"
#include "../include/RpUnitsCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    RpUnits* defineUnit(const char* unitSymbol, RpUnits* basis)
    {
        return RpUnits::define(unitSymbol, basis);
    }

    RpUnits* defineConv(    RpUnits* fromUnit,
                            RpUnits* toUnit,
                            double (*convForwFxnPtr)(double),
                            double (*convBackFxnPtr)(double)    )
    {
        return RpUnits::define(fromUnit,toUnit,convForwFxnPtr,convBackFxnPtr);
    }

    RpUnits* find(const char* key) 
    {
        return RpUnits::find(key);
    }

    const char* getUnits(RpUnits* unit)
    {
        return unit->getUnits().c_str();
    }

    const char* getUnitsName(RpUnits* unit)
    {
        return unit->getUnitsName().c_str();
    }

    double getExponent(RpUnits* unit)
    {
        return unit->getExponent();
    }

    RpUnits* getBasis(RpUnits* unit)
    {
        return unit->getBasis();
    }

    int makeMetric(RpUnits* basis)
    {
        return RpUnits::makeMetric(basis);
    }

    const char* convert_str(    RpUnits* fromUnits, 
                                RpUnits* toUnits, 
                                double val,
                                int showUnits   )
    {
        return convert_str_result(fromUnits,toUnits,val,showUnits,NULL);
    }
    
    const char* convert_str_result( RpUnits* fromUnits, 
                                    RpUnits* toUnits, 
                                    double val, 
                                    int showUnits,
                                    int* result )
    {
        return fromUnits->convert(toUnits,val,showUnits,result).c_str();
    }

    double convert_double(RpUnits* fromUnits, RpUnits* toUnits, double val)
    {
        return convert_double_result(fromUnits,toUnits,val,NULL);
    }

    double convert_double_result(   RpUnits* fromUnits, 
                                    RpUnits* toUnits, 
                                    double val, 
                                    int* result )
    {
        return fromUnits->convert(toUnits,val,result);
    }

#ifdef __cplusplus
}
#endif
