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
        RpUnits * getBasis() {return basis;};

        friend class RpUnits;

    private:
        const std::string units;
        double exponent;
        // RpUnits* basis[4]; 
        RpUnits* basis; 

        unit* prev;
        unit* next;

        unit ( 
                const std::string& units, 
                double&            exponent, 
                // RpUnits**           basis, 
                RpUnits*           basis, 
                unit*              next
             )
            :   units    (units), 
                exponent (exponent), 
                basis    (basis), 
                prev     (NULL),
                next     (next)
            { };


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

    private:

        RpUnits* fromPtr;
        RpUnits* toPtr;
        double (*convForwFxnPtr)(double);
        double (*convBackFxnPtr)(double);
        void* (*convForwFxnPtrVoid)(void*, void*);
        void* convForwData;
        void* (*convBackFxnPtrVoid)(void*, void*);
        void* convBackData;

        conversion* prev;
        conversion* next;

        // constructor
        // private because i only want RpUnits to be able to 
        // create a conversion
        conversion ( 
                RpUnits* fromPtr, 
                RpUnits* toPtr, 
                double (*convForwFxnPtr)(double),
                double (*convBackFxnPtr)(double),
                conversion* prev,
                conversion* next
             )
            :   fromPtr             (fromPtr),
                toPtr               (toPtr), 
                convForwFxnPtr      (convForwFxnPtr),
                convBackFxnPtr      (convBackFxnPtr),
                convForwFxnPtrVoid  (NULL),
                convForwData        (NULL),
                convBackFxnPtrVoid  (NULL),
                convBackData        (NULL),
                prev                (prev),
                next                (next)
            { 
            };

        conversion ( 
                RpUnits* fromPtr, 
                RpUnits* toPtr, 
                void* (*convForwFxnPtrVoid)(void*, void*),
                void* convForwData,
                void* (*convBackFxnPtrVoid)(void*, void*),
                void* convBackData,
                conversion* prev,
                conversion* next
             )
            :   fromPtr             (fromPtr),
                toPtr               (toPtr), 
                convForwFxnPtr      (NULL),
                convBackFxnPtr      (NULL),
                convForwFxnPtrVoid  (convForwFxnPtrVoid),
                convForwData        (convForwData),
                convBackFxnPtrVoid  (convBackFxnPtrVoid),
                convBackData        (convBackData),
                prev                (prev),
                next                (next)
            { 
            };

        // copy constructor

        // destructor
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
            { };

        /*
        ~convEntry()
        {

        }
        */
};

class RpUnits
{
    /* 
     * helpful units site
     * http://aurora.regenstrief.org/~gunther/units.html
     */

    public:
        
        // users member fxns
        std::string getUnits();
        std::string getUnitsName();
        double getExponent();
        RpUnits* getBasis();

        // convert from one RpUnits to another if the conversion is defined
        double convert(RpUnits* toUnits, double val, int* result = NULL); 
        // convert from one RpUnits to another if the conversion is defined
        void* convert(RpUnits* toUnits, void* val, int* result = NULL); 
        // convert from one RpUnits to another if the conversion is defined
        double convert(std::string, double val); 
        // convert from one RpUnits to another if the conversion is defined
        std::string convert (   RpUnits* toUnits, 
                                double val, 
                                int showUnits = 0, 
                                int* result = NULL  ); 

        static std::string convert ( std::string val,
                                     std::string toUnits,
                                     int showUnits,
                                     int* result = NULL );

        // dictionary to store the units.
        // static RpDict<std::string,RpUnits> dict;

        // turn the current unit to the metric system
        // this should only be used for units that are part of the 
        // metric system. doesnt deal with exponents, just prefixes
        double makeBasis(double value, int* result = NULL);
        RpUnits & makeBasis(double* value, int* result = NULL);
        
        static int makeMetric(RpUnits * basis);

        // find a RpUnits object that should exist in RpUnitsTable
        // returns 0 on success (object was found)
        // returns !0 on failure (object not found)
        static RpUnits* find(std::string key)
        {
            // dict.find seems to return a (RpUnits* const) so i had to 
            // cast it as a (RpUnits*)

            // dict pointer
             // RpUnits* unitEntry = (RpUnits*) *(dict.find(key).getValue());
             RpUnits* unitEntry = (RpUnits*) *(dict->find(key).getValue());

            // RpUnits* unitEntry = (RpUnits*) dEntr.getValue();

            // dict pointer
            // if (unitEntry == (RpUnits*)dict.getNullEntry().getValue()) {
            if (unitEntry == (RpUnits*)dict->getNullEntry().getValue()) {
                unitEntry = NULL;
            }

            return unitEntry;
        };

        // user calls define to add a RpUnits object or to add a relation rule
        //
        // add RpUnits Object
        static RpUnits * define(const std::string units, RpUnits * basis);
        static RpUnits * defineCmplx(const std::string units,RpUnits * basis);
        //
        // add relation rule 

        static RpUnits * define(RpUnits* from, 
                                RpUnits* to, 
                                double (*convForwFxnPtr)(double),
                                double (*convBackFxnPtr)(double));

        static RpUnits * define(RpUnits* from, 
                                RpUnits* to, 
                                void* (*convForwFxnPtr)(void*, void*),
                                void* convForwData,
                                void* (*convBackFxnPtr)(void*, void*),
                                void* convBackData);

        // populate the dictionary with a set of units specified by group
        // if group equals......................then load................
        //      "all"                       load all available units
        //      "energy"                    load units related to length
        //      "length"                    load units related to length
        //      "temp"                      load units related to temperature
        //      "time"                      load units related to time
        //  (no other groups have been created)

        static int addPresets (std::string group);

        // undefining a relation rule is probably not needed 
        // int undefine(); // delete a relation

        // convert fxn will be taken care of in the RpUnitsConversion class
        // i think
        // int convert(const char* from, const char* to);

        // why are these functions friends...
        // probably need to find a better way to let RpUnits 
        // use the RpDict and RpDictEntry fxns
        friend class RpDict<std::string,RpUnits*>;
        friend class RpDictEntry<std::string,RpUnits*>;

        // copy constructor
        RpUnits ( const RpUnits& myRpUnit )
        { 

            /*
            from = myRpUnit.from;
            to = myRpUnit.to;
            convertForw = myRpUnit.convertForw;
            convertBack = myRpUnit.convertBack;
            */

            // copy constructor for unit 
            unit* tmp = NULL;
            unit* newUnit = NULL;
            unit* p = myRpUnit.head;

            while (p != NULL) {
                newUnit = new unit(p->units, p->exponent,p->basis,NULL);

                newUnit->prev = tmp;
                if (tmp) {
                    tmp->next = newUnit;
                }

                tmp = newUnit;
                p = p->next;
            }

            if (tmp) {
                while (tmp->prev != NULL) {
                   tmp = tmp->prev;
                }
            }

            head = tmp;
                
        };
        
        /*
        RpUnits& operator= (const RpUnits& myRpUnit) {

            if ( this != &myRpUnit ) {
                delete head;
                delete convList;
                delete conv;
            }
        }
        */
        
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
        convEntry* convList;

        // used by the RpUnits when defining conversion elements
        conversion* conv;

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
                    // RpUnits* basis[] 
                    RpUnits* basis
                )
            :   head        ( new unit( units, exponent, basis, NULL) ),
                convList    (NULL),
                conv        (NULL)
        {};

        // create a conversion element



        
        RpUnits ( 
                    RpUnits* from,
                    RpUnits* to,
                    double (*convForwFxnPtr)(double),
                    double (*convBackFxnPtr)(double),
                    conversion* prev,
                    conversion* next
                )
            :   head (NULL),
                convList (NULL),
                conv (new conversion 
                        (from,to,convForwFxnPtr,convBackFxnPtr,prev,next))
        { 
            connectConversion(from);
            connectConversion(to);
        };




        
        RpUnits ( 
                    RpUnits* from,
                    RpUnits* to,
                    void* (*convForwFxnPtr)(void*, void*),
                    void* convForwData,
                    void* (*convBackFxnPtr)(void*, void*),
                    void* convBackData,
                    conversion* prev,
                    conversion* next
                )
            :   head (NULL),
                convList (NULL),
                conv (new conversion (  from, to,
                                        convForwFxnPtr, convForwData,
                                        convBackFxnPtr, convBackData,
                                        prev, next
                                     )
                     )
        { 
            connectConversion(from);
            connectConversion(to);
        };

        // default destructor
        //
        ~RpUnits ()
        {
            // clean up dynamic memory

            unit* p = head;
            while (p != NULL) {
                unit* tmp = p;
                p = p->next;
                delete tmp;
            }
        }

        void RpUnits::connectConversion(RpUnits* myRpUnit);

        void RpUnits::fillMetricMap();

        // create the container keeping track of defined units
        // returns 0 on success (table created or already exists)
        // returns !0 on failure (table not created or cannot exist)
        int RpUnits::createUnitsTable();

        // insert new RpUnits object into RpUnitsTable
        // returns 0 on success (object inserted or already exists)
        // returns !0 on failure (object cannot be inserted or dne)
        int RpUnits::insert(std::string key);   

        // link two RpUnits objects that already exist in RpUnitsTable
        // returns 0 on success (linking within table was successful)
        // returns !0 on failure (linking was not successful)
        int RpUnits::link(RpUnits * unitA);


        static int pre_compare( std::string& units, RpUnits* basis = NULL);
        void addUnit( const std::string& units,
                      double &  exponent,
                      RpUnits * basis
                     );

        static int addPresetAll();
        static int addPresetEnergy();
        static int addPresetLength();
        static int addPresetTemp();
        static int addPresetTime();


};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

// #include "../src/RpUnits.cc"

#endif
