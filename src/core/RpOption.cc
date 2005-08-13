
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

