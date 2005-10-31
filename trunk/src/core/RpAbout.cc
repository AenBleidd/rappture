/*
 * ----------------------------------------------------------------------
 *  RpAbout - Rappture 2.0 About XML object
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

 #include "RpAbout.h"

/**********************************************************************/
// METHOD: setLabel()
/// Set the label of this object.
/**
 */

RpAbout&
RpAbout::setLabel(std::string newLabel) {

    label = newLabel;
    return *this;
}

/**********************************************************************/
// METHOD: setDesc()
/// Set the description of this object.
/**
 */

RpAbout&
RpAbout::setDesc(std::string newDesc) {

    desc = newDesc;
    return *this;
}

/**********************************************************************/
// METHOD: setHints()
/// Set the hints of this object.
/**
 */

RpAbout&
RpAbout::setHints(std::string newHints) {

    hints = newHints;
    return *this;
}

/**********************************************************************/
// METHOD: setColor()
/// Set the color of this object.
/**
 */

RpAbout&
RpAbout::setColor(std::string newColor) {

    color = newColor;
    return *this;
}

/**********************************************************************/
// METHOD: setIcon()
/// Set the icon of this object.
/**
 */

RpAbout&
RpAbout::setIcon(std::string newIcon) {

    icon = newIcon;
    return *this;
}


/**********************************************************************/
// METHOD: getLabel()
/// Report the label of this object.
/**
 */

std::string
RpAbout::getLabel() const {

    return label;
}

/**********************************************************************/
// METHOD: getDesc()
/// Report the description of this object.
/**
 */

std::string
RpAbout::getDesc() const {

    return desc;
}

/**********************************************************************/
// METHOD: getHints()
/// Report the hints of this object.
/**
 */

std::string
RpAbout::getHints() const {

    return hints;
}

/**********************************************************************/
// METHOD: getColor()
/// Report the color of this object.
/**
 */

std::string
RpAbout::getColor() const {

    return color;
}

/**********************************************************************/
// METHOD: getIcon()
/// Report the icon of this object.
/**
 */

std::string
RpAbout::getIcon() const {

    return icon;
}


// -------------------------------------------------------------------- //
