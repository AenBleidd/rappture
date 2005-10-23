/*
 * ======================================================================
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <errno.h>


// #include "RpLibrary.h"

#ifndef _RpVARIABLE_H
    #include "RpVariable.h"
#endif

#ifndef _RpBOOLEAN_H 
#define _RpBOOLEAN_H

class RpBoolean : public RpVariable
{
    public:
        
        // users member fxns

        virtual RpBoolean& setDefaultValue   (std::string newDefaultVal);
        virtual RpBoolean& setCurrentValue   (std::string newCurrentVal);

        // base class makes 
        // these functions virtual and derived class has a different
        // return type. compiler doesnt like this. need to find another
        // way around this
        //
        // if we keep the null_val=NULL will that give us undefined behavior?
        //
        std::string getDefaultValue         (void* null_val=NULL) const;
        std::string getCurrentValue         (void* null_val=NULL) const;

        // place the information from this object into the xml library 'lib'
        // virtual RpBoolean& put(RpLibrary lib);
        // RpBoolean& put() const;

        RpBoolean (
                    std::string path, 
                    std::string defaultVal
                )
            :   RpVariable  (path, new std::string (defaultVal) )
        { }

        RpBoolean (
                    std::string path, 
                    std::string defaultVal,
                    std::string label,
                    std::string desc
                )
            :   RpVariable  (path, new std::string (defaultVal), label, desc)
        { }
        
        // copy constructor
        RpBoolean ( const RpBoolean& myRpBoolean )
            :   RpVariable(myRpBoolean)
        {}

        // default destructor
        virtual ~RpBoolean ()
        {
            // clean up dynamic memory
        }

    private:

};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
