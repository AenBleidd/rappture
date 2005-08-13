
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <errno.h>

// #include "RpLibrary.h"

#ifndef _RpABOUT_H
    #include "RpAbout.h"
#endif

#ifndef _RpOPTION_H 
#define _RpOPTION_H

class RpOption
{
    public:
        
        // users member fxns

        virtual RpOption& setLabel       (std::string newLabel);
        virtual std::string getLabel    () const;

        /*
        RpOption ()
            :   about       (RpAbout())
        {};
        */

        RpOption (
                    std::string label
                )
            :   about       (RpAbout(label))
        {};
        
        // copy constructor
        RpOption ( const RpOption& myRpOption ) 
            :   about       (RpAbout(myRpOption.about))
        {}

        // default destructor
        virtual ~RpOption ()
        {
            // clean up dynamic memory
        }

    private:

        RpAbout about;
};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
