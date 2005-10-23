/*
 * ======================================================================
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
 #ifndef _RpSTRING_H
     #include "RpString.h"
 #endif

/************************************************************************
 *                                                                      
 * set the default value of this object
 *                                                                      
 ************************************************************************/

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

/************************************************************************
 *                                                                      
 * set the current value of this object
 *                                                                      
 ************************************************************************/

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


/************************************************************************
 *                                                                      
 * set the size (width and height) of this object
 *                                                                      
 ************************************************************************/
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

/************************************************************************
 *                                                                      
 * set the width of this object
 *                                                                      
 ************************************************************************/
RpString& 
RpString::setWidth(int newWidth)
{
    this->width = newWidth;
    return *this;
}

/************************************************************************
 *                                                                      
 * set the height of this object
 *                                                                      
 ************************************************************************/
RpString& 
RpString::setHeight(int newHeight)
{
    this->height = newHeight;
    return *this;
}


/************************************************************************
 *                                                                      
 * report the default value of this object 
 *                                                                      
 ************************************************************************/
std::string
RpString::getDefaultValue(void* null_val) const
{
    return *((std::string*) RpVariable::getDefaultValue()); 
}

/************************************************************************
 *                                                                      
 * report the current value of this object 
 *                                                                      
 ************************************************************************/
std::string
RpString::getCurrentValue(void* null_val) const
{
    return *((std::string*) RpVariable::getCurrentValue()); 
}

/************************************************************************
 *                                                                      
 * report the size of this object  in the form  Height x Width
 *                                                                      
 ************************************************************************/
std::string
RpString::getSize() const
{
    std::stringstream tmpStr;
    
    tmpStr << getWidth() << "x" << getHeight();
    return (tmpStr.str()); 
}

/************************************************************************
 *                                                                      
 * report the Height of this object 
 *                                                                      
 ************************************************************************/
int
RpString::getHeight() const
{
    return height; 
}

/************************************************************************
 *                                                                      
 * report the Width of this object 
 *                                                                      
 ************************************************************************/
int
RpString::getWidth() const
{
    return width; 
}

// -------------------------------------------------------------------- //

