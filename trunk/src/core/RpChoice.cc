/*
 * ======================================================================
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
 #ifndef _RpCHOICE_H
     #include "RpChoice.h"
 #endif

/************************************************************************
 *                                                                      
 * set the default value of a RpChoice object
 *                                                                      
 ************************************************************************/

RpChoice& 
RpChoice::setDefaultValue(std::string newDefaultVal)
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
 * set the current value of a RpChoice object
 *                                                                      
 ************************************************************************/

RpChoice& 
RpChoice::setCurrentValue(std::string newCurrentVal)
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
 * set an option for this object
 *                                                                      
 ************************************************************************/

RpChoice&
RpChoice::setOption(std::string newOption)
{
    options.push_back(RpOption(newOption));
    return *this;
}

/************************************************************************
 *                                                                      
 * delete the provided option from this object
 *                                                                      
 ************************************************************************/

RpChoice& 
RpChoice::deleteOption(std::string optionName)
{
    int lvc = 0;
    int vsize = options.size();
    // std::vector<RpOption>::iterator tempIterator;
    /*
    for (lvc=0,tempIterator=options.begin();lvc<vsize;lvc++,tempIterator++) {
        if (option[lvc] == option) {
            option.erase(tempIterator);
            break;
        }
    }
    */
    for (lvc = 0;lvc < vsize; lvc++) {
        if (options[lvc].getLabel() == optionName) {
            options.erase(options.begin()+lvc);
            break;
        }
    } 
    return *this;
}

/************************************************************************
 *                                                                      
 * report the default value of this object 
 *                                                                      
 ************************************************************************/

std::string
RpChoice::getDefaultValue(void* null_val) const
{
    return *((std::string*) RpVariable::getDefaultValue()); 
}

/************************************************************************
 *                                                                      
 * report the current value of the object 
 *                                                                      
 ************************************************************************/

std::string
RpChoice::getCurrentValue(void* null_val) const
{
    return *((std::string*) RpVariable::getCurrentValue()); 
}


/************************************************************************
 *                                                                      
 * report the options of the object 
 *                                                                      
 ************************************************************************/

std::string
RpChoice::getFirstOption()
{
    optionsIter = options.begin();
    return ((*optionsIter).getLabel()); 
}

/************************************************************************
 *                                                                      
 * report the next options of the object 
 *
 * if you get to the end of the options list, "" is returned.
 *                                                                      
 ************************************************************************/

std::string
RpChoice::getNextOption()
{
    optionsIter++;
    if (optionsIter == options.end()) {
        return std::string("");
    }
    return ((*optionsIter).getLabel()); 
}

/************************************************************************
 *                                                                      
 * report the number of options (items) in this object 
 *
 ************************************************************************/

unsigned int
RpChoice::getOptListSize() const
{
    return options.size(); 
}

// -------------------------------------------------------------------- //
