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

#ifndef _RpUNITS_H
    #include "RpUnits.h"
#endif

#ifndef _RpNUMBER_H 
#define _RpNUMBER_H

class RpNumber : public RpVariable
{
    public:

        // users member fxns

        /*
        virtual RpNumber& setUnits(std::string newUnits);
        */

        virtual RpNumber& setDefaultValue(double newDefaultVal);
        virtual RpNumber& setCurrentValue(double newCurrentVal);
        virtual RpNumber& setMin(double newMin);
        virtual RpNumber& setMax(double newMax);

        std::string getUnits() const;
        // changed from "Value" to "Val" because base class makes 
        // these functions virtual and derived class has a different
        // return type. compiler doesnt like this. need to find another
        // way around this
        //
        // if we keep the null_val=NULL will that give us undefined behavior?
        //
        double getDefaultValue(void* null_val=NULL) const;
        double getCurrentValue(void* null_val=NULL) const;
        double getMin() const;
        double getMax() const;

        double convert( std::string toUnitStr, 
                        int storeResult, 
                        int* result = NULL  );
        double convert(std::string toUnitStr, int* result = NULL);
        double convert(const RpUnits* toUnit, int* result = NULL);

        // place the information from this object into the xml library 'lib'
        // virtual RpNumber& put(RpLibrary lib);
        RpNumber& put() const;
        RpNumber& put(double currentVal);



        /*
         * user provides a 'path' for where this object lives in xml file
         * user calls define with standard hints as described in Number docs
         *  'units' - string providing units of the number
         *  'defaultVal' - numeric value to be used if user provides no value
         *  'min' - (optional) minimum allowed value
         *  'max' - (optional) maximum allowed value
         *  'label' - (optional) name of the input for a gui field
         *  'desc' - (optional) description of the input for the gui
         *
         *  Notes: 
         *      if the user sets a min or max value the following applies:
         *          if the value of min is greater than the value of default,
         *          then min will be set to default
         *
         *          if the value of max is less than the value of default, 
         *          then max will be set to default.
         *
         *
         *
         */

        /* what about unit-less numbers */
        RpNumber (
                    std::string path, 
                    std::string units,
                    double defaultVal
                )
            :   RpVariable  (path, new double (defaultVal) ),
                // units       (units),
                min         (0),
                max         (0),
                minmaxSet   (0)
        {
            this->units = RpUnits::find(units);
            if (! this->units) {
                this->units = RpUnits::define(units,NULL);
            }
        }

        RpNumber (
                    std::string path, 
                    std::string units,
                    double defaultVal,
                    double min,
                    double max,
                    std::string label,
                    std::string desc
                )
            :   RpVariable  (path, new double (defaultVal), label, desc),
                // units       (units),
                min         (min),
                max         (max),
                minmaxSet   (0)
        { 

            this->units = RpUnits::find(units);
            if (! this->units) {
                this->units = RpUnits::define(units,NULL);
            }

            if ((min == 0) && (max == 0)) {
                minmaxSet = 0;
            }
            else {

                if (min > defaultVal) {
                    this->min = defaultVal;
                }

                if (max < defaultVal) {
                    this->max = defaultVal;
                }
            }

        }

        // copy constructor
        RpNumber ( const RpNumber& myRpNumber )
            :   RpVariable(myRpNumber),
                units       (myRpNumber.units),
                min         (myRpNumber.min),
                max         (myRpNumber.max),
                minmaxSet   (myRpNumber.minmaxSet)
        {}

        // default destructor
        // virtual ~RpNumber ()
        virtual ~RpNumber ()
        {
            // clean up dynamic memory

        }
    private:

        // std::string units;
        const RpUnits* units;
        double min;
        double max;

        // flag to tell if the user specified the min and max values 
        int minmaxSet;

};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
