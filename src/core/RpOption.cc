/*
 * ======================================================================
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
 #ifndef _RpOPTION_H
     #include "RpOption.h"
 #endif

/************************************************************************
 *                                                                      
 * set the label of this object
 *                                                                      
 ************************************************************************/

RpOption&
RpOption::setLabel(std::string newLabel)
{
    about.setLabel(newLabel);
    return *this; 
}


/************************************************************************
 *                                                                      
 * report the label of this object 
 *                                                                      
 ************************************************************************/
std::string 
RpOption::getLabel() const
{
    return about.getLabel(); 
}

// -------------------------------------------------------------------- //

