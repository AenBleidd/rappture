
 #ifndef _RpVARIABLE_H
     #include "RpVariable.h"
 #endif

/************************************************************************
 *                                                                      
 * set the path of this object 
 *                                                                      
 ************************************************************************/

RpVariable&
RpVariable::setPath(std::string newPath)
{
    path = newPath;
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the default value of this object 
 *                                                                      
 ************************************************************************/

RpVariable&
RpVariable::setDefaultValue(void* newDefaultVal)
{
    // who is responsible for freeing the pointer?
    defaultVal = newDefaultVal; 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the current value of this object 
 *                                                                      
 ************************************************************************/

RpVariable&
RpVariable::setCurrentValue(void* newCurrentVal)
{
    // who is responsible for freeing the pointer?
    currentVal = newCurrentVal; 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the label of this object 
 *                                                                      
 ************************************************************************/

RpVariable&
RpVariable::setLabel(std::string newLabel)
{
    about.setLabel(newLabel); 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the desc of this object 
 *                                                                      
 ************************************************************************/

RpVariable& 
RpVariable::setDesc(std::string newDesc)
{
    about.setDesc(newDesc); 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the hints of this object 
 *                                                                      
 ************************************************************************/

RpVariable& 
RpVariable::setHints(std::string newHints)
{
    about.setHints(newHints); 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the color of this object 
 *                                                                      
 ************************************************************************/

RpVariable& 
RpVariable::setColor(std::string newColor)
{
    about.setColor(newColor); 
    return *this; 
}

/************************************************************************
 *                                                                      
 * set the icon of this object 
 *                                                                      
 ************************************************************************/

RpVariable& 
RpVariable::setIcon(std::string newIcon)
{
    about.setIcon(newIcon); 
    return *this; 
}


/************************************************************************
 *                                                                      
 * report the path of the object 
 *                                                                      
 ************************************************************************/
std::string 
RpVariable::getPath() const
{
    return path; 
}

/************************************************************************
 *                                                                      
 * report the default value of the object 
 *                                                                      
 ************************************************************************/
void*
RpVariable::getDefaultValue() const
{
    return defaultVal; 
}

/************************************************************************
 *                                                                      
 * report the current value of the object 
 *                                                                      
 ************************************************************************/
void* 
RpVariable::getCurrentValue() const
{
    return currentVal; 
}

/************************************************************************
 *                                                                      
 * report the label of the object 
 *                                                                      
 ************************************************************************/
std::string 
RpVariable::getLabel() const
{
    return about.getLabel(); 
}

/************************************************************************
 *                                                                      
 * report the desc of the object 
 *                                                                      
 ************************************************************************/
std::string 
RpVariable::getDesc() const
{
    return about.getDesc(); 
}

/************************************************************************
 *                                                                      
 * report the hints of this object 
 *                                                                      
 ************************************************************************/
std::string 
RpVariable::getHints() const
{
    return about.getHints(); 
}

/************************************************************************
 *                                                                      
 * report the color of this object 
 *                                                                      
 ************************************************************************/
std::string 
RpVariable::getColor() const
{
    return about.getColor(); 
}

/************************************************************************
 *                                                                      
 * report the icon of this object 
 *                                                                      
 ************************************************************************/
std::string 
RpVariable::getIcon() const
{
    return about.getIcon(); 
}


// -------------------------------------------------------------------- //

