/*
 * ----------------------------------------------------------------------
 *  RpUnits.h - Header file for Rappture Units
 *
 *   RpUnits Class Declaration / Definition
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RpUNITS_H
#define _RpUNITS_H

enum RP_UNITS_CONSTS {
    RPUNITS_UNITS_OFF           = 0,
    RPUNITS_UNITS_ON            = 1,

    // units naming flags
    RPUNITS_POS_EXP             = 1,
    RPUNITS_NEG_EXP             = 2,
    RPUNITS_STRICT_NAME         = 4,
    RPUNITS_ORIG_EXP            = 7
};

// RpUnits Case Insensitivity define
#define RPUNITS_CASE_INSENSITIVE true

#define RPUNITS_METRIC          true

//define our different types of units
#define RP_TYPE_ENERGY      "energy"
#define RP_TYPE_EPOT        "electric_potential"
#define RP_TYPE_LENGTH      "length"
#define RP_TYPE_TEMP        "temperature"
#define RP_TYPE_TIME        "time"
#define RP_TYPE_VOLUME      "volume"
#define RP_TYPE_ANGLE       "angle"
#define RP_TYPE_MASS        "mass"
#define RP_TYPE_PREFIX      "prefix"
#define RP_TYPE_PRESSURE    "pressure"
#define RP_TYPE_CONC        "concentration"
#define RP_TYPE_FORCE       "force"
#define RP_TYPE_MAGNETIC    "magnetic"
#define RP_TYPE_MISC        "misc"
#define RP_TYPE_POWER       "power"


#ifdef __cplusplus

#include <iostream>
#include <string>
#include <list>
#include <sstream>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "RpDict.h"
#include "RpUnitsStd.h"

#define LIST_TEMPLATE RpUnitsListEntry

class RpUnits;

class RpUnitsPreset {
    public:
        RpUnitsPreset () {
            addPresetAll();
        };

        static int  addPresetAll();
        static int  addPresetEnergy();
        static int  addPresetLength();
        static int  addPresetTemp();
        static int  addPresetTime();
        static int  addPresetVolume();
        static int  addPresetAngle();
        static int  addPresetMass();
        static int  addPresetPrefix();
        static int  addPresetPressure();
        static int  addPresetConcentration();
        static int  addPresetForce();
        static int  addPresetMagnetic();
        static int  addPresetMisc();
        static int  addPresetPower();
};

class RpUnitsTypes {
    public:

        typedef bool (*RpUnitsTypesHint)(RpUnits*);
        static RpUnitsTypesHint getTypeHint (std::string type);

        static bool hintTypePrefix    ( RpUnits* unitObj );
        static bool hintTypeNonPrefix ( RpUnits* unitObj );
        static bool hintTypeEnergy    ( RpUnits* unitObj );
        static bool hintTypeEPot      ( RpUnits* unitObj );
        static bool hintTypeLength    ( RpUnits* unitObj );
        static bool hintTypeTemp      ( RpUnits* unitObj );
        static bool hintTypeTime      ( RpUnits* unitObj );
        static bool hintTypeVolume    ( RpUnits* unitObj );
        static bool hintTypeAngle     ( RpUnits* unitObj );
        static bool hintTypeMass      ( RpUnits* unitObj );
        static bool hintTypePressure  ( RpUnits* unitObj );
        static bool hintTypeConc      ( RpUnits* unitObj );
        static bool hintTypeForce     ( RpUnits* unitObj );
        static bool hintTypeMagnetic  ( RpUnits* unitObj );
        static bool hintTypeMisc      ( RpUnits* unitObj );
        static bool hintTypePower     ( RpUnits* unitObj );

    private:

        RpUnitsTypes () {};
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

// used by the RpUnits class to create a linked list of the incarnated units
// associated with the specific unit.
//
// we could templitize this and make a generic linked list
// or could use generic linked list class from book programming with objects
//
class incarnationEntry
{

    public:

        friend class RpUnits;

        virtual ~incarnationEntry()
        {}

    private:

        const RpUnits* unit;
        incarnationEntry*  prev;
        incarnationEntry*  next;

        incarnationEntry (
                const RpUnits* unit,
                incarnationEntry*  prev,
                incarnationEntry*  next
             )
            :   unit    (unit),
                prev    (prev),
                next    (next)
            {};

};

class RpUnitsListEntry
{

    public:

        // constructor
        RpUnitsListEntry (const RpUnits* unit, double exponent, const RpUnits* prefix=NULL)
            : unit     (unit),
              exponent (exponent),
              prefix   (prefix)
        {};

        // copy constructor
        RpUnitsListEntry (const RpUnitsListEntry& other)
            : unit     (other.unit),
              exponent (other.exponent),
              prefix   (other.prefix)
        {};

        // copy assignment operator
        RpUnitsListEntry& operator= (const RpUnitsListEntry& other) {
            unit = other.unit;
            exponent = other.exponent;
            prefix = other.prefix;
            return *this;
        }

        // negate the exponent
        void negateExponent() const;

        // print the name of the object
        std::string name(int flags=RPUNITS_ORIG_EXP) const;

        // report the basis of the RpUnits object being stored.
        const RpUnits* getBasis() const;

        // return the RpUnits*
        const RpUnits* getUnitsObj() const;

        // return the exponent
        double getExponent() const;

        // return the metric prefix associated with this object
        const RpUnits* getPrefix() const;

        // destructor
        virtual ~RpUnitsListEntry ()
        {}

    private:

        const RpUnits* unit;
        mutable double exponent;
        const RpUnits* prefix;
};

class RpUnits
{
    /*
     * helpful units site
     * http://aurora.regenstrief.org/~gunther/units.html
     * http://www.nodc.noaa.gov/dsdt/ucg/
     * http://www.cellml.org/meeting_minutes/archive/
     *        20010110_meeting_minutes.html
     */

    public:

        static bool cmpFxn (char c1, char c2)
        {
            int lc1 = toupper(static_cast<unsigned char>(c1));
            int lc2 = toupper(static_cast<unsigned char>(c2));

            if ( (lc1 < lc2) || (lc1 > lc2) ) {
                return false;
            }

            return true;

        }
        struct _key_compare:
            public
            std::binary_function<std::string,std::string,bool> {

                bool operator() (   const std::string& lhs,
                                    const std::string& rhs ) const
                {
                    return std::lexicographical_compare( lhs.begin(),lhs.end(),
                                                         rhs.begin(),rhs.end(),
                                                         RpUnits::cmpFxn  );
                }
            };

        // why are these functions friends...
        // probably need to find a better way to let RpUnits
        // use the RpDict and RpDictEntry fxns
        friend class RpDict<std::string,RpUnits*,_key_compare>;
        friend class RpDictEntry<std::string,RpUnits*,_key_compare>;
        // users member fxns
        std::string getUnits() const;
        std::string getUnitsName(int flags=RPUNITS_ORIG_EXP) const;
        std::string getSearchName() const;
        double getExponent() const;
        const RpUnits* getBasis() const;
        RpUnits& setMetric(bool newVal);

        // retrieve a units type.
        std::string getType() const;

        // retrieve the case insensitivity of this unit object
        bool getCI() const;

        // retrieve a list of compatible units.
        std::list<std::string> getCompatible(double expMultiplier=1.0) const;


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
                                int showUnits = RPUNITS_UNITS_OFF,
                                int* result = NULL  ) const;

        static std::string convert ( std::string val,
                                     std::string toUnits,
                                     int showUnits = RPUNITS_UNITS_OFF,
                                     int* result = NULL );

        // turn the current unit to the metric system
        // this should only be used for units that are part of the
        // metric system. doesnt deal with exponents, just prefixes
        double makeBasis(double value, int* result = NULL) const;
        const RpUnits & makeBasis(double* value, int* result = NULL) const;

        static int makeMetric(RpUnits * basis);

        // find a RpUnits object that should exist in RpUnitsTable
        // returns 0 on success (object was found)
        // returns !0 on failure (object not found)
        static const RpUnits* find( std::string key,
        RpDict<std::string,RpUnits*,_key_compare>::RpDictHint hint = NULL);

        // validate is very similar to find, but it works better
        // for seeing complex units can be interpreted.
        // it validates that if the a certain units string is
        // provided as a unit type, then all of the base components
        // of that unit are available for conversions.
        // returns 0 on success (units are valid)
        // returns !0 on failure (units not valid)
        static int validate(std::string& inUnits,
                            std::string& type,
                            std::list<std::string>* compatList=NULL);

        // user calls define to add a RpUnits object or to add a relation rule
        //
        // add RpUnits Object
        static RpUnits * define(const std::string units,
                                const RpUnits* basis=NULL,
                                const std::string type="",
                                bool metric=false,
                                bool caseInsensitive=RPUNITS_CASE_INSENSITIVE);

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

        static int incarnate (  const RpUnits* abstraction,
                                const RpUnits* entity);

        // populate the dictionary with a set of units specified by group
        // if group equals........................then load................
        //                    "all"           load all available units
        //  RP_TYPE_ENERGY    "energy"        load units related to energy
        //  RP_TYPE_EPOT      "electric_potential" load units related to electric potential
        //  RP_TYPE_LENGTH    "length"        load units related to length
        //  RP_TYPE_TEMP      "temperature"   load units related to temperature
        //  RP_TYPE_TIME      "time"          load units related to time
        //  RP_TYPE_VOLUME    "volume"        load units related to volume
        //  RP_TYPE_ANGLE     "angle"         load units related to angles
        //  RP_TYPE_MASS      "mass"          load units related to mass
        //  RP_TYPE_PREFIX    "prefix"        load unit prefixes
        //  RP_TYPE_PRESSURE  "pressure"      load units related to pressure
        //  RP_TYPE_CONC      "concentration" load units related to concentration
        //  RP_TYPE_FORCE     "force"         load units related to force
        //  RP_TYPE_MAGNETIC  "magnetic"      load units related to magnetics
        //  RP_TYPE_MISC      "misc"          load units related to everything else
        //  RP_TYPE_POWER     "power"         load units related to power
        //  (no other groups have been created)

        static int addPresets (const std::string group);

        // undefining a relation rule is probably not needed
        // int undefine(); // delete a relation


        friend int insert(std::string key,RpUnits* val);


        // copy constructor
        RpUnits (RpUnits &other)
            : units    (other.units),
              exponent (other.exponent),
              basis    (other.basis),
              type     (other.type),
              metric   (other.metric),
              ci       (other.ci),
              convList (NULL)
        {
            convEntry* q = NULL;
            convEntry* curr = NULL;

            incarnationEntry* r = NULL;
            incarnationEntry* rcurr = NULL;

            dict = other.dict;

            if (other.convList) {
                q = other.convList;
                convList = new convEntry (q->conv,NULL,NULL);
                curr = convList;
                while (q->next) {
                    q = q->next;
                    curr->next = new convEntry (q->conv,curr,NULL);
                    curr = curr->next;
                }
            }

            if (other.incarnationList) {
                r = other.incarnationList;
                incarnationList = new incarnationEntry (r->unit,NULL,NULL);
                rcurr = incarnationList;
                while (r->next) {
                    r = r->next;
                    rcurr->next = new incarnationEntry (r->unit,rcurr,NULL);
                    rcurr = rcurr->next;
                }
            }

        }

        // copy assignment operator
        RpUnits& operator= (const RpUnits& other) {

            convEntry* q = NULL;
            convEntry* curr = NULL;

            incarnationEntry* r = NULL;
            incarnationEntry* rcurr = NULL;

            if ( this != &other ) {
                delete convList;
            }

            dict = other.dict;
            units = other.units;
            exponent = other.exponent;
            basis = other.basis;
            type = other.type;
            metric = other.metric;
            ci = other.ci;

            if (other.convList) {
                q = other.convList;
                convList = new convEntry (q->conv,NULL,NULL);
                curr = convList;
                while (q->next) {
                    q = q->next;
                    curr->next = new convEntry (q->conv,curr,NULL);
                    curr = curr->next;
                }
            }

            if (other.incarnationList) {
                r = other.incarnationList;
                incarnationList = new incarnationEntry (r->unit,NULL,NULL);
                rcurr = incarnationList;
                while (r->next) {
                    r = r->next;
                    rcurr->next = new incarnationEntry (r->unit,rcurr,NULL);
                    rcurr = rcurr->next;
                }
            }

            return *this;
        }

        // default destructor
        //
        ~RpUnits ()
        {
            // go to the type list and remove a variable by this name


            // clean up dynamic memory

            convEntry* p = convList;
            convEntry* tmp = p;

            incarnationEntry* r = incarnationList;
            incarnationEntry* rtmp = r;

            while (p != NULL) {
                tmp = p;
                p = p->next;
                delete tmp;
            }

            while (p != NULL) {
                rtmp = r;
                r = r->next;
                delete rtmp;
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

        std::string units;
        double exponent;
        const RpUnits* basis;
        std::string type;

        // tell if the unit can accept metric prefixes
        bool metric;

        // should this unit be inserted as a case insensitive unit?
        bool ci;

        // linked list of units this RpUnit can convert to
        // its mutable because the connectConversion function takes in a
        // const RpUnits* and attempts to change the convList variable
        // within the RpUnits Object
        mutable convEntry* convList;

        // linked list of incarnation units for this RpUnit
        // its mutable because the connectIncarnation function takes in a
        // const RpUnits* and attempts to change the incarnationList variable
        // within the RpUnits Object
        mutable incarnationEntry* incarnationList;

        // dictionary to store the units.
        static RpDict<std::string,RpUnits*,_key_compare>* dict;

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
                    const RpUnits* basis,
                    const std::string type,
                    bool metric,
                    bool caseInsensitive
                )
            :   units           (units),
                exponent        (exponent),
                basis           (basis),
                type            (type),
                metric          (metric),
                ci              (caseInsensitive),
                convList        (NULL),
                incarnationList (NULL)
        {};

        // insert new RpUnits object into RpUnitsTable
        // returns 0 on success (object inserted or already exists)
        // returns !0 on failure (object cannot be inserted or dne)
        // int RpUnits::insert(std::string key) const;

        typedef std::list<LIST_TEMPLATE> RpUnitsList;
        typedef double (*convFxnPtrD) (double);
        typedef std::list<convFxnPtrD> convertList;
        typedef RpUnitsList::iterator RpUnitsListIter;

        // void newExponent                (double newExponent) {exponent = newExponent;};

        static int      units2list          ( const std::string& inUnits,
                                              RpUnitsList& outList,
                                              std::string& type         );
        static int      list2units          ( RpUnitsList& inList,
                                              std::string& outUnitsStr  );
        static int      grabExponent        ( const std::string& inStr,
                                              double* exp               );
        static int      grabUnitString      ( const std::string& inStr  );
        static int      grabUnits           ( std::string inStr,
                                              int* offset,
                                              const RpUnits** unit,
                                              const RpUnits** prefix    );
        static int      checkMetricPrefix   ( std::string inStr,
                                              int* offset,
                                              const RpUnits** prefix    );
        static int      negateListExponents ( RpUnitsList& unitsList    );
        static int      printList           ( RpUnitsList& unitsList    );

        static int compareListEntryBasis    ( RpUnitsList& fromList,
                                              RpUnitsListIter& fromIter,
                                              RpUnitsListIter& toIter   );

        static int compareListEntrySearch   ( RpUnitsList& fromList,
                                              RpUnitsListIter& fromIter,
                                              RpUnitsListIter& toIter   );

        void            connectConversion   ( conversion* conv      ) const;
        void            connectIncarnation  ( const RpUnits* unit   ) const;

        // return the conversion object that will convert
        // from this RpUnits to the proovided toUnits object
        // if the conversion is defined
        int             getConvertFxnList   ( const RpUnits* toUnits,
                                              convertList& cList    ) const;
        static int      applyConversion     ( double* val,
                                              convertList& cList    );
        static int      combineLists        ( convertList& l1,
                                              convertList& l2       );
        static int      printList           ( convertList& l1       );

};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

int list2str (std::list<std::string>& inList, std::string& outString);

int unitSlice (std::string inStr, std::string& outUnits, double& outVal);

#endif // ifdef __cplusplus

#endif // ifndef _RpUNITS_H
