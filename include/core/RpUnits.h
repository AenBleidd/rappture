/*
 * ----------------------------------------------------------------------
 *  RpUnits.h - Header file for Rappture Units
 *
 *   RpUnits Class Declaration / Definition
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <errno.h>

#ifndef _RpDICT_H
    #include "RpDict.h"
#endif

#include "RpUnitsStd.h"

#ifndef _RpUNITS_H
#define _RpUNITS_H

class RpUnits;

class unit
{

    public:

        const std::string getUnits(){ return units; };
        double getExponent() {return exponent;};
        const RpUnits * getBasis() {return basis;};

        friend class RpUnits;

    private:
        const std::string units;
        double exponent;
        const RpUnits* basis;

        unit* prev;
        unit* next;

        // private constructor
        unit (
                const std::string& units,
                double&            exponent,
                const RpUnits*     basis,
                unit*              prev,
                unit*              next
             )
            :   units    (units),
                exponent (exponent),
                basis    (basis),
                prev     (prev),
                next     (next)
            {};

        /*
        // private copy constructor
        unit ( const unit& other )
            :   units        other.units,
                exponent     other.exponent,
                basis        other.basis,
                prev         unit(other.prev),
                next         unit(other.next)
            {};

        // copy assignment operator
        unit& operator= (unit& other) 
            :   units        other.units,
                exponent     other.exponent,
                basis        RpUnits(other.basis),
                prev         unit(other.prev),
                next         unit(other.next)
        {}
        */

        /*
        // destructor (its not virtual yet, still testing)
        ~unit () {

        }
        */

        void newExponent(double newExponent) {exponent = newExponent;};


};

// simple class to hold info about a conversion.
// holds where we are converting from
// holds where we are converting to
// hold the pointer to a function to do the forward conversion (from->to)
// hold the pointer to a function to do the backward conversion (to->from)
//
class conversion
{
    public:

        const RpUnits* getFrom()    { return (const RpUnits*) fromPtr; };
        const RpUnits* getTo()      { return (const RpUnits*) toPtr; };

        friend class RpUnits;

        // copy constructor
        conversion ( conversion& other)
            :   fromPtr             (other.fromPtr),
                toPtr               (other.toPtr),
                convForwFxnPtr      (other.convForwFxnPtr),
                convBackFxnPtr      (other.convBackFxnPtr),
                convForwFxnPtrDD    (other.convForwFxnPtrDD),
                convBackFxnPtrDD    (other.convBackFxnPtrDD),
                convForwFxnPtrVoid  (other.convForwFxnPtrVoid),
                convForwData        (other.convForwData),
                convBackFxnPtrVoid  (other.convBackFxnPtrVoid),
                convBackData        (other.convBackData)
            {};

        // copy assignment operator
        conversion& operator= ( conversion& other )
            {
                fromPtr             = other.fromPtr;
                toPtr               = other.toPtr;
                convForwFxnPtr      = other.convForwFxnPtr;
                convBackFxnPtr      = other.convBackFxnPtr;
                convForwFxnPtrDD    = other.convForwFxnPtrDD;
                convBackFxnPtrDD    = other.convBackFxnPtrDD;
                convForwFxnPtrVoid  = other.convForwFxnPtrVoid;
                convForwData        = other.convForwData;
                convBackFxnPtrVoid  = other.convBackFxnPtrVoid;
                convBackData        = other.convBackData;
                return *this;
            }

        // default destructor
        virtual ~conversion ()
        {}

    private:

        const RpUnits* fromPtr;
        const RpUnits* toPtr;
        double (*convForwFxnPtr)(double);
        double (*convBackFxnPtr)(double);
        double (*convForwFxnPtrDD)(double,double);
        double (*convBackFxnPtrDD)(double,double);
        void* (*convForwFxnPtrVoid)(void*, void*);
        void* convForwData;
        void* (*convBackFxnPtrVoid)(void*, void*);
        void* convBackData;

        // constructor
        // private because i only want RpUnits to be able to
        // create a conversion
        conversion (
                const RpUnits* fromPtr,
                const RpUnits* toPtr,
                double (*convForwFxnPtr)(double),
                double (*convBackFxnPtr)(double)
             )
            :   fromPtr             (fromPtr),
                toPtr               (toPtr),
                convForwFxnPtr      (convForwFxnPtr),
                convBackFxnPtr      (convBackFxnPtr),
                convForwFxnPtrDD    (NULL),
                convBackFxnPtrDD    (NULL),
                convForwFxnPtrVoid  (NULL),
                convForwData        (NULL),
                convBackFxnPtrVoid  (NULL),
                convBackData        (NULL)
            {};

        // constructor for 2 argument conversion functions
        conversion (
                const RpUnits* fromPtr,
                const RpUnits* toPtr,
                double (*convForwFxnPtr)(double,double),
                double (*convBackFxnPtr)(double,double)
             )
            :   fromPtr             (fromPtr),
                toPtr               (toPtr),
                convForwFxnPtr      (NULL),
                convBackFxnPtr      (NULL),
                convForwFxnPtrDD    (convForwFxnPtr),
                convBackFxnPtrDD    (convBackFxnPtr),
                convForwFxnPtrVoid  (NULL),
                convForwData        (NULL),
                convBackFxnPtrVoid  (NULL),
                convBackData        (NULL)
            {};

        // constructor for user defined void* returning 1 arg conversion fxns.
        // the 1 arg is a user defined void* object
        conversion (
                const RpUnits* fromPtr,
                const RpUnits* toPtr,
                void* (*convForwFxnPtrVoid)(void*, void*),
                void* convForwData,
                void* (*convBackFxnPtrVoid)(void*, void*),
                void* convBackData
             )
            :   fromPtr             (fromPtr),
                toPtr               (toPtr),
                convForwFxnPtr      (NULL),
                convBackFxnPtr      (NULL),
                convForwFxnPtrDD    (NULL),
                convBackFxnPtrDD    (NULL),
                convForwFxnPtrVoid  (convForwFxnPtrVoid),
                convForwData        (convForwData),
                convBackFxnPtrVoid  (convBackFxnPtrVoid),
                convBackData        (convBackData)
            {};

};

// used by the RpUnits class to create a linked list of the conversions
// associated with the specific unit.
//
// we could templitize this and make a generic linked list
// or could use generic linked list class from book programming with objects
//
class convEntry
{

    public:

        friend class RpUnits;

        virtual ~convEntry()
        {}

    private:

        conversion* conv;
        convEntry*  prev;
        convEntry*  next;

        convEntry (
                conversion* conv,
                convEntry*  prev,
                convEntry*  next
             )
            :   conv    (conv),
                prev    (prev),
                next    (next)
            {};

};

class RpUnits
{
    /*
     * helpful units site
     * http://aurora.regenstrief.org/~gunther/units.html
     */

    public:

        // users member fxns
        std::string getUnits() const;
        std::string getUnitsName() const;
        double getExponent() const;
        const RpUnits* getBasis() const;

        // convert from one RpUnits to another if the conversion is defined
        double convert(         const RpUnits* toUnits, 
                                double val, 
                                int* result=NULL    ) const;
        // convert from one RpUnits to another if the conversion is defined
        void* convert(          const RpUnits* toUnits, 
                                void* val, 
                                int* result=NULL) const;
        // convert from one RpUnits to another if the conversion is defined
        // double convert(std::string, double val);
        // convert from one RpUnits to another if the conversion is defined
        std::string convert (   const RpUnits* toUnits,
                                double val,
                                int showUnits = 0,
                                int* result = NULL  ) const;

        static std::string convert ( std::string val,
                                     std::string toUnits,
                                     int showUnits,
                                     int* result = NULL );

        // dictionary to store the units.
        // static RpDict<std::string,RpUnits> dict;

        // turn the current unit to the metric system
        // this should only be used for units that are part of the
        // metric system. doesnt deal with exponents, just prefixes
        double makeBasis(double value, int* result = NULL) const;
        const RpUnits & makeBasis(double* value, int* result = NULL) const;

        static int makeMetric(const RpUnits * basis);

        // find a RpUnits object that should exist in RpUnitsTable
        // returns 0 on success (object was found)
        // returns !0 on failure (object not found)
        static const RpUnits* find(std::string key);

        // user calls define to add a RpUnits object or to add a relation rule
        //
        // add RpUnits Object
        static RpUnits * define(const std::string units,
                                const RpUnits* basis);
                                // unsigned int create=0);
        static RpUnits * defineCmplx(   const std::string units,
                                        const RpUnits* basis);
        //
        // add relation rule

        static RpUnits * define(const RpUnits* from,
                                const RpUnits* to,
                                double (*convForwFxnPtr)(double),
                                double (*convBackFxnPtr)(double));

        static RpUnits * define(const RpUnits* from,
                                const RpUnits* to,
                                double (*convForwFxnPtr)(double,double),
                                double (*convBackFxnPtr)(double,double));

        static RpUnits * define(const RpUnits* from,
                                const RpUnits* to,
                                void* (*convForwFxnPtr)(void*, void*),
                                void* convForwData,
                                void* (*convBackFxnPtr)(void*, void*),
                                void* convBackData);

        // populate the dictionary with a set of units specified by group
        // if group equals......................then load................
        //      "all"                       load all available units
        //      "energy"                    load units related to energy
        //      "length"                    load units related to length
        //      "temp"                      load units related to temperature
        //      "time"                      load units related to time
        //      "volume"                    load units related to volume
        //  (no other groups have been created)

        static int addPresets (const std::string group);

        // undefining a relation rule is probably not needed
        // int undefine(); // delete a relation

        // why are these functions friends...
        // probably need to find a better way to let RpUnits
        // use the RpDict and RpDictEntry fxns
        friend class RpDict<std::string,RpUnits*>;
        friend class RpDictEntry<std::string,RpUnits*>;

        // copy constructor
        RpUnits (RpUnits &other)
            : head (NULL),
              convList (NULL)
        {
            unit* p = NULL;
            unit* current = NULL;
            convEntry* q = NULL;
            convEntry* curr2 = NULL;

            dict = other.dict;

            if (other.head) {
                p = other.head;
                head = new unit(p->units, p->exponent, p->basis, NULL, NULL);
                current = head;

                while (p->next) {
                    p = p->next;
                    current->next = new unit( p->units,
                                              p->exponent,
                                              p->basis,
                                              current,
                                              NULL        );
                    current = current->next;
                }
            }
            if (other.convList) {
                q = other.convList;
                convList = new convEntry (q->conv,NULL,NULL);
                curr2 = convList;
                while (q->next) {
                    q = q->next;
                    curr2->next = new convEntry (q->conv,curr2,NULL);
                    curr2 = curr2->next;
                }
            }
        }

        // copy assignment operator
        RpUnits& operator= (const RpUnits& other) {

            unit* p = NULL;
            unit* current = NULL;
            convEntry* q = NULL;
            convEntry* curr2 = NULL;

            if ( this != &other ) {
                delete head;
                delete convList;
            }

            dict = other.dict;

            if (other.head) {
                p = other.head;
                head = new unit(p->units, p->exponent, p->basis, NULL, NULL);
                current = head;

                while (p->next) {
                    p = p->next;
                    current->next = new unit( p->units,
                                              p->exponent,
                                              p->basis,
                                              current,
                                              NULL        );
                    current = current->next;
                }
            }
            if (other.convList) {
                q = other.convList;
                convList = new convEntry (q->conv,NULL,NULL);
                curr2 = convList;
                while (q->next) {
                    q = q->next;
                    curr2->next = new convEntry (q->conv,curr2,NULL);
                    curr2 = curr2->next;
                }
            }

            return *this;
        }

        // default destructor
        //
        ~RpUnits ()
        {
            // clean up dynamic memory

            unit* p = head;
            unit* tmp = p;
            convEntry* p2 = convList;
            convEntry* tmp2 = p2;

            while (p != NULL) {
                tmp = p;
                p = p->next;
                delete tmp;
            }

            while (p2 != NULL) {
                tmp2 = p2;
                p2 = p2->next;
                delete tmp2;
            }
        }

    private:

        // i hope to replace these vars with a small class representing a
        // of a linked list of units. that way we can handle complex units.
        // this will also require that we move the getUnits, getExponent, 
        // and getBasis, makeBasis functionality into the smaller class,
        // that represents each part of the units. we should keep getUnits,
        // getBasis, and makeBasis in this class, but modify them to go
        // through the linked list of units and combine the units back into
        // what the caller expects to see.
        // ie. cm2/Vms (mobility), will be split into the objects cm2, V, ms
        // when getUnits is called, the user should see cm2/Vms again.
        // for basis, possibly let the user choose a basis, and for makeBasis
        // move through the linked list, only converting the metric elements.
        //

        // used by the RpUnits when defining units elements
        unit* head;

        // linked list of units this RpUnit can convert to
        // its mutable because the connectConversion function takes in a
        // const RpUnits* and attempts to change the convList variable
        // within the RpUnits Object
        mutable convEntry* convList;


        // dictionary to store the units.
        static RpDict<std::string,RpUnits*>* dict;

        // create a units element
        // class takes in three pieces of info
        // 1) string describing the units
        // 2) double describing the exponent
        // 3) pointer to RpUnits object describing the basis 
        //      of the units (the fundamental unit)
        //



        RpUnits (
                    const std::string& units,
                    double& exponent,
                    const RpUnits* basis
                )
            :   head        ( new unit( units, exponent, basis, NULL, NULL) ),
                convList    (NULL)
        {};

        void RpUnits::connectConversion(conversion* conv) const;

        // insert new RpUnits object into RpUnitsTable
        // returns 0 on success (object inserted or already exists)
        // returns !0 on failure (object cannot be inserted or dne)
        int RpUnits::insert(std::string key);

        // link two RpUnits objects that already exist in RpUnitsTable
        // returns 0 on success (linking within table was successful)
        // returns !0 on failure (linking was not successful)
        // int RpUnits::link(RpUnits * unitA);


        static int pre_compare( std::string& units,
                                const RpUnits* basis = NULL);

        void addUnit( const std::string& units,
                      double &  exponent,
                      const RpUnits * basis
                     );

        static int addPresetAll();
        static int addPresetEnergy();
        static int addPresetLength();
        static int addPresetTemp();
        static int addPresetTime();
        static int addPresetVolume();


};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
