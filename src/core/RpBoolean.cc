/*
 * ======================================================================
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
 #ifndef _RpBOOLEAN_H
     #include "RpBoolean.h"
 #endif

/************************************************************************
 *                                                                      
 * set the default value of this object
 *                                                                      
 ************************************************************************/

RpBoolean& 
RpBoolean::setDefaultValue(std::string newDefaultVal)
{
    std::string* def = NULL;
    
    def = (std::string*) RpVariable::getDefaultValue(); 

    if (!def) {
        RpVariable::setDefaultValue(new std::string (newDefaultVal));
    }
    else {
        *def = newDefaultVal;
    }

    return *this;
}

/************************************************************************
 *                                                                      
 * set the current value of this object
 *                                                                      
 ************************************************************************/

RpBoolean& 
RpBoolean::setCurrentValue(std::string newCurrentVal)
{
    std::string* cur = (std::string*) RpVariable::getCurrentValue();
    std::string* def = (std::string*) RpVariable::getDefaultValue();

    if (cur == def) {
        RpVariable::setCurrentValue(new std::string (newCurrentVal));
    }
    else {
        *cur = newCurrentVal;
    }

    return *this;
}


/************************************************************************
 *                                                                      
 * report the default value of this object 
 *                                                                      
 ************************************************************************/
std::string
RpBoolean::getDefaultValue(void* null_val) const
{
    return *((std::string*) RpVariable::getDefaultValue()); 
}

/************************************************************************
 *                                                                      
 * report the current value of this object 
 *                                                                      
 ************************************************************************/
std::string
RpBoolean::getCurrentValue(void* null_val) const
{
    return *((std::string*) RpVariable::getCurrentValue()); 
}

// -------------------------------------------------------------------- //

