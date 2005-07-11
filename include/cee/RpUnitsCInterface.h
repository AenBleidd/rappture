

#ifdef __cplusplus
extern "C" {
#endif 

    typedef struct RpUnits RpUnits;

    // unit definition functions
    RpUnits* defineUnit(const char* unitSymbol, RpUnits* basis);

    // conversion definition functions
    RpUnits* defineConv(    RpUnits* fromUnit,
                            RpUnits* toUnit,
                            double (*convForwFxnPtr)(double),
                            double (*convBackFxnPtr)(double)    );

    // unit attribute access functions
    const char* getUnits(RpUnits* unit);

    const char* getUnitsName(RpUnits* unit);
    
    double getExponent(RpUnits* unit);
    
    RpUnits* getBasis(RpUnits* unit);

    RpUnits* find(const char* unitSymbol);

    int makeMetric(RpUnits* basis);

    // convert functions 
    const char* convert_str(    RpUnits* fromUnits, 
                                RpUnits* toUnits, 
                                double val,
                                int showUnits   );
    
    const char* convert_str_result( RpUnits* fromUnits, 
                                    RpUnits* toUnits, 
                                    double val, 
                                    int showUnits,
                                    int* result );

    double convert_double(RpUnits* fromUnits, RpUnits* toUnits, double val);

    double convert_double_result(   RpUnits* fromUnits, 
                                    RpUnits* toUnits, 
                                    double val, 
                                    int* result );


#ifdef __cplusplus
}
#endif
