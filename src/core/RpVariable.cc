/*
 * ----------------------------------------------------------------------
 *  RpVariable.cc
 *
 *   Rappture 2.0 Variable member functions
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

 #include "RpVariable.h"

/**********************************************************************/
// METHOD: setPath()
/// Set the path of this object
/**
 */

RpVariable&
RpVariable::setPath(std::string newPath)
{
    path = newPath;
    return *this; 
}

/**********************************************************************/
// METHOD: setDefaultValue()
/// Set the default value of this object
/**
 */

RpVariable&
RpVariable::setDefaultValue(void* newDefaultVal)
{
    // who is responsible for freeing the pointer?
    defaultVal = newDefaultVal; 
    return *this; 
}

/**********************************************************************/
// METHOD: setCurrentValue()
/// Set the current value of this object
/**
 */

RpVariable&
RpVariable::setCurrentValue(void* newCurrentVal)
{
    // who is responsible for freeing the pointer?
    currentVal = newCurrentVal; 
    return *this; 
}

/**********************************************************************/
// METHOD: setLabel()
/// Set the label of this object
/**
 */

RpVariable&
RpVariable::setLabel(std::string newLabel)
{
    about.setLabel(newLabel); 
    return *this; 
}

/**********************************************************************/
// METHOD: setDesc()
/// Set the desc of this object
/**
 */

RpVariable& 
RpVariable::setDesc(std::string newDesc)
{
    about.setDesc(newDesc); 
    return *this; 
}

/**********************************************************************/
// METHOD: setHints()
/// Set the hints of this object
/**
 */

RpVariable& 
RpVariable::setHints(std::string newHints)
{
    about.setHints(newHints); 
    return *this; 
}

/**********************************************************************/
// METHOD: setColor()
/// Set the color of this object
/**
 */

RpVariable& 
RpVariable::setColor(std::string newColor)
{
    about.setColor(newColor); 
    return *this; 
}

/**********************************************************************/
// METHOD: setIcon()
/// Set the icon of this object 
/**
 */

RpVariable& 
RpVariable::setIcon(std::string newIcon)
{
    about.setIcon(newIcon); 
    return *this; 
}


/**********************************************************************/
// METHOD: getPath()
/// Report the path of this object
/**
 */

std::string 
RpVariable::getPath() const
{
    return path; 
}

/**********************************************************************/
// METHOD: getDefaultValue()
/// Report the default value of the object
/**
 */

void*
RpVariable::getDefaultValue() const
{
    return defaultVal; 
}

/**********************************************************************/
// METHOD: getCurrentValue()
/// Report the current value of the object
/**
 */

void* 
RpVariable::getCurrentValue() const
{
    return currentVal; 
}

/**********************************************************************/
// METHOD: getLabel()
/// Report the label of the object
/**
 */

std::string 
RpVariable::getLabel() const
{
    return about.getLabel(); 
}

/**********************************************************************/
// METHOD: getDesc()
/// Report the desc of the object
/**
 */

std::string 
RpVariable::getDesc() const
{
    return about.getDesc(); 
}

/**********************************************************************/
// METHOD: getHints()
/// Report the hints of this object
/**
 */

std::string 
RpVariable::getHints() const
{
    return about.getHints(); 
}

/**********************************************************************/
// METHOD: getColor()
/// Report the color of this object
/**
 */

std::string 
RpVariable::getColor() const
{
    return about.getColor(); 
}

/**********************************************************************/
// METHOD: getIcon()
/// Report the icon of this object
/**
 */

std::string 
RpVariable::getIcon() const
{
    return about.getIcon(); 
}


// -------------------------------------------------------------------- //

