
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <errno.h>


// #include "RpLibrary.h"

#ifndef _RpABOUT_H 
#define _RpABOUT_H

class RpAbout
{
    public:
        
        // users member fxns

        virtual RpAbout& setLabel       (std::string newLabel);
        virtual RpAbout& setDesc        (std::string newDesc);
        virtual RpAbout& setHints       (std::string newHints);
        virtual RpAbout& setColor       (std::string newColor);
        virtual RpAbout& setIcon        (std::string newIcon);

        virtual std::string getLabel    () const;
        virtual std::string getDesc     () const;
        virtual std::string getHints    () const;
        virtual std::string getColor    () const;
        virtual std::string getIcon     () const;

        RpAbout ()
            :   label       (""),
                desc        (""),
                hints       (""),
                color       (""),
                icon        ("")
        {};

        RpAbout (
                    std::string label
                )
            :   label       (label),
                desc        (""),
                hints       (""),
                color       (""),
                icon        ("")
        {};

        RpAbout (
                    std::string label,
                    std::string desc
                )
            :   label       (label),
                desc        (desc),
                hints       (""),
                color       (""),
                icon        ("")
        {};

        RpAbout (
                    std::string label,
                    std::string desc,
                    std::string hints,
                    std::string color,
                    std::string icon
                )
            :   label       (label),
                desc        (desc),
                hints       (hints),
                color       (color),
                icon        (icon)
        {}
        
        // copy constructor
        RpAbout ( const RpAbout& myRpAbout ) 
            :   label       (myRpAbout.label),
                desc        (myRpAbout.desc),
                hints       (myRpAbout.hints),
                color       (myRpAbout.color),
                icon        (myRpAbout.icon)
        {}

        // default destructor
        virtual ~RpAbout ()
        {
            // clean up dynamic memory
        }

    private:

        std::string label;
        std::string desc;
        std::string hints;
        std::string color;
        std::string icon;


};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
