/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Option Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpOption.h"

/**********************************************************************/
// METHOD: setLabel()
/// Set the label of this object.
/**
 */

RpOption&
RpOption::setLabel(std::string newLabel)
{
    about.setLabel(newLabel);
    return *this; 
}


/**********************************************************************/
// METHOD: getLabel()
/// Report the label of this object.
/**
 */

std::string 
RpOption::getLabel() const
{
    return about.getLabel(); 
}

// -------------------------------------------------------------------- //

