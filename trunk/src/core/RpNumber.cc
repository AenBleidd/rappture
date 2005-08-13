 #ifndef _RpNUMBER_H
     #include "RpNumber.h"
 #endif

/************************************************************************
 *                                                                      
 * set the default value of a RpNumber object
 *                                                                      
 ************************************************************************/

RpNumber& 
RpNumber::setDefaultValue(double newDefaultVal)
{
    double* def = NULL;
    
    def = (double*) RpVariable::getDefaultValue(); 

    if (!def) {
        RpVariable::setDefaultValue(new double (newDefaultVal));
    }
    else {
        *def = newDefaultVal;
    }

    return *this;
}

/************************************************************************
 *                                                                      
 * set the current value of a RpNumber object
 *                                                                      
 ************************************************************************/

RpNumber& 
RpNumber::setCurrentValue(double newCurrentVal)
{
    double* cur = (double*) RpVariable::getCurrentValue();
    double* def = (double*) RpVariable::getDefaultValue();

    if (cur == def) {
        RpVariable::setCurrentValue(new double (newCurrentVal));
    }
    else {
        *cur = newCurrentVal;
    }

    return *this;
}


/************************************************************************
 *                                                                      
 * set the min value of a RpNumber object
 *                                                                      
 ************************************************************************/
RpNumber& 
RpNumber::setMin(double newMin)
{
    min = newMin;
    return *this;
}
/************************************************************************
 *                                                                      
 * set the min value of a RpNumber object
 *                                                                      
 ************************************************************************/
RpNumber& 
RpNumber::setMax(double newMax)
{
    max = newMax;
    return *this;
}
/************************************************************************
 *                                                                      
 * report the units of the object 
 *                                                                      
 ************************************************************************/

std::string 
RpNumber::getUnits() const
{
    return units->getUnitsName(); 
}


/************************************************************************
 *                                                                      
 * report the default value of the object 
 *                                                                      
 ************************************************************************/
double
RpNumber::getDefaultValue(void* null_val) const
{
    return *((double*) RpVariable::getDefaultValue()); 
}

/************************************************************************
 *                                                                      
 * report the current value of the object 
 *                                                                      
 ************************************************************************/
double
RpNumber::getCurrentValue(void* null_val) const
{
    return *((double*) RpVariable::getCurrentValue()); 
}

/************************************************************************
 *                                                                      
 * report the min of the object 
 *                                                                      
 ************************************************************************/
double
RpNumber::getMin() const
{
    return min; 
}

/************************************************************************
 *                                                                      
 * report the max of the object 
 *                                                                      
 ************************************************************************/
double
RpNumber::getMax() const
{
    return max; 
}

/************************************************************************
 *                                                                      
 * convert the number object to another unit from string
 * store the result as the currentValue
 *                                                                      
 ************************************************************************/
double
RpNumber::convert(std::string toUnitStr, int storeResult, int* result)
{
    RpUnits * toUnit = NULL;
    double retVal = 0;
    int my_result = 0;

    toUnit = RpUnits::find(toUnitStr);

    // set the result to the default value if it exists
    if (result) {
        *result = my_result;
    }

    if (!toUnit) {
        // should raise error! 
        // conversion not defined because unit does not exist
        return retVal;
    }

    // perform the conversion
    retVal = convert(toUnit,&my_result); 
    
    // check the result of the conversion and store if necessary
    if (my_result) {
        if (result) {
            *result = my_result;
        }

        // check if we should store the value and change units
        if (storeResult) {
            // no need to deallocate old units, 
            // because they are stored in static dictionary
            units = toUnit;
            RpNumber::setCurrentValue(retVal);
        }
    }

    return retVal;
}

/************************************************************************
 *                                                                      
 * convert the number object to another unit from string
 *                                                                      
 ************************************************************************/
double
RpNumber::convert(std::string toUnitStr, int* result)
{
    RpUnits * toUnit = NULL;
    toUnit = RpUnits::find(toUnitStr);
    if (!toUnit) {
        // should raise error! 
        // conversion not defined because unit does not exist
        if (result) {
            *result = -1;
        }
        return 0.0;
    }
    return convert(toUnit, result); 
}

/************************************************************************
 *                                                                      
 * convert the number object to another unit from RpUnits object
 *                                                                      
 ************************************************************************/
double
RpNumber::convert(RpUnits* toUnit, int *result)
{
    return units->convert(toUnit,getCurrentValue(), result); 
}


// -------------------------------------------------------------------- //

