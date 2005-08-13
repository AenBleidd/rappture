#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <errno.h>


// #include "RpLibrary.h"

#ifndef _RpVARIABLE_H
    #include "RpVariable.h"
#endif

#ifndef _RpSTRING_H 
#define _RpSTRING_H

class RpString : public RpVariable
{
    public:
        
        // users member fxns

        virtual RpString& setDefaultValue   (std::string newDefaultVal);
        virtual RpString& setCurrentValue   (std::string newCurrentVal);
        virtual RpString& setSize           (std::string newSize);
        virtual RpString& setHeight         (int newHeight);
        virtual RpString& setWidth          (int newWidth);

        // base class makes 
        // these functions virtual and derived class has a different
        // return type. compiler doesnt like this. need to find another
        // way around this
        //
        // if we keep the null_val=NULL will that give us undefined behavior?
        //
        std::string getDefaultValue         (void* null_val=NULL) const;
        std::string getCurrentValue         (void* null_val=NULL) const;
        std::string getSize                 () const;
        int         getHeight               () const;
        int         getWidth                () const;

        // place the information from this object into the xml library 'lib'
        // virtual RpString& put(RpLibrary lib);
        // RpString& put() const;

        RpString (
                    std::string path, 
                    std::string defaultVal
                )
            :   RpVariable  (path, new std::string (defaultVal) ),
                width       (0),
                height      (0)
        {}

        RpString (
                    std::string path, 
                    std::string defaultVal,
                    std::string sizeWxH
                )
            :   RpVariable  (path, new std::string (defaultVal) ),
                width       (0),
                height      (0)
        {
            setSize(sizeWxH);
        }

        RpString (
                    std::string path, 
                    std::string defaultVal,
                    std::string sizeWxH,
                    std::string label,
                    std::string desc
                )
            :   RpVariable  (path, new std::string (defaultVal), label, desc),
                width       (0),
                height      (0)
        {
            setSize(sizeWxH);
        }
        
        // copy constructor
        RpString ( const RpString& myRpString )
            :   RpVariable(myRpString),
                width       (myRpString.width),
                height      (myRpString.height)
        {}

        // default destructor
        virtual ~RpString ()
        {
            // clean up dynamic memory
        }

    private:
        int width;
        int height;
        std::string hints;

        

};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
