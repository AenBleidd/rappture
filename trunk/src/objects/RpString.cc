/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 String Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpString.h"

/**********************************************************************/
// METHOD: setDefaultValue()
/// set the default value of this object.
/**
 */

RpString& 
RpString::setDefaultValue(std::string newDefaultVal)
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

/**********************************************************************/
// METHOD: setCurrentValue()
/// set the current value of this object
/**
 */

RpString& 
RpString::setCurrentValue(std::string newCurrentVal)
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

/**********************************************************************/
// METHOD: setSize()
/// set the size (width and height) of this object.
/**
 */

RpString& 
RpString::setSize(std::string sizeWxH)
{
    unsigned int xloc = sizeWxH.find("x");

    if (xloc == std::string::npos) {
        // sizeWxH is of incorrect format
        // raise error!

    }
    else {           
        this->width = atoi(sizeWxH.substr(0,xloc).c_str());
        this->height = atoi(sizeWxH.substr(xloc).c_str());
    }

    return *this;
}

/**********************************************************************/
// METHOD: setWidth()
/// set the width of this object
/**
 */

RpString& 
RpString::setWidth(int newWidth)
{
    this->width = newWidth;
    return *this;
}

/**********************************************************************/
// METHOD: setHeight()
/// set the height of this object.
/**
 */

RpString& 
RpString::setHeight(int newHeight)
{
    this->height = newHeight;
    return *this;
}


/**********************************************************************/
// METHOD: getDefaultValue()
/// report the default value of this object.
/**
 */

std::string
RpString::getDefaultValue(void* null_val) const
{
    return *((std::string*) RpVariable::getDefaultValue()); 
}

/**********************************************************************/
// METHOD: getCurrentValue()
/// report the current value of this object.
/**
 */

std::string
RpString::getCurrentValue(void* null_val) const
{
    return *((std::string*) RpVariable::getCurrentValue()); 
}

/**********************************************************************/
// METHOD: getSize()
/// report the size of this object  in the form  Height x Width
/**
 */

std::string
RpString::getSize() const
{
    std::stringstream tmpStr;

    tmpStr << getWidth() << "x" << getHeight();
    return (tmpStr.str()); 
}

/**********************************************************************/
// METHOD: getHeight()
/// report the Height of this object.
/**
 */

int
RpString::getHeight() const
{
    return height; 
}

/**********************************************************************/
// METHOD: getWidth()
/// report the Width of this object.
/**
 */

int
RpString::getWidth() const
{
    return width; 
}

// -------------------------------------------------------------------- //

