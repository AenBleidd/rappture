
 #ifndef _RpABOUT_H
     #include "RpAbout.h"
 #endif

/************************************************************************
 *                                                                      
 * set the label of this object
 *                                                                      
 ************************************************************************/

RpAbout&
RpAbout::setLabel(std::string newLabel)
{
    label = newLabel;
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the description of this object 
 *                                                                      
 ************************************************************************/

RpAbout&
RpAbout::setDesc(std::string newDesc)
{
    desc = newDesc; 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the hints of this object
 *                                                                      
 ************************************************************************/

RpAbout&
RpAbout::setHints(std::string newHints)
{
    hints = newHints; 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the color of this object
 *                                                                      
 ************************************************************************/

RpAbout&
RpAbout::setColor(std::string newColor)
{
    color = newColor; 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the icon of this object
 *                                                                      
 ************************************************************************/

RpAbout& 
RpAbout::setIcon(std::string newIcon)
{
    icon = newIcon; 
    return *this; 
}


/************************************************************************
 *                                                                      
 * report the label of this object 
 *                                                                      
 ************************************************************************/
std::string 
RpAbout::getLabel() const
{
    return label; 
}

/************************************************************************
 *                                                                      
 * report the description of this object
 *                                                                      
 ************************************************************************/
std::string
RpAbout::getDesc() const
{
    return desc; 
}

/************************************************************************
 *                                                                      
 * report the hints of this object
 *                                                                      
 ************************************************************************/
std::string
RpAbout::getHints() const
{
    return hints; 
}

/************************************************************************
 *                                                                      
 * report the color of this object 
 *                                                                      
 ************************************************************************/
std::string 
RpAbout::getColor() const
{
    return color; 
}

/************************************************************************
 *                                                                      
 * report the icon of this object 
 *                                                                      
 ************************************************************************/
std::string 
RpAbout::getIcon() const
{
    return icon; 
}


// -------------------------------------------------------------------- //

