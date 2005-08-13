
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <errno.h>


// #include "RpLibrary.h"

#ifndef _RpABOUT_H
    #include "RpAbout.h"
#endif

#ifndef _RpVARIABLE_H 
#define _RpVARIABLE_H

class RpVariable
{
    public:
        
        // users member fxns

        virtual RpVariable& setPath         (std::string newPath);
        virtual RpVariable& setDefaultValue (void* newDefaultVal);
        virtual RpVariable& setCurrentValue (void* newCurrentVal);
        virtual RpVariable& setLabel        (std::string newLabel);
        virtual RpVariable& setDesc         (std::string newDesc);
        virtual RpVariable& setHints        (std::string newHints);
        virtual RpVariable& setColor        (std::string newColor);
        virtual RpVariable& setIcon         (std::string newIcon);

        virtual std::string getPath         () const;
        /* maybe we dont need 2 get functions? */
        virtual void* getDefaultValue       () const;
        virtual void* getCurrentValue       () const;
        virtual std::string getLabel        () const;
        virtual std::string getDesc         () const;
        virtual std::string getHints        () const;
        virtual std::string getColor        () const;
        virtual std::string getIcon         () const;

        // place the information from this object into the xml library 'lib'
        // virtual RpNumber& put(RpLibrary lib);
        // virtual RpVariable& put() const;

        // we need a copy of defaultVal in currentVal, not the same pointer.
        // need to fix this.
        
        RpVariable (
                    std::string path, 
                    void* defaultVal
                )
            :   about       (RpAbout()),
                path        (path),
                defaultVal  (defaultVal),
                currentVal  (defaultVal)
        {
            // about's default no-arg const is called
        };

        RpVariable (
                    std::string path, 
                    void* defaultVal,
                    std::string label,
                    std::string desc
                )
            :   about       (RpAbout(label,desc)),
                path        (path),
                defaultVal  (defaultVal),
                currentVal  (defaultVal)
        {}
        
        // copy constructor
        RpVariable ( const RpVariable& myRpVariable ) 
                // will the RpAbout copy constructor be called here?
            :   about       (RpAbout(myRpVariable.about)),
                path        (myRpVariable.path),
                defaultVal  (myRpVariable.defaultVal),
                currentVal  (myRpVariable.currentVal)
        {}

        // default destructor
        virtual ~RpVariable ()
        {
            // clean up dynamic memory
        }

    private:

        RpAbout about;
        std::string path;
        void* defaultVal;
        void* currentVal;


};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
