/*
 * ----------------------------------------------------------------------
 *  RpUnits.cc
 *
 *   Data Members and member functions for the RpUnits class
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpUnits.h"

// dict pointer
RpDict<std::string,RpUnits*>* RpUnits::dict = new RpDict<std::string,RpUnits*>();

// install predefined units
static RpUnitsPreset loader;

/**********************************************************************/
// METHOD: define()
/// Define a unit type to be stored as a Rappture Unit.
/**
 */

RpUnits *
RpUnits::define(    const std::string units, 
                    const RpUnits* basis, 
                    const std::string type  ) {

    RpUnits* newRpUnit = NULL;

    std::string searchStr = units;
    std::string sendStr = "";
    int len = searchStr.length();
    int idx = len-1;
    double exponent = 1;

    if (units.empty()) {
        // raise error, user sent null units!
        return NULL;
    }

    // check to see if the user is trying to trick me!
    if ( (basis) && (units == basis->getUnits()) ) {
        // dont trick me!
        return NULL;
    }

    // check to see if the said unit can already be found in the dictionary
    if (RpUnits::find(units)) {
        return NULL;
    }

    //default exponent
    exponent = 1;

    // check to see if there is an exponent at the end 
    // of the search string
    idx = RpUnits::grabExponent(searchStr, &exponent);
    searchStr.erase(idx);

    // move idx pointer back to where last character was found
    idx--;

    if ( searchStr[0] == '/') {
        // need to negate all of the previous exponents
        exponent = -1*exponent;
        sendStr = searchStr.c_str()+1;
    }
    else {
        // sendStr = searchStr.substr(idx+1,);
        // we have a unit string to parse
        sendStr = searchStr;
    }

    newRpUnit = new RpUnits(sendStr, exponent, basis, type);
    if (newRpUnit) {
        insert(newRpUnit->getUnitsName(),newRpUnit);
    }

    // return a copy of the new object to user
    return newRpUnit;
}

/**********************************************************************/
// METHOD: grabExponent()
/// Return exponent from a units string containing a unit name and exponent
/**
 */

int
RpUnits::grabExponent(const std::string& inStr, double* exp) {

    int len = inStr.length();
    int idx = len - 1;

    *exp = 1;

    while (isdigit(inStr[idx])) {
        idx--;
    }

    if ( (inStr[idx] == '+') || (inStr[idx] == '-') ) {
        idx--;
    }

    idx++;

    if (idx != len) {
        // process the exponent.
        *exp = strtod(inStr.c_str()+idx,NULL);
    }

    return idx;
}

/**********************************************************************/
// METHOD: grabUnitString()
/// Return units name from a units string containing a unit name and exponent
/**
 */

int
RpUnits::grabUnitString ( const std::string& inStr ) {

    int idx = inStr.length() - 1;

    while ((idx >= 0) && isalpha(inStr[idx])) {
        idx--;
    }

    // move the index forward one position to
    // represent the start of the unit string
    idx++;

    return idx;
}

/**********************************************************************/
// METHOD: grabUnits()
/// Search for the provided units exponent pair in the dictionary.
/**
 */

const RpUnits*
RpUnits::grabUnits ( std::string inStr, int* offset) {

    const RpUnits* unit = NULL;
    int len = inStr.length();

    while ( ! inStr.empty() ) {
        unit = RpUnits::find(inStr);
        if (unit) {
            *offset = len - inStr.length();
            break;
        }
        inStr.erase(0,1);
    }

    return unit;
}


/**********************************************************************/
// METHOD: getType()
/// Return the type of an RpUnits object.
/**
 */
std::string
RpUnits::getType() const {
    return this->type;
}

/**********************************************************************/
// METHOD: getCompatible()
/// Return a list of units compatible with this RpUnits object.
/**
 */
std::list<std::string>
RpUnits::getCompatible(double expMultiplier) const {

    std::list<std::string> compatList;
    std::list<std::string> basisCompatList;
    std::string myName          = getUnitsName();
    // std::stringstream otherName;
    std::string otherName       = "";
    std::string otherBasisName  = "";
    std::string blank           = "";
    // double otherExp             = 1.0;
    convEntry* myConversions    = this->convList;
    const RpUnits * basis       = NULL;

    if (this->basis) {
        basisCompatList = this->basis->getCompatible();
        compatList.merge(basisCompatList);
    }
    else {
        // only basis units should be in here
        //
        // run through the conversion list
        // for each entry, look at the name
        // if the name is not equal to the name of this RpUnits object,
        // store the fromPtr->getUnitsName() into compatList
        // else store the toPtr->getUnitsName() into compatList
        //
        while (myConversions != NULL) {

            otherName = myConversions->conv->toPtr->getUnitsName();
            /*
            otherName.str("");
            otherName << myConversions->conv->toPtr->getUnitsName();
            otherExp = myConversions->conv->fromPtr->getExponent();
            if ( (otherExp != 1) && (expMultiplier != 1) ) {
                otherName << otherExp * expMultiplier;
            }
            */
            basis = myConversions->conv->toPtr->basis;

            // if (myName == otherName.str()) {
            if (myName == otherName) {
                otherName = myConversions->conv->fromPtr->getUnitsName();
                /*
                otherName.str("");
                otherName << myConversions->conv->fromPtr->getUnitsName();
                if ( (otherExp != 1) && (expMultiplier != 1) ) {
                    otherExp = myConversions->conv->fromPtr->getExponent();
                    otherName << otherExp * expMultiplier;
                }
                */
                basis = myConversions->conv->fromPtr->basis;
            }

            // check to see if they are the same basis, 
            // no need to list all of the metric conversions.
            if (basis) {
                if (basis->getUnitsName() == myName) {
                    // do not add this unit to the conversion
                    // because its a derived unit.
                    myConversions = myConversions->next;
                    continue;
                }
            }

            // add the other unit's name to the list of compatible units
            // compatList.push_back(otherName.str());
            compatList.push_back(otherName);

            // advance to the next conversion
            myConversions = myConversions->next;
        }
    }

    compatList.push_back(myName);
    compatList.sort();
    compatList.unique();
    return compatList;

}

/**********************************************************************/
// METHOD: define()
/// Define a unit conversion with one arg double function pointers.
/**
 */

RpUnits *
RpUnits::define(  const RpUnits* from,
                  const RpUnits* to,
                  double (*convForwFxnPtr)(double),
                  double (*convBackFxnPtr)(double)  ) {

    // this is kinda the wrong way to get the job done...
    // how do we only create 1 conversion object and share it between atleast two RpUnits
    // objs so that when the RpUnits objs are deleted, we are not trying to delete already
    // deleted memory.
    // so for the sake of safety we get the following few lines of code.

    conversion* conv1 = NULL;
    conversion* conv2 = NULL;

    if (from && to) {

        conv1 = new conversion (from,to,convForwFxnPtr,convBackFxnPtr);
        conv2 = new conversion (from,to,convForwFxnPtr,convBackFxnPtr);

        from->connectConversion(conv1);
        to->connectConversion(conv2);
    }

    return NULL;
}

/**********************************************************************/
// METHOD: define()
/// Define a unit conversion with two arg double function pointers.
/**
 */

RpUnits *
RpUnits::define(  const RpUnits* from,
                  const RpUnits* to,
                  double (*convForwFxnPtr)(double,double),
                  double (*convBackFxnPtr)(double,double)) {

    // this is kinda the wrong way to get the job done...
    // how do we only create 1 conversion object and share it between 
    // atleast two RpUnits objs so that when the RpUnits objs are 
    // deleted, we are not trying to delete already deleted memory.
    // so for the sake of safety we get the following few lines of code.

    conversion* conv1 = NULL;
    conversion* conv2 = NULL;

    if (from && to) {
        conv1 = new conversion (from,to,convForwFxnPtr,convBackFxnPtr);
        conv2 = new conversion (from,to,convForwFxnPtr,convBackFxnPtr);

        from->connectConversion(conv1);
        to->connectConversion(conv2);
    }

    return NULL;
}

/**********************************************************************/
// METHOD: define()
/// Define a unit conversion with two arg void* function pointers.
/**
 */

RpUnits *
RpUnits::define(  const RpUnits* from,
                  const RpUnits* to,
                  void* (*convForwFxnPtr)(void*, void*),
                  void* convForwData,
                  void* (*convBackFxnPtr)(void*, void*),
                  void* convBackData) {

    // this is kinda the wrong way to get the job done...
    // how do we only create 1 conversion object and share it between at
    // least two RpUnits objs so that when the RpUnits objs are deleted, 
    // we are not trying to delete already deleted memory.
    // so for the sake of safety we get the following few lines of code.

    conversion* conv1 = NULL;
    conversion* conv2 = NULL;

    if (from && to) {
        conv1 = new conversion ( from, to, convForwFxnPtr,convForwData,
                                 convBackFxnPtr,convBackData);
        conv2 = new conversion ( from,to,convForwFxnPtr,convForwData,
                                 convBackFxnPtr,convBackData);

        from->connectConversion(conv1);
        to->connectConversion(conv2);
    }

    return NULL;
}


/**********************************************************************/
// METHOD: getUnits()
/// Report the text portion of the units of this object back to caller.
/**
 * \sa {getUnitsName}
 */

std::string
RpUnits::getUnits() const {

    return units;
}

/**********************************************************************/
// METHOD: getUnitsName()
/// Report the full name of the units of this object back to caller.
/**
 * Reports the full text and exponent of the units represented by this
 * object, back to the caller. Note that if the exponent == 1, no
 * exponent will be printed.
 */

std::string
RpUnits::getUnitsName() const {

    std::stringstream unitText;
    double exponent;

    exponent = getExponent();

    if (exponent == 1) {
        unitText << units;
    }
    else {
        unitText << units << exponent;
    }

    return (std::string(unitText.str()));
}

/**********************************************************************/
// METHOD: getExponent()
/// Report the exponent of the units of this object back to caller.
/**
 * Reports the exponent of the units represented by this
 * object, back to the caller. Note that if the exponent == 1, no
 * exponent will be printed.
 */

double
RpUnits::getExponent() const {

    return exponent;
}

/**********************************************************************/
// METHOD: getBasis()
/// Retrieve the RpUnits object representing the basis of this object.
/**
 * Returns a pointer to a RpUnits object which, on success, points to the 
 * RpUnits object that is the basis of the calling object.
 */

const RpUnits *
RpUnits::getBasis() const {

    return basis;
}

/**********************************************************************/
// METHOD: makeBasis()
/// Convert a value into its RpUnits's basis.
/**
 *  convert the current unit to its basis units
 *
 *  Return Codes
 *      0) no error (could also mean or no prefix was found)
 *          in some cases, this means the value is in its basis format
 *      1) the prefix found does not have a built in factor associated.
 *
 */

double
RpUnits::makeBasis(double value, int* result) const {

    double retVal = value;

    if (result) {
        *result = 0;
    }

    if (basis == NULL) {
        // this unit is a basis
        // do nothing
    }
    else {
        retVal = convert(basis,value,result);
    }

    return retVal;
}

/**********************************************************************/
// METHOD: makeBasis()
/// Convert a value into its RpUnits's basis.
/**
 *
 */

const RpUnits&
RpUnits::makeBasis(double* value, int* result) const {
    double retVal = *value;
    int convResult = 1;

    if (basis == NULL) {
        // this unit is a basis
        // do nothing
    }
    else {
        retVal = convert(basis,retVal,&convResult);
    }

    if ( (convResult == 0) ) {
        *value = retVal;
    }

    if (result) {
        *result = convResult;
    }

    return *this;
}

/**********************************************************************/
// METHOD: makeMetric()
/// Define a unit type to be stored as a Rappture Unit.
/**
 *  static int makeMetric(RpUnits * basis);
 *  create the metric attachments for the given basis.
 *  should only be used if this unit is of metric type
 */

int
RpUnits::makeMetric(const RpUnits* basis) {

    if (!basis) {
        return 0;
    }

    std::string basisName = basis->getUnitsName();
    std::string name;

    name = "c" + basisName;
    RpUnits * centi = RpUnits::define(name, basis, basis->type);
    RpUnits::define(centi, basis, centi2base, base2centi);

    name = "m" + basisName;
    RpUnits * milli = RpUnits::define(name, basis, basis->type);
    RpUnits::define(milli, basis, milli2base, base2milli);

    name = "u" + basisName;
    RpUnits * micro = RpUnits::define(name, basis, basis->type);
    RpUnits::define(micro, basis, micro2base, base2micro);

    name = "n" + basisName;
    RpUnits * nano  = RpUnits::define(name, basis, basis->type);
    RpUnits::define(nano, basis, nano2base, base2nano);

    name = "p" + basisName;
    RpUnits * pico  = RpUnits::define(name, basis, basis->type);
    RpUnits::define(pico, basis, pico2base, base2pico);

    name = "f" + basisName;
    RpUnits * femto = RpUnits::define(name, basis, basis->type);
    RpUnits::define(femto, basis, femto2base, base2femto);

    name = "a" + basisName;
    RpUnits * atto  = RpUnits::define(name, basis, basis->type);
    RpUnits::define(atto, basis, atto2base, base2atto);

    name = "k" + basisName;
    RpUnits * kilo  = RpUnits::define(name, basis, basis->type);
    RpUnits::define(kilo, basis, kilo2base, base2kilo);

    name = "M" + basisName;
    RpUnits * mega  = RpUnits::define(name, basis, basis->type);
    RpUnits::define(mega, basis, mega2base, base2mega);

    name = "G" + basisName;
    RpUnits * giga  = RpUnits::define(name, basis, basis->type);
    RpUnits::define(giga, basis, giga2base, base2giga);

    name = "T" + basisName;
    RpUnits * tera  = RpUnits::define(name, basis, basis->type);
    RpUnits::define(tera, basis, tera2base, base2tera);

    name = "P" + basisName;
    RpUnits * peta  = RpUnits::define(name, basis, basis->type);
    RpUnits::define(peta, basis, peta2base, base2peta);

    return (1);
}


/**********************************************************************/
// METHOD: find()
/// Find an RpUnits Object from the provided string.
/**
 */

const RpUnits*
RpUnits::find(std::string key) {

    // dict pointer
    const RpUnits* unitEntry = NULL;
    double exponent = 1;
    int idx = 0;
    std::stringstream tmpKey;

    if (key[0] == '/') {
        // check to see if there is an exponent at the end
        // of the search string
        idx = RpUnits::grabExponent(key, &exponent);
        tmpKey << key.substr(1,idx-1) << (-1*exponent);
        key = tmpKey.str();
    }

    unitEntry = *(dict->find(key).getValue());

    // dict pointer
    if (unitEntry == *(dict->getNullEntry().getValue()) ) {
        unitEntry = NULL;
    }

    return unitEntry;
}

/**********************************************************************/
// METHOD: validate()
/// Split a string of units and check that each unit is available as an object
/**
 * Splits a string of units like cm2/kVns into a list of units like
 * cm2, kV1, ns1 where an exponent is provided for each list entry.
 * It checks to see that each unit actually exists as a valid defined unit.
 * If the unit exists or can be interpreted, the function keeps parsing the
 * string until it reaches the end of the string. If the function comes
 * across a unit that is unrecognized or can not be interpreted, then it
 * returns error (a non-zero value).
 * 
 * this code is very similar to units2list()
 *
 * if &compatList == NULL, no compatible list of units will be generated.
 * this function does not do a good job of placing the available units 
 * back into the original formula. i still need to work on this.
 */

int
RpUnits::validate ( const std::string& inUnits,
                    std::string& type,
                    std::list<std::string>* compatList ) {

    std::string myInUnits   = inUnits;
    std::string sendUnitStr = "";
    double exponent         = 1;
    int offset              = 0;
    int idx                 = 0;
    int last                = 0;
    int err                 = 0; // did we come across an unrecognized unit
    const RpUnits* unit     = NULL;
    std::list<std::string> basisCompatList;
    std::list<std::string>::iterator compatListIter;
    std::stringstream unitWExp;


    while ( !myInUnits.empty() ) {

        // check to see if we came across a '/' character
        last = myInUnits.length()-1;
        if (myInUnits[last] == '/') {
            type = myInUnits[last] + type;
            myInUnits.erase(last);
            continue;
        }

        // get the exponent
        offset = RpUnits::grabExponent(myInUnits,&exponent);
        myInUnits.erase(offset);
        idx = offset - 1;

        // grab the largest string we can find
        offset = RpUnits::grabUnitString(myInUnits);
        idx = offset;

        // figure out if we have some defined units in that string
        sendUnitStr = myInUnits.substr(offset,std::string::npos);
        if ((unit = grabUnits(sendUnitStr,&offset))) {
            // a unit was found
            // erase the found unit's name from our search string
            myInUnits.erase(idx+offset);

            // add the type to the type string
            type = unit->getType() + type;

            // merge the compatible units
            if (compatList) {
                basisCompatList = unit->getCompatible(exponent);

                // adjust exponents as necessary
                if ( (exponent != 0) && (exponent != 1) ) {
                    compatListIter = compatList->begin();
                    while (compatListIter != compatList->end()) {
                        unitWExp << *compatListIter << exponent;
                        *compatListIter = unitWExp.str();
                    }
                }

                compatList->merge(basisCompatList);
            }
        }
        else {
            // we came across a unit we did not recognize
            // raise error and exit
            err++;
            break;
        }

        // reset our vars
        idx = 0;
        offset = 0;
        exponent = 1;
    }

    // clean out any duplicate entries.
    if (compatList) {
        compatList->unique();
    }

    return err;
}


/**********************************************************************/
// METHOD: negateListExponents()
/// Negate the exponents on every element in unitsList
/**
 */

int
RpUnits::negateListExponents(RpUnitsList& unitsList) {
    RpUnitsListIter iter = unitsList.begin();
    int nodeCnt = unitsList.size();

    if (nodeCnt > 0) {
        for (; iter != unitsList.end(); iter++) {
            iter->negateExponent();
            nodeCnt--;
        }
    }

    return nodeCnt;
}

/**********************************************************************/
// METHOD: negateExponent()
/// Negate the exponent on the current RpUnitsListEntry
/**
 */

void
RpUnitsListEntry::negateExponent() const {
    exponent = exponent * -1;
    return;
}

/**********************************************************************/
// METHOD: name()
/// Provide the caller with the name of this object
/**
 */

std::string
RpUnitsListEntry::name() const {
    std::stringstream name;
    name << unit->getUnits() << exponent;
    return std::string(name.str());
}

/**********************************************************************/
// METHOD: define()
/// Provide the caller with the basis of the RpUnits object being stored
/**
 */

const RpUnits*
RpUnitsListEntry::getBasis() const {
    return unit->getBasis();
}

/**********************************************************************/
// METHOD: getUnitsObj()
/// Return the RpUnits Object from a RpUnitsListEntry.
/**
 */

const RpUnits*
RpUnitsListEntry::getUnitsObj() const {
    return unit;
}

/**********************************************************************/
// METHOD: getExponent()
/// Return the exponent of an RpUnitsListEntry.
/**
 */

double
RpUnitsListEntry::getExponent() const {
    return exponent;
}

/**********************************************************************/
// METHOD: printList()
/// Traverse a RpUnitsList and print out the name of each element.
/**
 */

int
RpUnits::printList(RpUnitsList& unitsList) {
    RpUnitsListIter iter = unitsList.begin();
    int nodeCnt = unitsList.size();

    if (nodeCnt > 0) {
        for (; iter != unitsList.end(); iter++) {
            std::cout << iter->name() << " ";
            nodeCnt--;
        }
        std::cout << std::endl;
    }

    return nodeCnt;
}

/**********************************************************************/
// METHOD: units2list()
/// Split a string of units into a list of base units with exponents.
/**
 * Splits a string of units like cm2/kVns into a list of units like
 * cm2, kV1, ns1 where an exponent is provided for each list entry.
 * List entries are found by comparing units strings to the names
 * in the dictionary.
 */

int
RpUnits::units2list ( const std::string& inUnits,
                      RpUnitsList& outList ) {

    std::string myInUnits   = inUnits;
    std::string sendUnitStr = "";
    double exponent         = 1;
    int offset              = 0;
    int idx                 = 0;
    int last                = 0;
    const RpUnits* unit     = NULL;


    while ( !myInUnits.empty() ) {

        // check to see if we came across a '/' character
        last = myInUnits.length()-1;
        if (myInUnits[last] == '/') {
            myInUnits.erase(last);
            // multiply previous exponents by -1
            if ( ! outList.empty() ) {
                RpUnits::negateListExponents(outList);
            }
            continue;
        }

        // get the exponent
        offset = RpUnits::grabExponent(myInUnits,&exponent);
        myInUnits.erase(offset);
        idx = offset - 1;

        // grab the largest string we can find
        offset = RpUnits::grabUnitString(myInUnits);
        idx = offset;

        // figure out if we have some defined units in that string
        sendUnitStr = myInUnits.substr(offset,std::string::npos);
        unit = grabUnits(sendUnitStr,&offset);
        if (unit) {
            // a unit was found
            // add this unit to the list
            // erase the found unit's name from our search string
            outList.push_front(RpUnitsListEntry(unit,exponent));
            myInUnits.erase(idx+offset);
        }
        else {
            // we came across a unit we did not recognize
            // raise error and delete character for now
            myInUnits.erase(last);
        }

        // reset our vars
        idx = 0;
        offset = 0;
        exponent = 1;
    }

    return 0;
}

/**********************************************************************/
// METHOD: compareListEntryBasis()
/// Compare two RpUnits objects to see if they are related by a basis
/**
 * One step in converting between Rappture Units Objects is to check
 * to see if the conversion is an intra-basis conversion. Intra-basis
 * conversions include those where all conversions are done within
 * the same basis.
 *
 * Examples of intra-basis conversions include:
 *     m -> cm  ( meters to centimeters )
 *     cm -> m  ( centimeters to meters )
 *     cm -> nm ( centimenters to nanometers )
 */

int RpUnits::compareListEntryBasis ( RpUnitsList& fromList,
                                     RpUnitsListIter& fromIter,
                                     RpUnitsListIter& toIter ) {

    const RpUnits* toBasis = NULL;
    const RpUnits* fromBasis = NULL;
    int retVal = 1;
    double fromExp = 0;
    double toExp = 0;

    fromIter = fromList.begin();

    // get the basis of the object being stored
    // if the basis is NULL, then we'll compare the object
    // itself because the object is the basis.
    toBasis = toIter->getBasis();
    if (toBasis == NULL) {
        toBasis = toIter->getUnitsObj();
    }

    toExp   = toIter->getExponent();

    while ( fromIter != fromList.end() ) {

        fromExp = fromIter->getExponent();

        // in order to convert, exponents must be equal.
        if (fromExp == toExp) {

            // get the basis of the object being stored
            // if the basis is NULL, then we'll compare the object
            // itself because the object is the basis.
            fromBasis = fromIter->getBasis();
            if (fromBasis == NULL) {
                fromBasis = fromIter->getUnitsObj();
            }

            if (toBasis == fromBasis) {
                // conversion needed between 2 units of the same basis.
                // these two units could actually be the same unit (m->m)
                retVal = 0;
                break;
            }
        }

        fromIter++;
    }

    return retVal;
}

/**********************************************************************/
// METHOD: compareListEntrySearch()
/// this function will soon be removed.
/**
 */

int RpUnits::compareListEntrySearch ( RpUnitsList& fromList,
                                     RpUnitsListIter& fromIter,
                                     RpUnitsListIter& toIter ) {

    const RpUnits* toBasis = NULL;
    const RpUnits* fromBasis = NULL;
    int retVal = 1;

    fromIter = fromList.begin();

    // get the basis of the object being stored
    // if the basis is NULL, then we'll compare the object
    // itself because the object is the basis.
    toBasis = toIter->getBasis();
    if (toBasis == NULL) {
        toBasis = toIter->getUnitsObj();
    }

    while ( fromIter != fromList.end() ) {

        // get the basis of the object being stored
        // if the basis is NULL, then we'll compare the object
        // itself because the object is the basis.
        fromBasis = fromIter->getBasis();
        if (fromBasis == NULL) {
            fromBasis = fromIter->getUnitsObj();
        }

        if (toBasis == fromBasis) {
            // conversion needed between 2 units of the same basis.
            // these two units could actually be the same unit (m->m)
            retVal = 0;
            break;
        }

        fromIter++;
    }

    return retVal;
}

/**********************************************************************/
// METHOD: convert()
/// Convert between RpUnits return a string value with or without units
/**
 * Convert function so people can just send in two strings and
 * we'll see if the units exists and do a conversion
 * Example:
 *     strVal = RpUnits::convert("300K","C",1);
 *
 * Returns a string with or without units.
 */

std::string
RpUnits::convert (  std::string val,
                    std::string toUnitsName,
                    int showUnits,
                    int* result ) {

    RpUnitsList toUnitsList;
    RpUnitsList fromUnitsList;

    RpUnitsListIter toIter;
    RpUnitsListIter fromIter;
    RpUnitsListIter tempIter;

    const RpUnits* toUnits = NULL;
    const RpUnits* fromUnits = NULL;

    std::string tmpNumVal = "";
    std::string fromUnitsName = "";
    std::string convVal = "";
    double origNumVal = 0;
    double numVal = 0;
    double toExp = 0;
    double fromExp = 0;
    int convResult = 0;
    char* endptr = NULL;
    std::stringstream outVal;

    int rv = 0;
    double factor = 1;


    // set  default result flag/error code
    if (result) {
        *result = 0;
    }

    // search our string to see where the numeric part stops
    // and the units part starts
    //
    //  convert("5J", "neV") => 3.12075e+28neV
    //  convert("3.12075e+28neV", "J") => 4.99999J
    // now we can actually get the scientific notation portion of the string.
    //

    numVal = strtod(val.c_str(),&endptr);
    origNumVal = numVal;

    if ( (numVal == 0) && (endptr == val.c_str()) ) {
        // no conversion was done.
        // number in incorrect format probably.
        if (result) {
            *result = 1;
        }
        return val;
    }

    fromUnitsName = std::string(endptr);

    if (toUnitsName.empty())  {
        // there were no units in the input 
        // string or no conversion needed
        // assume fromUnitsName = toUnitsName
        // return the correct value
        if (result) {
            *result = 0;
        }

        if (showUnits) {
            outVal << numVal << fromUnitsName;
        }
        else {
            outVal << numVal;
        }

        return std::string(outVal.str());
    }

    // check if the fromUnitsName is empty or
    // if the fromUnitsName == toUnitsName
    // these are conditions where no conversion is needed
    if ( (fromUnitsName.empty()) || (toUnitsName == fromUnitsName) )  {
        // there were no units in the input 
        // string or no conversion needed
        // assume fromUnitsName = toUnitsName
        // return the correct value
        if (result) {
            *result = 0;
        }

        if (showUnits) {
            outVal << numVal << toUnitsName;
        }
        else {
            outVal << numVal;
        }

        return std::string(outVal.str());
    }

    RpUnits::units2list(toUnitsName,toUnitsList);
    RpUnits::units2list(fromUnitsName,fromUnitsList);

    toIter = toUnitsList.begin();

    // pass 1: compare basis' of objects to find intra-basis conversions
    while ( toIter != toUnitsList.end() ) {
        rv = RpUnits::compareListEntryBasis(fromUnitsList, fromIter, toIter);
        if (rv == 0) {

            // check the names of the units provided by the user
            // if the names are the same, no need to do a conversion
            if (fromIter->name() != toIter->name()) {

                // do an intra-basis conversion
                toUnits = toIter->getUnitsObj();
                fromUnits = fromIter->getUnitsObj();

                // do conversions
                factor = 1;
                factor = fromUnits->convert(toUnits, factor, &convResult);
                numVal *= pow(factor,toIter->getExponent());
            }

            // remove the elements from the lists
            tempIter = toIter;
            toIter = toUnitsList.erase(tempIter);
//            toUnitsList.erase(tempIter);
//            toIter++;

            tempIter = fromIter;
            fromUnitsList.erase(tempIter);
        }
        else {
            // this is not an intra-basis conversion.
            // move onto the next toIter
            toIter++;
        }
    }

    toIter = toUnitsList.begin();
    fromIter = fromUnitsList.begin();

    // pass 2: look for inter-basis conversions
    if (fromIter != fromUnitsList.end()) {
        // double while loop to compare each toIter with each fromIter.
        // the outter while checks the toIter and the inner while
        // which is conveniently hidden, adjusts the fromIter and toIter
        // (at the bottom in the else statement).
        while (toIter != toUnitsList.end()) {

            toUnits = toIter->getUnitsObj();
            fromUnits = fromIter->getUnitsObj();

            // do an inter-basis conversion...the slow way
            // there has to be a better way to do this...
            convResult = 1;

            // in order to convert, exponents must be equal.
            fromExp = fromIter->getExponent();
            toExp   = toIter->getExponent();

            if (fromExp == toExp) {
                if (toExp == 1) {
                    numVal = fromUnits->convert(toUnits, numVal, &convResult);
                }
                else {
                    factor = 1;
                    factor = fromUnits->convert(toUnits, factor, &convResult);
                    numVal *= pow(factor,toExp);
                }
            }

            if (convResult == 0) {
                // successful conversion reported
                // remove the elements from the lists
                tempIter = toIter;
                toUnitsList.erase(tempIter);
                toIter++;

                tempIter = fromIter;
                fromUnitsList.erase(tempIter);

                // conversion complete, jump out of the 
                // while loop
                break;
            }
            else {
                // conversion was unsuccessful
                // move onto the next fromIter
                fromIter++;
                if (fromIter == fromUnitsList.end()) {
                    // this is not an inter-basis conversion.
                    // move onto the next toIter
                    fromIter = fromUnitsList.begin();
                    toIter++;
                }
            } // end unsuccessful conversion
        } // end toIter while loop
    } // end



    if ( (result) && (*result == 0) ) {
        *result = convResult;
    }

    if (showUnits) {
        outVal << numVal << toUnitsName;
    }
    else {
        outVal << numVal;
    }

    return std::string(outVal.str());

}

/**********************************************************************/
// METHOD: convert()
/// Convert between RpUnits return a string value with or without units
/**
 * Returns a string value with or without units.
 */

std::string
RpUnits::convert ( const  RpUnits* toUnits,
                   double val,
                   int showUnits,
                   int* result )  const {

    double retVal = convert(toUnits,val,result);
    std::stringstream unitText;


    if (showUnits) {
        unitText << retVal << toUnits->getUnitsName();
    }
    else {
        unitText << retVal;
    }

    return (std::string(unitText.str())); 

}

/**********************************************************************/
// METHOD: convert()
/// Convert between RpUnits using an RpUnits Object to describe toUnit.
/**
 * User function to convert a value to the provided RpUnits* toUnits
 * if it exists as a conversion from the basis
 * example
 *      cm.convert(meter,10)
 *      cm.convert(angstrum,100)
 *
 * Returns a double value without units.
 */

double
RpUnits::convert(const RpUnits* toUnit, double val, int* result) const {

    // currently we convert this object to its basis and look for the
    // connection to the toUnit object from the basis.

    double value = val;
    const RpUnits* toBasis = toUnit->getBasis();
    const RpUnits* fromUnit = this;
    const RpUnits* dictToUnit = NULL;
    convEntry *p;
    int my_result = 0;

    // set *result to a default value
    if (result) {
        *result = 1;
    }

    // guard against converting to the units you are converting from...
    // ie. meters->meters
    if (this->getUnitsName() == toUnit->getUnitsName()) {
        if (result) {
            *result = 0;
        }
        return val;
    }

    // convert unit to the basis
    // makeBasis(&value);
    // trying to avoid the recursive way of converting to the basis.
    // need to rethink this.
    // 
    if ( (basis) && (basis->getUnitsName() != toUnit->getUnitsName()) ) {
        value = convert(basis,value,&my_result);
        if (my_result == 0) {
            fromUnit = basis;
        }
    }

    // find the toUnit in our dictionary.
    // if the toUnits has a basis, we need to search for the basis
    // and convert between basis' and then convert again back to the 
    // original unit.
    if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
        dictToUnit = find(toBasis->getUnitsName());
    }
    else {
        dictToUnit = find(toUnit->getUnitsName());
    }

    // did we find the unit in the dictionary?
    if (dictToUnit == NULL) {
        // toUnit was not found in the dictionary
        return val;
    }

    // search through the conversion list to find
    // the conversion to the toUnit.

    if (basis) {
        p = basis->convList;
    }
    else {
        p = this->convList;
    }

    if (p == NULL) {
        // there are no conversions
        return val;
    }

    // loop through our conversion list looking for the correct conversion
    do {

        if ( (p->conv->toPtr == dictToUnit) && (p->conv->fromPtr == fromUnit) ) {
            // we found our conversion
            // call the function pointer with value

            // this should probably be re thought out
            // the problem is that convForwFxnPtr has the conversion for a
            // one arg conv function pointer and convForwFxnPtrDD has the
            // conversion for a two arg conv function pointer
            // need to make this simpler, more logical maybe only allow 2 arg
            if (       (p->conv->convForwFxnPtr) 
                    && (! p->conv->convForwFxnPtrDD) ) {

                value = p->conv->convForwFxnPtr(value);
            }
            else if (  (p->conv->convForwFxnPtrDD) 
                    && (! p->conv->convForwFxnPtr) ) {

                value = 
                    p->conv->convForwFxnPtrDD(value, fromUnit->getExponent());
            }

            // check to see if we converted to the actual requested unit
            // or to the requested unit's basis.
            // if we converted to the requested unit's basis. we need to
            // do one last conversion from the requested unit's basis back 
            // to the requested unit.
            if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
                my_result = 0;
                value = toBasis->convert(toUnit,value,&my_result);
                if (my_result != 0) {
                    if (result) {
                        *result += 1;
                    }
                }
            }

            // change the result code to zero, a conversion was performed
            // (we think)... its ture that it is possible to get to this 
            // point and have skipped the conversion because the 
            // conversion object was not properly created...
            // ie. both fxn ptrs were null or neither fxn ptr was null
            //
            if (result && (*result == 1)) {
                *result = 0;
            }
            break;
        }

        if ( (p->conv->toPtr == fromUnit) && (p->conv->fromPtr == dictToUnit) ) {
            // we found our conversion
            // call the function pointer with value

            // this should probably be re thought out
            // the problem is that convForwFxnPtr has the conversion for a
            // one arg conv function pointer and convForwFxnPtrDD has the
            // conversion for a two arg conv function pointer
            // need to make this simpler, more logical maybe only allow 2 arg
            if (       (p->conv->convBackFxnPtr) 
                    && (! p->conv->convBackFxnPtrDD) ) {

                value = p->conv->convBackFxnPtr(value);
            }
            else if (  (p->conv->convBackFxnPtrDD) 
                    && (! p->conv->convBackFxnPtr) ) {

                value = 
                    p->conv->convBackFxnPtrDD(value, fromUnit->getExponent());
            }

            // check to see if we converted to the actual requested unit
            // or to the requested unit's basis.
            // if we converted to the requested unit's basis. we need to
            // do one last conversion from the requested unit's basis back 
            // to the requested unit.
            if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
                my_result = 0;
                value = toBasis->convert(toUnit,value,&my_result);
                if (my_result != 0) {
                    if (result) {
                        *result += 1;
                    }
                }
            }

            // change the result code to zero, a conversion was performed
            // (we think)... its ture that it is possible to get to this 
            // point and have skipped the conversion because the 
            // conversion object was not properly created...
            // ie. both fxn ptrs were null or neither fxn ptr was null
            //
            if (result && (*result == 1)) {
                *result = 0;
            }
            break;
        }

        p = p->next;

    } while (p != NULL);


    if ( p == NULL) {
        // we did not find the conversion
        if (result) {
            *result += 1;
        }
        return val;
    }

    // we found the conversion.
    // return the converted value.
    return value;

}


/**********************************************************************/
// METHOD: convert()
/// Convert a value between RpUnits using user defined conversions
/**
 */

void*
RpUnits::convert(const RpUnits* toUnit, void* val, int* result) const {

    // currently we convert this object to its basis and look for the
    // connection ot the toUnit object from the basis.

    void* value = val;
    const RpUnits* toBasis = toUnit->getBasis();
    const RpUnits* fromUnit = this;
    const RpUnits* dictToUnit = NULL;
    convEntry *p;
    int my_result = 0;

    // set *result to a default value
    if (result) {
        *result = 1;
    }

    // guard against converting to the units you are converting from...
    // ie. meters->meters
    if (this->getUnitsName() == toUnit->getUnitsName()) {
        if (result) {
            *result = 0;
        }
        return val;
    }

    // convert unit to the basis
    // makeBasis(&value);
    // trying to avoid the recursive way of converting to the basis.
    // need to rethink this.
    //
    if ( (basis) && (basis->getUnitsName() != toUnit->getUnitsName()) ) {
        value = convert(basis,value,&my_result);
        if (my_result == 0) {
            fromUnit = basis;
        }
    }

    // find the toUnit in our dictionary.
    // if the toUnits has a basis, we need to search for the basis
    // and convert between basis' and then convert again back to the 
    // original unit.
    if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
        dictToUnit = find(toBasis->getUnitsName());
    }
    else {
        dictToUnit = find(toUnit->getUnitsName());
    }

    // did we find the unit in the dictionary?
    if (dictToUnit == NULL) {
        // toUnit was not found in the dictionary
        return val;
    }

    // search through the conversion list to find
    // the conversion to the toUnit.

    if (basis) {
        p = basis->convList;
    }
    else {
        p = this->convList;
    }

    if (p == NULL) {
        // there are no conversions
        return val;
    }

    // loop through our conversion list looking for the correct conversion
    do {

        if ( (p->conv->toPtr == dictToUnit) && (p->conv->fromPtr == fromUnit) ) {
            // we found our conversion
            // call the function pointer with value

            value = p->conv->convForwFxnPtrVoid(p->conv->convForwData,value);

            // check to see if we converted to the actual requested unit
            // or to the requested unit's basis.
            // if we converted to the requested unit's basis. we need to
            // do one last conversion from the requested unit's basis back 
            // to the requested unit.
            if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
                my_result = 0;
                value = toBasis->convert(toUnit,value,&my_result);
                if (my_result != 0) {
                    if (result) {
                        *result += 1;
                    }
                }
            }

            // change the result code to zero, a conversion was performed
            // (we think)... its ture that it is possible to get to this 
            // point and have skipped the conversion because the 
            // conversion object was not properly created...
            // ie. both fxn ptrs were null or neither fxn ptr was null
            //
            if (result && (*result == 1)) {
                *result = 0;
            }
            break;
        }

        if ( (p->conv->toPtr == fromUnit) && (p->conv->fromPtr == dictToUnit) ) {
            // we found our conversion
            // call the function pointer with value

            value = p->conv->convBackFxnPtrVoid(p->conv->convBackData,value);

            // check to see if we converted to the actual requested unit
            // or to the requested unit's basis.
            // if we converted to the requested unit's basis. we need to
            // do one last conversion from the requested unit's basis back 
            // to the requested unit.
            if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
                my_result = 0;
                value = toBasis->convert(toUnit,value,&my_result);
                if (my_result != 0) {
                    if (result) {
                        *result += 1;
                    }
                }
            }

            // change the result code to zero, a conversion was performed
            // (we think)... its ture that it is possible to get to this 
            // point and have skipped the conversion because the 
            // conversion object was not properly created...
            // ie. both fxn ptrs were null or neither fxn ptr was null
            //
            if (result && (*result == 1)) {
                *result = 0;
            }
            break;
        }

        p = p->next;

    } while (p != NULL);


    if ( p == NULL) {
        // we did not find the conversion
        if (result) {
            *result += 1;
        }
        return val;
    }

    // we found the conversion.
    // return the converted value.
    return value;

}

/**********************************************************************/
// METHOD: insert()
/// Place an RpUnits Object into the Rappture Units Dictionary.
/**
 * Return whether the inserted key was new with a non-zero 
 * value, or if the key already existed with a value of zero.
 */

int
insert(std::string key,RpUnits* val) {

    int newRecord = 0;
    // RpUnits* val = this;
    // dict pointer
    RpUnits::dict->set(key,val,&newRecord);
    return newRecord;
}

/**********************************************************************/
// METHOD: connectConversion()
/// Attach conversion information to a RpUnits Object.
/**
 */

void
RpUnits::connectConversion(conversion* conv) const {

    convEntry* p = convList;

    if (p == NULL) {
        convList = new convEntry (conv,NULL,NULL);
    }
    else {
        while (p->next != NULL) {
            p = p->next;
        }

        p->next = new convEntry (conv,p,NULL);
    }

}

/**********************************************************************/
// METHOD: addPresets()
/// Add a specific set of predefined units to the dictionary
/**
 */

int
RpUnits::addPresets (const std::string group) {
    int retVal = -1;
    if (group.compare("all") == 0) {
        retVal = RpUnitsPreset::addPresetAll();
    }
    else if (group.compare("energy") == 0) {
        retVal = RpUnitsPreset::addPresetEnergy();
    }
    else if (group.compare("length") == 0) {
        retVal = RpUnitsPreset::addPresetLength();
    }
    else if (group.compare("temp") == 0) {
        retVal = RpUnitsPreset::addPresetTemp();
    }
    else if (group.compare("time") == 0) {
        retVal = RpUnitsPreset::addPresetTime();
    }
    else if (group.compare("volume") == 0) {
        retVal = RpUnitsPreset::addPresetVolume();
    }
    else if (group.compare("angle") == 0) {
        retVal = RpUnitsPreset::addPresetAngle();
    }

    return retVal;
}

/**********************************************************************/
// METHOD: addPresetAll()
/// Call all of the addPreset* functions.
/**
 *
 * Add all predefined units to the units dictionary
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetAll () {

    int result = 0;

    result += addPresetTime();
    result += addPresetTemp();
    result += addPresetLength();
    result += addPresetEnergy();
    result += addPresetVolume();
    result += addPresetAngle();
    result += addPresetMass();
    result += addPresetMisc();

    return 0;
}

/**********************************************************************/
// METHOD: addPresetTime()
/// Add Time related units to the dictionary
/**
 * Defines the following units:
 *   seconds  (s)
 *   minutes  (min)
 *   hours    (h)
 *   days     (d)
 *
 *   month and year are not included because simple 
 *   day->month conversions may be misleading
 *   month->year conversions may be included in the future
 * 
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetTime () {

    RpUnits* second    = RpUnits::define("s", NULL, RP_TYPE_TIME);
    RpUnits* minute    = RpUnits::define("min", second, RP_TYPE_TIME);
    RpUnits* hour      = RpUnits::define("h", second, RP_TYPE_TIME);
    RpUnits* day       = RpUnits::define("d", second, RP_TYPE_TIME);

    RpUnits::makeMetric(second);

    // add time definitions

    RpUnits::define(second, minute, sec2min, min2sec);
    RpUnits::define(second, hour, sec2hour, hour2sec);
    RpUnits::define(second, day, sec2day, day2sec);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetTemp()
/// Add Temperature related units to the dictionary
/**
 * Defines the following units:
 *   fahrenheit  (F)
 *   celcius     (C)
 *   kelvin      (K)
 *   rankine     (R)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetTemp () {

    RpUnits* fahrenheit = RpUnits::define("F", NULL, RP_TYPE_TEMP);
    RpUnits* celcius    = RpUnits::define("C", NULL, RP_TYPE_TEMP);
    RpUnits* kelvin     = RpUnits::define("K", NULL, RP_TYPE_TEMP);
    RpUnits* rankine    = RpUnits::define("R", NULL, RP_TYPE_TEMP);

    // add temperature definitions
    RpUnits::define(fahrenheit, celcius, fahrenheit2centigrade, centigrade2fahrenheit);
    RpUnits::define(celcius, kelvin, centigrade2kelvin, kelvin2centigrade);
    RpUnits::define(fahrenheit, kelvin, fahrenheit2kelvin, kelvin2fahrenheit);
    RpUnits::define(rankine, kelvin, rankine2kelvin, kelvin2rankine);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetLength()
/// Add Length related units to the dictionary
/**
 * Defines the following units:
 *   meters         (m)
 *   angstrom       (A)
 *   inch           (in)
 *   feet           (ft)
 *   yard           (yd)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetLength () {

    RpUnits* meters     = RpUnits::define("m", NULL, RP_TYPE_LENGTH);
    RpUnits* angstrom   = RpUnits::define("A", NULL, RP_TYPE_LENGTH);
    RpUnits* inch       = RpUnits::define("in", NULL, RP_TYPE_LENGTH);
    RpUnits* feet       = RpUnits::define("ft", NULL, RP_TYPE_LENGTH);
    RpUnits* yard       = RpUnits::define("yd", NULL, RP_TYPE_LENGTH);

    RpUnits::makeMetric(meters);

    // add length definitions
    RpUnits::define(angstrom, meters, angstrom2meter, meter2angstrom);
    RpUnits::define(inch, meters, inch2meter, meter2inch);
    RpUnits::define(feet, meters, feet2meter, meter2feet);
    RpUnits::define(yard, meters, yard2meter, meter2yard);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetEnergy()
/// Add Energy related units to the dictionary
/**
 * Defines the following units:
 *   volt          (V)
 *   electron Volt (eV)
 *   joule         (J)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetEnergy () {

    RpUnits* volt       = RpUnits::define("V", NULL, RP_TYPE_ENERGY);
    RpUnits* eVolt      = RpUnits::define("eV", NULL, RP_TYPE_ENERGY);
    RpUnits* joule      = RpUnits::define("J", NULL, RP_TYPE_ENERGY);

    RpUnits::makeMetric(volt);
    RpUnits::makeMetric(eVolt);
    RpUnits::makeMetric(joule);

    // add energy definitions
    RpUnits::define(eVolt,joule,electronVolt2joule,joule2electronVolt);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetVolume()
/// Add Volume related units to the dictionary
/**
 * Defines the following units:
 *   cubic feet (ft3)
 *   us gallons (gal)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetVolume () {

    RpUnits* cubic_meter  = RpUnits::define("m3", NULL, RP_TYPE_VOLUME);
    // RpUnits* pcubic_meter  = RpUnits::define("/m3", NULL, RP_TYPE_VOLUME);
    RpUnits* cubic_feet   = RpUnits::define("ft3", NULL, RP_TYPE_VOLUME);
    RpUnits* us_gallon    = RpUnits::define("gal", NULL, RP_TYPE_VOLUME);

    RpUnits::makeMetric(cubic_meter);

    // add energy definitions
    RpUnits::define(cubic_meter,cubic_feet,meter2feet,feet2meter);
    RpUnits::define(cubic_meter,us_gallon,cubicMeter2usGallon,usGallon2cubicMeter);
    RpUnits::define(cubic_feet,us_gallon,cubicFeet2usGallon,usGallon2cubicFeet);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetAngle()
/// Add Angle related units to the dictionary
/**
 * Defines the following units:
 *   degrees  (deg)
 *   gradians (grad)
 *   radians  (rad) (and metric extensions)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetAngle () {

    RpUnits* degree  = RpUnits::define("deg",  NULL, RP_TYPE_ANGLE);
    RpUnits* gradian = RpUnits::define("grad", NULL, RP_TYPE_ANGLE);
    RpUnits* radian  = RpUnits::define("rad",  NULL, RP_TYPE_ANGLE);

    RpUnits::makeMetric(radian);

    // add angle definitions
    RpUnits::define(degree,gradian,deg2grad,grad2deg);
    RpUnits::define(radian,degree,rad2deg,deg2rad);
    RpUnits::define(radian,gradian,rad2grad,grad2rad);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetMass()
/// Add Mass related units to the dictionary
/**
 * Defines the following units:
 *   gram  (g)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetMass () {

    RpUnits* gram  = RpUnits::define("g",  NULL, RP_TYPE_MASS);

    RpUnits::makeMetric(gram);

    // add mass definitions
    // RpUnits::define(radian,gradian,rad2grad,grad2rad);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetMisc()
/// Add Misc related units to the dictionary
/**
 * Defines the following units:
 *   mole  (mol)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetMisc () {

    RpUnits* mole  = RpUnits::define("mol",  NULL, RP_TYPE_MISC);
    RpUnits* hertz = RpUnits::define("Hz",  NULL, RP_TYPE_MISC);
    RpUnits* becquerel = RpUnits::define("Bq",  NULL, RP_TYPE_MISC);

    RpUnits::makeMetric(mole);
    RpUnits::makeMetric(hertz);
    RpUnits::makeMetric(becquerel);

    // add misc definitions
    // RpUnits::define(radian,gradian,rad2grad,grad2rad);

    return 0;
}

// -------------------------------------------------------------------- //

