#include "RpUnits.h"
#include "RpUnitsCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

    RpUnits* defineUnit(const char* unitSymbol, RpUnits* basis) {
        
        return RpUnits::define(unitSymbol, basis);
    }

    RpUnits* defineConv(    RpUnits* fromUnit,
                            RpUnits* toUnit,
                            double (*convForwFxnPtr)(double),
                            double (*convBackFxnPtr)(double)    ) {

        return RpUnits::define(fromUnit,toUnit,convForwFxnPtr,convBackFxnPtr);
    }

    RpUnits* find(const char* key) {
        
        return RpUnits::find(key);
    }

    const char* getUnits(RpUnits* unit) {
        
        static std::string retVal;
        retVal = unit->getUnits();
        return retVal.c_str();
    }

    const char* getUnitsName(RpUnits* unit) {
        
        static std::string retVal;
        retVal = unit->getUnitsName();
        return retVal.c_str();
    }

    double getExponent(RpUnits* unit) {

        return unit->getExponent();
    }

    RpUnits* getBasis(RpUnits* unit) {
        
        return unit->getBasis();
    }

    int makeMetric(RpUnits* basis) {

        return RpUnits::makeMetric(basis);
    }

    const char* convert (   const char* fromVal,
                            const char* toUnitsName,
                            int showUnits,
                            int* result ) {

        static std::string retVal;
        retVal = RpUnits::convert(fromVal,toUnitsName,showUnits,result);
        return retVal.c_str();
    }

    const char* convert_str (   const char* fromVal,
                                const char* toUnitsName,
                                int showUnits,
                                int* result ) {

        static std::string retVal;
        retVal = RpUnits::convert(fromVal,toUnitsName,showUnits,result);
        return retVal.c_str();
    }

    const char* convert_obj_str(    RpUnits* fromUnits, 
                                    RpUnits* toUnits, 
                                    double val,
                                    int showUnits   ) {

        return convert_obj_str_result(fromUnits,toUnits,val,showUnits,NULL);
    }
    
    const char* convert_obj_str_result( RpUnits* fromUnits, 
                                        RpUnits* toUnits, 
                                        double val, 
                                        int showUnits,
                                        int* result ) {
        
        static std::string retVal;
        retVal = fromUnits->convert(toUnits,val,showUnits,result);
        return retVal.c_str();
    }

    double convert_dbl (    const char* fromVal,
                            const char* toUnitsName,
                            int* result ) {

        std::string convStr;
        double retVal = 0.0;

        convStr = RpUnits::convert(fromVal,toUnitsName,0,result);

        if (!convStr.empty()) {
            retVal = atof(convStr.c_str());
        }

        return retVal;
    }

    double convert_obj_double(  RpUnits* fromUnits, 
                                RpUnits* toUnits, 
                                double val  ) {

        return convert_obj_double_result(fromUnits,toUnits,val,NULL);
    }

    double convert_obj_double_result(   RpUnits* fromUnits, 
                                        RpUnits* toUnits, 
                                        double val, 
                                        int* result ) {
        
        return fromUnits->convert(toUnits,val,result);
    }

    int add_presets ( const char* presetName ) {

        return RpUnits::addPresets(presetName);
    }

#ifdef __cplusplus
}
#endif
