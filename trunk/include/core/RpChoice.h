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
#include <vector>


// #include "RpLibrary.h"

#ifndef _RpVARIABLE_H
    #include "RpVariable.h"
#endif

#ifndef _RpOPTION_H
    #include "RpOption.h"
#endif

#ifndef _RpCHOICE_H 
#define _RpCHOICE_H

class RpChoice : public RpVariable
{
    public:
        
        // users member fxns

        virtual RpChoice& setDefaultValue   (std::string newDefaultVal);
        virtual RpChoice& setCurrentValue   (std::string newCurrentVal);
        virtual RpChoice& setOption         (std::string newOption);
        virtual RpChoice& deleteOption      (std::string newOption);

        // changed from "Value" to "Val" because base class makes 
        // these functions virtual and derived class has a different
        // return type. compiler doesnt like this. need to find another
        // way around this
        //
        // if we keep the null_val=NULL will that give us undefined behavior?
        //
        virtual std::string getDefaultValue (void* null_val=NULL) const;
        virtual std::string getCurrentValue (void* null_val=NULL) const;
        virtual std::string getFirstOption  ();
        virtual std::string getNextOption   ();
        virtual unsigned int getOptListSize () const;

        
        // place the information from this object into the xml library 'lib'
        // virtual RpChoice& put(RpLibrary lib);
        // RpChoice& put() const;

        RpChoice (
                    std::string path, 
                    std::string defaultVal,
                    std::string optionList
                )
            :   RpVariable  (path, new std::string (defaultVal) )
        {
            // optionList is a comma separated list of options

            optionsIter = options.begin();
            std::string::size_type pos = optionList.find (',',0);
            std::string::size_type lastPos = 0;
            std::string tmpStr;
            

            while (pos != std::string::npos) {
                tmpStr = optionList.substr(lastPos, pos-lastPos);
                setOption(tmpStr);
                lastPos = pos + 1;
                pos = optionList.find (",",lastPos);
            }

            tmpStr = optionList.substr(lastPos);
            setOption(tmpStr);

        }

        RpChoice (
                    std::string path, 
                    std::string defaultVal,
                    std::string label,
                    std::string desc,
                    std::string optionList
                )
            :   RpVariable  (path, new std::string (defaultVal), label, desc)
        { 
            // optionList is a comma separated list of options

            optionsIter = options.begin();
            std::string::size_type pos = optionList.find (',',0);
            std::string::size_type lastPos = 0;
            std::string tmpStr;
            

            while (pos != std::string::npos) {
                tmpStr = optionList.substr(lastPos, pos-lastPos);
                setOption(tmpStr);
                lastPos = pos + 1;
                pos = optionList.find (",",lastPos);
            }

            tmpStr = optionList.substr(lastPos);
            setOption(tmpStr);

        }
        
        // copy constructor
        RpChoice ( const RpChoice& myRpChoice )
            :   RpVariable      (myRpChoice),
                options         (myRpChoice.options)
        {
        }

        // default destructor
        virtual ~RpChoice ()
        {
            // clean up dynamic memory

        }
    private:

        std::vector<RpOption> options;
        std::vector<RpOption>::iterator optionsIter;
};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
