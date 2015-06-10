/*
 * ----------------------------------------------------------------------
 *  RpUnits.cc
 *
 *   Data Members and member functions for the RpUnits class
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpUnits.h"
#include <cstdio>
#include <algorithm>

// dict pointer
// set the dictionary to be case insensitive for seaches and storage
RpDict<std::string,RpUnits*,RpUnits::_key_compare>* RpUnits::dict =
    new RpDict<std::string,RpUnits*,RpUnits::_key_compare>(
        RPUNITS_CASE_INSENSITIVE);

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
                    const std::string type,
                    bool metric,
                    bool caseInsensitive    ) {

    RpUnits* newRpUnit = NULL;

    std::string searchStr = units;
    std::string sendStr = "";
    int len = searchStr.length();
    int idx = len-1;
    double exponent = 1;

    RpUnitsTypes::RpUnitsTypesHint hint = NULL;

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
    hint = RpUnitsTypes::getTypeHint(type);
    if (RpUnits::find(units,hint)) {
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

    newRpUnit = new RpUnits(    sendStr,
                                exponent,
                                basis,
                                type,
                                metric,
                                caseInsensitive );
    if (newRpUnit) {
        insert(newRpUnit->getUnitsName(),newRpUnit);
    }

    // return a copy of the new object to user
    return newRpUnit;
}

/**********************************************************************/
// METHOD: incarnate()
/// Link two RpUnits where the entity is an incarnate of the abstraction
/**
 */

int
RpUnits::incarnate(const RpUnits* abstraction, const RpUnits* entity) {

    int retVal = 1;

    abstraction->connectIncarnation(entity);
    entity->connectIncarnation(abstraction);

    retVal = 0;
    return retVal;
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
 * this function will be the cause of problems related to adding symbols like %
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

int
RpUnits::grabUnits (    std::string inStr,
                        int* offset,
                        const RpUnits** unit,
                        const RpUnits** prefix   ) {

    int len = inStr.length();
    std::string preStr = "";

    if ( (unit == NULL) || (prefix == NULL) ) {
        // incorrect function call, return error
        return -1;
    }

    *unit = NULL;
    *prefix = NULL;

    while ( ! inStr.empty() ) {
        *unit = RpUnits::find(inStr,&RpUnitsTypes::hintTypeNonPrefix);
        if (*unit) {
            *offset = len - inStr.length();

            if ((*unit)->metric) {
                RpUnits::checkMetricPrefix(preStr,offset,prefix);
            }

            break;
        }
        preStr = preStr + inStr.substr(0,1);
        inStr.erase(0,1);
    }

    return 0;
}

/**********************************************************************/
// METHOD: checkMetrixPrefix()
/// Compare a string with available metric prefixes
/**
 * The metric prefix only has one or two letters before the main unit.
 * We take in the string of characters before the found unit and search
 * for the two characters closest to the found unit. If those two
 * characters are not found in the dictionary as a prefix, then we erase
 * the 0th character, and search for the 1th character. The 1th character
 * is the character closest to the found unit. If it is found as a prefix
 * in the dictionary, it is returned. If no prefix is found, NULL is
 * returned.
 */

int
RpUnits::checkMetricPrefix  (   std::string inStr,
                                int* offset,
                                const RpUnits** prefix   ) {

    int inStrLen = 0;
    std::string searchStr = "";

    inStrLen = inStr.length();

    if (inStrLen == 0) {
        // no prefix to search for, exit
        return 0;
    }

    if (prefix == NULL) {
        // incorrect function call, return error
        return -1;
    }


    if (inStrLen > 2) {
        searchStr = inStr.substr( inStrLen-2 );
    }
    else {
        searchStr = inStr;
    }

    *prefix = NULL;

    *prefix = RpUnits::find(searchStr,&RpUnitsTypes::hintTypePrefix);
    if ( (*prefix) == NULL ) {
        // the two letter prefix was not found,
        // try the one letter prefix
        searchStr.erase(0,1);
        *prefix = RpUnits::find(searchStr,&RpUnitsTypes::hintTypePrefix);
    }

    if (*prefix != NULL) {
        // if a prefix was found, adjust the offset to reflect
        // the need to erase the prefix as well as the unit name
        *offset = *offset - searchStr.length();
    }

    return 0;
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
// METHOD: getCI()
/// Return the case insensitivity of an RpUnits object.
/**
 */
bool
RpUnits::getCI() const {
    return this->ci;
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
    std::list<std::string> incarnationCompatList;
    std::stringstream myName;
    std::stringstream otherName;
    std::string otherBasisName  = "";
    std::string blank           = "";
    double otherExp             = 1.0;
    double myExp                = 1.0;
    double incExp               = 1.0;
    convEntry* myConversions    = this->convList;
    incarnationEntry* myIncarnations = this->incarnationList;
    const RpUnits * basis       = NULL;

    myName.str("");
    myName << getUnits();
    myExp = getExponent() * expMultiplier;

    if (this->basis) {
        basisCompatList = this->basis->getCompatible(expMultiplier);
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

            otherName.str("");
            // otherName << myConversions->conv->toPtr->getUnitsName();
            otherName << myConversions->conv->toPtr->getUnits();
            otherExp = myConversions->conv->toPtr->getExponent();
            basis = myConversions->conv->toPtr->basis;

            if (myName.str() == otherName.str()) {
                otherName.str("");
                // otherName << myConversions->conv->fromPtr->getUnitsName();
                otherName << myConversions->conv->fromPtr->getUnits();
                otherExp = myConversions->conv->fromPtr->getExponent();
                basis = myConversions->conv->fromPtr->basis;
            }

            // check to see if they are the same basis,
            // no need to list all of the metric conversions.
            if (basis) {
                if (basis->getUnitsName() == myName.str()) {
                    // do not add this unit to the conversion
                    // because its a derived unit.
                    myConversions = myConversions->next;
                    continue;
                }
            }

            // adjust the exponent as requested by fxn caller
            otherExp = otherExp * expMultiplier;

            // adjust the other units name to match exponent
            if ( (otherExp > 0) && (otherExp != 1) ) {
                otherName << otherExp;
            }
            else if (otherExp < 0) {
                otherName.str("/"+otherName.str());
                if (otherExp < -1) {
                    otherName.seekp(0,std::ios_base::end);
                    otherName << otherExp*-1;
                }
            }

            // add the other unit's name to the list of compatible units
            compatList.push_back(otherName.str());
            // compatList.push_back(otherName);

            // advance to the next conversion
            myConversions = myConversions->next;
        }

        // now go throught the incarnation list to see if there are other
        // compatible units listed there.
        while (myIncarnations != NULL) {
            incExp = myIncarnations->unit->getExponent();
            if (incExp == myExp) {
                incarnationCompatList = myIncarnations->unit->getCompatible();
                compatList.merge(incarnationCompatList);
                break;
            }
            else if ((-1.0*incExp) == myExp) {
                incarnationCompatList = myIncarnations->unit->getCompatible(-1);
                compatList.merge(incarnationCompatList);
                break;
            }
            else if (   (myExp == int(myExp))   &&
                        (incExp == int(incExp)) &&
                        ( (int(myExp)%int(incExp)) == 0) &&
                        ( (myExp/incExp) != 1)  &&
                        ( (myExp/incExp) != -1) &&
                        ( myExp != 1 )          &&
                        ( myExp != -1 )         &&
                        ( incExp != 1 )         &&
                        ( incExp != -1 )        ) {
                incarnationCompatList = myIncarnations->unit->getCompatible(myExp/incExp);
                compatList.merge(incarnationCompatList);
                break;
            }
            else {
                // do nothing
            }
            myIncarnations = myIncarnations->next;
        }
    }

    // adjust the exponent as requested by fxn caller
    // myExp = myExp * expMultiplier;

    // adjust the other units name to match exponent
    if ( (expMultiplier > 0) && (expMultiplier != 1) ) {
        // myName << expMultiplier;
        myName << myExp;
    }
    else if (expMultiplier < 0) {
        myName.str("/"+myName.str());
        if (myExp < -1) {
            myName.seekp(0,std::ios_base::end);
            myName << myExp*-1;
        }
    }

    compatList.push_back(myName.str());
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
RpUnits::getUnitsName(int flags) const {

    std::stringstream unitText;
    double exponent;

    exponent = getExponent();

    if ( (RPUNITS_ORIG_EXP & flags) == RPUNITS_POS_EXP)  {
        if (exponent < 0) {
            exponent = exponent * -1;
        }
    }
    else if ( (RPUNITS_ORIG_EXP & flags) == RPUNITS_NEG_EXP)  {
        if (exponent > 0) {
            exponent = exponent * -1;
        }
    }

    if (exponent == 1) {
        unitText << units;
    }
    else {
        unitText << units << exponent;
    }

    return (std::string(unitText.str()));
}

/**********************************************************************/
// METHOD: getSearchName()
/// Report the units name used when searching through the units dictionary.
/**
 * Reports the search name used to store and retrieve this object from
 * the units dictionary.
 */

std::string
RpUnits::getSearchName() const {

    std::string searchName = getUnitsName();

    std::transform( searchName.begin(),
                    searchName.end(),
                    searchName.begin(),
                    tolower );

    return (searchName);
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
// METHOD: setMetric()
/// Set the metric flag of the object.
/**
 * Set the metric flag of the object
 */

RpUnits&
RpUnits::setMetric(bool newVal) {

    metric = newVal;
    return *this;
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

/*
int
RpUnits::makeMetric(RpUnits* basis) {

    if (!basis) {
        return 0;
    }

    basis->setMetric(true);

    return 0;
}
*/

/**********************************************************************/
// METHOD: find()
/// Find a simple RpUnits Object from the provided string.
/**
 */

const RpUnits*
RpUnits::find(std::string key,
        RpDict<std::string,RpUnits*,_key_compare>::RpDictHint hint ) {

    RpDictEntry<std::string,RpUnits*,_key_compare>*
        unitEntry = &(dict->getNullEntry());
    RpDictEntry<std::string,RpUnits*,_key_compare>*
        nullEntry = &(dict->getNullEntry());
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

    if (unitEntry == nullEntry) {
        // pass 1 - look for the unit name as it was stated by the user
        // dict->toggleCI();
        unitEntry = &(dict->find(key,hint,!RPUNITS_CASE_INSENSITIVE));
        // dict->toggleCI();
    }

    if (unitEntry == nullEntry) {
        // pass 2 - use case insensitivity to look for the unit
        unitEntry = &(dict->find(key,hint,RPUNITS_CASE_INSENSITIVE));
    }

    if ( (!unitEntry->isValid()) || (unitEntry == nullEntry) ) {
        // unitEntry = NULL;
        return NULL;
    }

    return *(unitEntry->getValue());
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
 * if &compatList == NULL, no compatible list of units will be generated.
 * this function does not do a good job of placing the available units
 * back into the original formula. i still need to work on this.
 */

int
RpUnits::validate ( std::string& inUnits,
                    std::string& type,
                    std::list<std::string>* compatList ) {

    std::string sendUnitStr = "";
    double exponent         = 1;
    int err                 = 0;
    const RpUnits* unit     = NULL;
    std::list<std::string> basisCompatList;
    std::list<std::string>::iterator compatListIter;
    std::stringstream unitWExp;
    RpUnitsList inUnitsList;
    RpUnitsListIter inIter;

    // err tells us if we encountered any unrecognized units
    err = RpUnits::units2list(inUnits,inUnitsList,type);
    RpUnits::list2units(inUnitsList,inUnits);
    inIter = inUnitsList.begin();

    while ( inIter != inUnitsList.end() ) {

        unit = inIter->getUnitsObj();
        exponent = inIter->getExponent();

        // merge the compatible units
        if (compatList) {

            basisCompatList = unit->getCompatible(exponent);
            compatList->merge(basisCompatList);
        }

        inIter++;
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
RpUnitsListEntry::name(int flags) const {
    std::stringstream name;
    double myExp = exponent;

    if ( (RPUNITS_ORIG_EXP & flags) == RPUNITS_POS_EXP)  {
        if (myExp < 0) {
            myExp = myExp * -1;
        }
    }
    else if ( (RPUNITS_ORIG_EXP & flags) == RPUNITS_NEG_EXP)  {
        if (myExp > 0) {
            myExp = myExp * -1;
        }
    }

    if (prefix != NULL) {
        name << prefix->getUnits();
    }

    name << unit->getUnits();

    if ((RPUNITS_ORIG_EXP & flags) == RPUNITS_STRICT_NAME) {
        // if the user asks for strict naming,
        // always place the exponent on the name
        name << myExp;
    }
    else if (myExp != 1.0) {
        // if the user does not ask for strict naming,
        // check to see if the exponent == 1.
        // If not, then add exponent to name
        name << myExp;
    }

    return std::string(name.str());
}

/**********************************************************************/
// METHOD: getBasis()
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
// METHOD: getPrefix()
/// Return the prefix of an RpUnitsListEntry.
/**
 */

const RpUnits*
RpUnitsListEntry::getPrefix() const {
    return prefix;
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
/// Split a string of units into a list of units with exponents and prefixes.
/**
 * Splits a string of units like cm2/kVns into a list of units like
 * cm2, kV-1, ns-1 where an exponent is provided for each list entry.
 * List entries are found by comparing units strings to the names
 * in the dictionary.
 */

int
RpUnits::units2list ( const std::string& inUnits,
                      RpUnitsList& outList,
                      std::string& type ) {

    std::string myInUnits   = inUnits;
    std::stringstream sendUnitStr;
    double exponent         = 1;
    int offset              = 0;
    int idx                 = 0;
    int last                = 0;
    int err                 = 0;
    const RpUnits* unit     = NULL;
    const RpUnits* prefix   = NULL;

    while ( !myInUnits.empty() ) {

        // check to see if we came across a '/' character
        last = myInUnits.length()-1;
        if (myInUnits[last] == '/') {
            type = myInUnits[last] + type;
            myInUnits.erase(last);
            // multiply previous exponents by -1
            if ( ! outList.empty() ) {
                RpUnits::negateListExponents(outList);
            }
            continue;
        }

        // check to see if we came across a '*' character
        if (myInUnits[last] == '*') {
            // type = myInUnits[last] + type;
            // ignore * because we assume everything is multiplied together
            myInUnits.erase(last);
            continue;
        }

        // ignore whitespace characters
        if (isspace(myInUnits[last])) {
            // ignore whitespace characters, they represent multiplication
            myInUnits.erase(last);
            continue;
        }

        // get the exponent
        offset = RpUnits::grabExponent(myInUnits,&exponent);
        myInUnits.erase(offset);
        idx = offset - 1;
        last = myInUnits.length()-1;
        if (last == -1) {
            // the string is empty, units were not correctly entered
            err = 1;
            break;
        }

        // grab the largest string we can find
        offset = RpUnits::grabUnitString(myInUnits);

        // if offset > length, then the grabUnitString went through the whole
        // string and did not find a good string we could use as units.
        // this generally means the string was filled with non alphabetical
        // symbols like *&^%$#@!)(~`{}[]:;"'?/><,.-_=+\ or |

        if (offset > last) {
            err = 1;
            // erase the last offending character
            myInUnits.erase(last);
            // reset our vars and try again
            idx = 0;
            offset = 0;
            exponent = 1;
            continue;
        }
        else {
            idx = offset;
        }

        // figure out if we have some defined units in that string
        sendUnitStr.str(myInUnits.substr(offset,std::string::npos));
        grabUnits(sendUnitStr.str(),&offset,&unit,&prefix);
        if (unit) {
            // a unit was found
            // add this unit to the list
            // erase the found unit's name from our search string
            outList.push_front(RpUnitsListEntry(unit,exponent,prefix));
            if (type.compare("") == 0) {
                type = unit->getType();
            }
            else if (type[0] == '/') {
                type = unit->getType() + type;
            }
            else {
                type = unit->getType() + "*" + type;
            }
            myInUnits.erase(idx+offset);
        }
        else {
            // we came across a unit we did not recognize
            // raise error and delete character for now
            err = 1;
            myInUnits.erase(idx);
        }

        /*
        // if the exponent != 1,-1 then do a second search
        // for the unit+exponent string that might be defined.
        // this is to cover the case were we have defined conversions
        // m3<->gal, m3<->L but m is defined
        if ( (exponent != 1) && (exponent != -1) ) {
            sendUnitStr.str("");
            sendUnitStr << unit->getUnits() << exponent;
            unit = grabUnits(sendUnitStr.str(),&offset);
            if (unit) {
                // a unit was found
                // add this unit to the list
                outList.push_front(RpUnitsListEntry(unit,1.0));
            }
            else {
                // we came across a unit we did not recognize
                // do nothing
            }
        }
        */

        // reset our vars
        idx = 0;
        offset = 0;
        exponent = 1;
    }

    return err;
}

/**********************************************************************/
// METHOD: list2units()
/// Join a list of units into a string with proper exponents.
/**
 * Joins a list of units like cm2, kV-1, ns-1, creating a string
 * like cm2/kVns.
 */

int
RpUnits::list2units ( RpUnitsList& inList,
                      std::string& outUnitsStr) {

    RpUnitsListIter inListIter;
    std::string inUnits     = "";
    double exp              = 0;
    int err                 = 0;
    std::string numerator   = "";
    std::string denominator = "";

    inListIter = inList.begin();

    while (inListIter != inList.end()) {
        exp = inListIter->getExponent();
        if (exp > 0) {
            numerator += inListIter->name();
        }
        else if (exp < 0) {
            denominator += inListIter->name(RPUNITS_POS_EXP);
        }
        else {
            // we shouldn't get units with exponents of zero
        }
        inListIter++;
    }

    outUnitsStr = numerator;
    if ( denominator.compare("") != 0 ) {
        outUnitsStr += "/" + denominator;
    }

    return err;
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
    const RpUnits* toPrefix = NULL;
    const RpUnits* fromUnits = NULL;
    const RpUnits* fromPrefix = NULL;

    std::string tmpNumVal = "";
    std::string fromUnitsName = "";
    std::string convVal = "";
    std::string type = "";     // junk var used because units2list requires it
    std::string retStr = "";
    double numVal = 0;
    double toExp = 0;
    double fromExp = 0;
    int convErr = 0;
    std::stringstream outVal;

    double copies = 0;

    std::list<std::string> compatList;
    std::string listStr;

    convertList cList;
    convertList totalConvList;


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

    convErr = unitSlice(val,fromUnitsName,numVal);

    if (convErr != 0) {
        // no conversion was done.
        // number in incorrect format probably.
        if (result) {
            *result = 1;
        }
        return val;
    }

    if (toUnitsName.empty())  {
        // there were no units in the input
        // string or no conversion needed
        // assume fromUnitsName = toUnitsName
        // return the correct value
        if (result) {
            *result = 0;
        }

        if (showUnits == RPUNITS_UNITS_ON) {
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

        if (showUnits == RPUNITS_UNITS_ON) {
            outVal << numVal << toUnitsName;
        }
        else {
            outVal << numVal;
        }

        return std::string(outVal.str());
    }

    convErr = RpUnits::units2list(toUnitsName,toUnitsList,type);
    if (convErr) {
        if (result) {
            *result = convErr;
        }
        retStr = "Unrecognized units: \"" + toUnitsName + "\". Please specify valid Rappture Units";
        return retStr;
    }

    convErr = RpUnits::units2list(fromUnitsName,fromUnitsList,type);
    if (convErr) {
        if (result) {
            *result = convErr;
        }
        type = "";
        RpUnits::validate(toUnitsName,type,&compatList);
        list2str(compatList,listStr);
        retStr = "Unrecognized units: \"" + fromUnitsName
                + "\".\nShould be units of type " + type + " (" + listStr + ")";
        return retStr;
    }

    fromIter = fromUnitsList.begin();
    toIter = toUnitsList.begin();

    while ( (toIter != toUnitsList.end()) && (fromIter != fromUnitsList.end()) && (!convErr) ) {
        fromUnits = fromIter->getUnitsObj();
        fromPrefix = fromIter->getPrefix();
        toUnits = toIter->getUnitsObj();
        toPrefix = toIter->getPrefix();

        cList.clear();

        if (fromPrefix != NULL) {
            cList.push_back(fromPrefix->convList->conv->convForwFxnPtr);
        }

        convErr = fromUnits->getConvertFxnList(toUnits, cList);

        if (toPrefix != NULL) {
            cList.push_back(toPrefix->convList->conv->convBackFxnPtr);
        }

        if (convErr == 0) {

            toExp = toIter->getExponent();
            fromExp = fromIter->getExponent();

            if (fromExp == toExp) {
                copies = fromExp;
                if (fromExp < 0) {
                    copies = copies * -1.00;
                    totalConvList.push_back(&invert);
                }
                while (copies > 0) {
                    combineLists(totalConvList,cList);
                    copies--;
                }
                if (fromExp < 0) {
                    totalConvList.push_back(&invert);
                }
            }
            else {
                // currently we cannot handle conversions of
                // units where the exponents are different
                convErr++;
            }

        }

        if (convErr == 0) {
            // successful conversion reported
            // remove the elements from the lists
            tempIter = toIter;
            toIter++;
            toUnitsList.erase(tempIter);

            tempIter = fromIter;
            fromUnitsList.erase(tempIter);
            fromIter = fromUnitsList.begin();
        }
        else {
            // no conversion available?
            fromIter++;
            if (fromIter == fromUnitsList.end()) {

                fromIter = fromUnitsList.begin();
                toIter++;

                if (toIter == toUnitsList.end())  {

                    toIter = toUnitsList.begin();

                    // raise error that there was an
                    // unrecognized conversion request

                    convErr++;
                    retStr = "Conversion unavailable: (";
                    while (fromIter != fromUnitsList.end()) {
                        /*
                        if (fromIter != fromUnitsList.begin()) {
                            retStr += " or ";
                        }
                        */
                        retStr += fromIter->name();
                        fromIter++;
                    }
                    retStr += ") -> (";

                    // tempIter = toIter;

                    while (toIter != toUnitsList.end()) {
                        retStr += toIter->name();
                        toIter++;
                    }
                    retStr += ")";

                    type = "";
                    RpUnits::validate(toUnitsName,type,&compatList);
                    list2str(compatList,listStr);
                    retStr += "\nPlease enter units of type "
                                + type + " (" + listStr + ")";


                    // exit and report the error

                    /*
                    toIter = tempIter;
                    toIter++;
                    toUnitsList.erase(tempIter);
                    */
                }
                else {
                    // keep searching for units to convert
                    // until we are out of units in the
                    // fromUnitsList and toUnitsList.

                    convErr = 0;
                }
            }
            else {
                // keep searching for units to convert
                // until we are out of units in the
                // fromUnitsList and toUnitsList.

                convErr = 0;
            }
        }
    }



    if (convErr == 0) {
        // if ( (fromIter != fromUnitsList.end()) || (toIter != toUnitsList.end()) ) {
        if ( fromUnitsList.size() || toUnitsList.size() ) {
            // raise error that there was an
            // unrecognized conversion request

            convErr++;
            retStr = "unmatched units in conversion: (";

            fromIter = fromUnitsList.begin();
            while (fromIter != fromUnitsList.end()) {
                retStr += fromIter->name();
                fromIter++;
            }

            if (fromUnitsList.size() && toUnitsList.size()) {
                retStr += ") -> (";
            }

            toIter = toUnitsList.begin();
            while (toIter != toUnitsList.end()) {
                retStr += toIter->name();
                toIter++;
            }
            retStr += ")";
            type = "";
            RpUnits::validate(toUnitsName,type,&compatList);
            list2str(compatList,listStr);
            retStr += "\nPlease enter units of type "
                        + type + " (" + listStr + ")";

        }
        else {
            // apply the conversion and check for errors
            convErr = applyConversion (&numVal, totalConvList);
            if (convErr == 0) {
                // outVal.flags(std::ios::fixed);
                // outVal.precision(10);
                if (showUnits == RPUNITS_UNITS_ON) {
                    outVal << numVal << toUnitsName;
                }
                else {
                    outVal << numVal;
                }
                retStr = outVal.str();
            }
            else {

            }
        }
    }

    if ( (result) && (*result == 0) ) {
        *result = convErr;
    }

    return retStr;

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


    if (showUnits == RPUNITS_UNITS_ON) {
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

    RpUnitsTypes::RpUnitsTypesHint hint = NULL;

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
        hint = RpUnitsTypes::getTypeHint(toBasis->getType());
        dictToUnit = find(toBasis->getUnitsName(), hint);
    }
    else {
        hint = RpUnitsTypes::getTypeHint(toUnit->getType());
        dictToUnit = find(toUnit->getUnitsName(), hint);
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

    RpUnitsTypes::RpUnitsTypesHint hint = NULL;

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
        hint = RpUnitsTypes::getTypeHint(toBasis->getType());
        dictToUnit = find(toBasis->getUnitsName(), hint);
    }
    else {
        hint = RpUnitsTypes::getTypeHint(toUnit->getType());
        dictToUnit = find(toUnit->getUnitsName(), hint);
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
// METHOD: getConvertFxnList()
/// Return list of fxn pointers for converting two simple RpUnits objects.
/**
 * Return the conversion list that will convert from this RpUnits
 * object to the provided toUnits object if the conversion is defined
 * example
 *      cm.getConvertFxnList(meter,cList)
 *      cm.getConvertFxnList(angstrum,cList)
 *
 * Returns a list of conversion objects, represented by cList,
 * on success that a value can be applied to. The return value
 * will be zero (0).
 * Returns non-zero value on failure.
 */

int
RpUnits::getConvertFxnList(const RpUnits* toUnit, convertList& cList) const {

    // currently we convert this object to its basis and look for the
    // connection to the toUnit object from the basis.

    const RpUnits* toBasis = toUnit->getBasis();
    const RpUnits* fromUnit = this;
    const RpUnits* dictToUnit = NULL;
    convEntry *p;
    int result = 0;

    // guard against converting to the units you are converting from...
    // ie. meters->meters
    if (this->getUnitsName() == toUnit->getUnitsName()) {
        return result;
    }

    // convert unit to the basis
    // makeBasis(&value);
    // trying to avoid the recursive way of converting to the basis.
    // need to rethink this.
    //
    if ( (basis) && (basis->getUnitsName() != toUnit->getUnitsName()) ) {
        result = fromUnit->getConvertFxnList(basis,cList);
        if (result == 0) {
            fromUnit = basis;
        }
        else {
            // exit because an error occured while
            // trying to convert to the basis
            return result;
        }
    }

    // find the toUnit in our dictionary.
    // if the toUnits has a basis, we need to search for the basis
    // and convert between basis' and then convert again back to the
    // original unit.
    if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
        dictToUnit = find(  toBasis->getUnitsName(),
                            &RpUnitsTypes::hintTypeNonPrefix );
    }
    else {
        dictToUnit = find(  toUnit->getUnitsName(),
                            &RpUnitsTypes::hintTypeNonPrefix );
    }

    // did we find the unit in the dictionary?
    if (dictToUnit == NULL) {
        // toUnit was not found in the dictionary
        result = 1;
        return result;
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
        result = 1;
        return result;
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

                // value = p->conv->convForwFxnPtr(value);
                cList.push_back(p->conv->convForwFxnPtr);
            }
            /*
            else if (  (p->conv->convForwFxnPtrDD)
                    && (! p->conv->convForwFxnPtr) ) {

                // value = p->conv->convForwFxnPtrDD(value, fromUnit->getExponent());
                cList.pushback(conv);
            }
            */

            // check to see if we converted to the actual requested unit
            // or to the requested unit's basis.
            // if we converted to the requested unit's basis. we need to
            // do one last conversion from the requested unit's basis back
            // to the requested unit.
            if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
                result += toBasis->getConvertFxnList(toUnit,cList);
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

                // value = p->conv->convBackFxnPtr(value);
                cList.push_back(p->conv->convBackFxnPtr);
            }
            /*
            else if (  (p->conv->convBackFxnPtrDD)
                    && (! p->conv->convBackFxnPtr) ) {

                // value = p->conv->convBackFxnPtrDD(value, fromUnit->getExponent());
                cList.pushback(conv);
            }
            */

            // check to see if we converted to the actual requested unit
            // or to the requested unit's basis.
            // if we converted to the requested unit's basis. we need to
            // do one last conversion from the requested unit's basis back
            // to the requested unit.
            if ( (toBasis) && (toBasis->getUnitsName() != fromUnit->getUnitsName()) ) {
                result += toBasis->getConvertFxnList(toUnit,cList);
            }

            break;
        }

        p = p->next;

    } while (p != NULL);


    if ( p == NULL) {
        // we did not find the conversion
        result += 1;
    }

    // return the converted value and result flag
    return result;
}

/**********************************************************************/
// METHOD: applyConversion()
/// Apply a list of conversions in cList to the value val
/**
 * Apply a list of conversions, represented by cList, to the value
 * val.
 *
 * Returns an integer value of zero (0) on success
 * Returns non-zero value on failure.
 */

int
RpUnits::applyConversion(double* val, convertList& cList) {

    convertList::iterator iter;

    if(val == NULL) {
        return 1;
    }

    for(iter = cList.begin(); iter != cList.end(); iter++)
    {
        *val = (*iter)(*val);
    }

    return 0;
}

/**********************************************************************/
// METHOD: combineLists()
/// combine two convertLists in an orderly fasion
/**
 *
 * elements of l2 are pushed onto l1 in the same order in which it
 * exists in l2. l1 is changed in this function.
 *
 * Returns an integer value of zero (0) on success
 * Returns non-zero value on failure.
 */

int
RpUnits::combineLists(convertList& l1, convertList& l2) {

    for (convertList::iterator iter = l2.begin(); iter != l2.end(); iter++) {
        l1.push_back(*iter);
    }
    return 0;

}

/**********************************************************************/
// METHOD: printList()
/// print a list
/**
 *
 * Returns an integer value of zero (0) on success
 * Returns non-zero value on failure.
 */

int
RpUnits::printList(convertList& l1) {

    for (convertList::iterator iter = l1.begin(); iter != l1.end(); iter++) {
        printf("%p\n", *iter);
    }
    return 0;

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
    RpUnitsTypes::RpUnitsTypesHint hint = NULL;

    if (val == NULL) {
        return -1;
    }

    hint = RpUnitsTypes::getTypeHint(val->getType());

    RpUnits::dict->set(key,val,hint,&newRecord,val->getCI());

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
// METHOD: connectIncarnation()
/// Attach incarnation object information to a RpUnits Object.
/**
 */

void
RpUnits::connectIncarnation(const RpUnits* unit) const {

    incarnationEntry* p = incarnationList;

    if (p == NULL) {
        incarnationList = new incarnationEntry (unit,NULL,NULL);
    }
    else {
        while (p->next != NULL) {
            p = p->next;
        }

        p->next = new incarnationEntry (unit,p,NULL);
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
    else if (group.compare(RP_TYPE_ENERGY) == 0) {
        retVal = RpUnitsPreset::addPresetEnergy();
    }
    else if (group.compare(RP_TYPE_LENGTH) == 0) {
        retVal = RpUnitsPreset::addPresetLength();
    }
    else if (group.compare(RP_TYPE_TEMP) == 0) {
        retVal = RpUnitsPreset::addPresetTemp();
    }
    else if (group.compare(RP_TYPE_TIME) == 0) {
        retVal = RpUnitsPreset::addPresetTime();
    }
    else if (group.compare(RP_TYPE_VOLUME) == 0) {
        retVal = RpUnitsPreset::addPresetVolume();
    }
    else if (group.compare(RP_TYPE_ANGLE) == 0) {
        retVal = RpUnitsPreset::addPresetAngle();
    }
    else if (group.compare(RP_TYPE_MASS) == 0) {
        retVal = RpUnitsPreset::addPresetMass();
    }
    else if (group.compare(RP_TYPE_PREFIX) == 0) {
        retVal = RpUnitsPreset::addPresetPrefix();
    }
    else if (group.compare(RP_TYPE_PRESSURE) == 0) {
        retVal = RpUnitsPreset::addPresetPressure();
    }
    else if (group.compare(RP_TYPE_CONC) == 0) {
        retVal = RpUnitsPreset::addPresetConcentration();
    }
    else if (group.compare(RP_TYPE_FORCE) == 0) {
        retVal = RpUnitsPreset::addPresetForce();
    }
    else if (group.compare(RP_TYPE_MAGNETIC) == 0) {
        retVal = RpUnitsPreset::addPresetMagnetic();
    }
    else if (group.compare(RP_TYPE_MISC) == 0) {
        retVal = RpUnitsPreset::addPresetMisc();
    }
    else if (group.compare(RP_TYPE_POWER) == 0) {
        retVal = RpUnitsPreset::addPresetPower();
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

    result += addPresetPrefix();
    result += addPresetTime();
    result += addPresetTemp();
    result += addPresetLength();
    result += addPresetEnergy();
    result += addPresetVolume();
    result += addPresetAngle();
    result += addPresetMass();
    result += addPresetPressure();
    result += addPresetConcentration();
    result += addPresetForce();
    result += addPresetMagnetic();
    result += addPresetMisc();
    result += addPresetPower();

    return 0;
}


/**********************************************************************/
// METHOD: addPresetPrefix()
///
/**
 * Defines the following unit prefixes:
 *   deci        (d)
 *   centi       (c)
 *   milli       (m)
 *   micro       (u)
 *   nano        (n)
 *   pico        (p)
 *   femto       (f)
 *   atto        (a)
 *   deca        (da)
 *   hecto       (h)
 *   kilo        (k)
 *   mega        (M)
 *   giga        (G)
 *   tera        (T)
 *   peta        (P)
 *   exa         (E)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetPrefix () {

    std::string type = RP_TYPE_PREFIX;
    RpUnits* basis = NULL;

    RpUnits * deci  = NULL;
    RpUnits * centi = NULL;
    RpUnits * milli = NULL;
    RpUnits * micro = NULL;
    RpUnits * nano  = NULL;
    RpUnits * pico  = NULL;
    RpUnits * femto = NULL;
    RpUnits * atto  = NULL;
    RpUnits * deca  = NULL;
    RpUnits * hecto = NULL;
    RpUnits * kilo  = NULL;
    RpUnits * mega  = NULL;
    RpUnits * giga  = NULL;
    RpUnits * tera  = NULL;
    RpUnits * peta  = NULL;
    RpUnits * exa   = NULL;

    deci  = RpUnits::define ( "d",  basis, type);
    centi = RpUnits::define ( "c",  basis, type);
    milli = RpUnits::define ( "m",  basis, type, !RPUNITS_METRIC,
                              !RPUNITS_CASE_INSENSITIVE);
    micro = RpUnits::define ( "u",  basis, type);
    nano  = RpUnits::define ( "n",  basis, type, !RPUNITS_METRIC,
                              !RPUNITS_CASE_INSENSITIVE);
    pico  = RpUnits::define ( "p",  basis, type, !RPUNITS_METRIC,
                              !RPUNITS_CASE_INSENSITIVE);
    femto = RpUnits::define ( "f",  basis, type);
    atto  = RpUnits::define ( "a",  basis, type);
    deca  = RpUnits::define ( "da", basis, type);
    hecto = RpUnits::define ( "h",  basis, type);
    kilo  = RpUnits::define ( "k",  basis, type);
    mega  = RpUnits::define ( "M",  basis, type, !RPUNITS_METRIC,
                              !RPUNITS_CASE_INSENSITIVE);
    giga  = RpUnits::define ( "G",  basis, type);
    tera  = RpUnits::define ( "T",  basis, type);
    peta  = RpUnits::define ( "P",  basis, type, !RPUNITS_METRIC,
                              !RPUNITS_CASE_INSENSITIVE);
    exa  = RpUnits::define  ( "E",  basis, type);

    // the use of the unit as the from and the to unit is a hack
    // that can be resolved by creating a RpPrefix object
    // the define() function cannot handle NULL as to unit.
    RpUnits::define ( deci,  deci , deci2base,  base2deci);
    RpUnits::define ( centi, centi, centi2base, base2centi);
    RpUnits::define ( milli, milli, milli2base, base2milli);
    RpUnits::define ( micro, micro, micro2base, base2micro);
    RpUnits::define ( nano,  nano , nano2base,  base2nano);
    RpUnits::define ( pico,  pico , pico2base,  base2pico);
    RpUnits::define ( femto, femto, femto2base, base2femto);
    RpUnits::define ( atto,  atto , atto2base,  base2atto);
    RpUnits::define ( deca,  deca , deca2base,  base2deca);
    RpUnits::define ( hecto, hecto, hecto2base, base2hecto);
    RpUnits::define ( kilo,  kilo , kilo2base,  base2kilo);
    RpUnits::define ( mega,  mega , mega2base,  base2mega);
    RpUnits::define ( giga,  giga , giga2base,  base2giga);
    RpUnits::define ( tera,  tera , tera2base,  base2tera);
    RpUnits::define ( peta,  peta , peta2base,  base2peta);
    RpUnits::define ( exa,   exa  , exa2base,   base2exa);

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

    RpUnits* second    = NULL;
    RpUnits* minute    = NULL;
    RpUnits* hour      = NULL;
    RpUnits* day       = NULL;

    second    = RpUnits::define("s", NULL, RP_TYPE_TIME, RPUNITS_METRIC);
    minute    = RpUnits::define("min", second, RP_TYPE_TIME);
    hour      = RpUnits::define("h", second, RP_TYPE_TIME);
    day       = RpUnits::define("d", second, RP_TYPE_TIME);

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

    RpUnits* fahrenheit = NULL;
    RpUnits* celcius    = NULL;
    RpUnits* kelvin     = NULL;
    RpUnits* rankine    = NULL;

    fahrenheit = RpUnits::define("F", NULL, RP_TYPE_TEMP);
    celcius    = RpUnits::define("C", NULL, RP_TYPE_TEMP, RPUNITS_METRIC);
    kelvin     = RpUnits::define("K", NULL, RP_TYPE_TEMP,RPUNITS_METRIC);
    rankine    = RpUnits::define("R", NULL, RP_TYPE_TEMP);

    // add temperature definitions
    RpUnits::define(fahrenheit, celcius, fahrenheit2centigrade, centigrade2fahrenheit);
    RpUnits::define(celcius, kelvin, centigrade2kelvin, kelvin2centigrade);
    RpUnits::define(fahrenheit, kelvin, fahrenheit2kelvin, kelvin2fahrenheit);
    RpUnits::define(rankine, kelvin, rankine2kelvin, kelvin2rankine);
    RpUnits::define(fahrenheit, rankine, fahrenheit2rankine, rankine2fahrenheit);
    RpUnits::define(celcius, rankine, celcius2rankine, rankine2celcius);

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

    RpUnits* meters     = NULL;
    RpUnits* angstrom   = NULL;
    RpUnits* bohr       = NULL;
    RpUnits* inch       = NULL;
    RpUnits* feet       = NULL;
    RpUnits* yard       = NULL;
    RpUnits* mile       = NULL;

    meters     = RpUnits::define("m", NULL, RP_TYPE_LENGTH, RPUNITS_METRIC);
    angstrom   = RpUnits::define("A", NULL, RP_TYPE_LENGTH);
    bohr       = RpUnits::define("bohr", NULL, RP_TYPE_LENGTH);
    inch       = RpUnits::define("in", NULL, RP_TYPE_LENGTH);
    feet       = RpUnits::define("ft", inch, RP_TYPE_LENGTH);
    yard       = RpUnits::define("yd", inch, RP_TYPE_LENGTH);
    mile       = RpUnits::define("mi", inch, RP_TYPE_LENGTH);

    // RpUnits::makeMetric(meters);

    // add length definitions
    RpUnits::define(angstrom, meters, angstrom2meter, meter2angstrom);
    RpUnits::define(bohr, meters, bohr2meter, meter2bohr);
    RpUnits::define(inch, feet, inch2feet, feet2inch);
    RpUnits::define(inch, yard, inch2yard, yard2inch);
    RpUnits::define(inch, meters, inch2meter, meter2inch);
    RpUnits::define(inch, mile, inch2mile, mile2inch);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetEnergy()
/// Add Energy related units to the dictionary
/**
 * Defines the following units:
 *   electron Volt (eV)
 *   joule         (J)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetEnergy () {

    RpUnits* eVolt      = NULL;
    RpUnits* joule      = NULL;

    eVolt      = RpUnits::define("eV", NULL, RP_TYPE_ENERGY, RPUNITS_METRIC);
    joule      = RpUnits::define("J", NULL, RP_TYPE_ENERGY, RPUNITS_METRIC);

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
 *   liter      (L)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetVolume () {

    // RpUnits* cubic_meter  = RpUnits::define("m3", NULL, RP_TYPE_VOLUME);
    // RpUnits* cubic_feet   = RpUnits::define("ft3", NULL, RP_TYPE_VOLUME);
    // RpUnits* us_gallon    = NULL;
    // RpUnits* liter        = NULL;

    RpUnits::define("gal", NULL, RP_TYPE_VOLUME);
    RpUnits::define("L", NULL, RP_TYPE_VOLUME, RPUNITS_METRIC);

    /*
    // RpUnits::makeMetric(cubic_meter);
    const RpUnits* meter = NULL;
    const RpUnits* foot = NULL;

    meter = RpUnits::find("m");
    if (meter && cubic_meter) {
        RpUnits::incarnate(meter,cubic_meter);
    }
    else {
        // raise an error, could not find meter unit
    }

    foot = RpUnits::find("ft");
    if (foot && cubic_feet) {
        RpUnits::incarnate(foot,cubic_feet);
    }
    else {
        // raise an error, could not find meter unit
    }
    */

    // RpUnits::makeMetric(liter);


    // add volume definitions
    // RpUnits::define(cubic_meter,cubic_feet,meter2feet,feet2meter);
    // RpUnits::define(cubic_meter,us_gallon,cubicMeter2usGallon,usGallon2cubicMeter);
    // RpUnits::define(cubic_feet,us_gallon,cubicFeet2usGallon,usGallon2cubicFeet);
    // RpUnits::define(cubic_meter,liter,cubicMeter2liter,liter2cubicMeter);
    // RpUnits::define(liter,us_gallon,liter2us_gallon,us_gallon2liter);

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

    RpUnits* degree  = NULL;
    RpUnits* gradian = NULL;
    RpUnits* radian  = NULL;

    degree  = RpUnits::define("deg",  NULL, RP_TYPE_ANGLE);
    gradian = RpUnits::define("grad", NULL, RP_TYPE_ANGLE);
    radian  = RpUnits::define("rad",  NULL, RP_TYPE_ANGLE, RPUNITS_METRIC);

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

    RpUnits::define("g", NULL, RP_TYPE_MASS, RPUNITS_METRIC,!RPUNITS_CASE_INSENSITIVE);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetPressure()
/// Add pressure related units to the dictionary
/**
 * http://www.ilpi.com/msds/ref/pressureunits.html
 *
 * Defines the following units:
 *   atmosphere             (atm)
 *   bar                    (bar)
 *   pascal                 (Pa)
 *   pounds/(in^2)          (psi)
 *   torr                   (torr)
 *   millimeters Mercury    (mmHg)
 *
 * mmHg was added because as a convenience to those who have not
 * yet switched over to the new representation of torr.
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetPressure () {

    RpUnits* atmosphere = NULL;
    RpUnits* bar        = NULL;
    RpUnits* pascal     = NULL;
    RpUnits* psi        = NULL;
    RpUnits* torr       = NULL;
    RpUnits* mmHg       = NULL;

    atmosphere  = RpUnits::define("atm", NULL, RP_TYPE_PRESSURE);
    bar     = RpUnits::define("bar",  NULL, RP_TYPE_PRESSURE, RPUNITS_METRIC);
    pascal  = RpUnits::define("Pa",   NULL, RP_TYPE_PRESSURE, RPUNITS_METRIC);
    psi     = RpUnits::define("psi",  NULL, RP_TYPE_PRESSURE);
    torr    = RpUnits::define("torr", NULL, RP_TYPE_PRESSURE);
    mmHg    = RpUnits::define("mmHg", torr, RP_TYPE_PRESSURE);

    RpUnits::define(bar,pascal,bar2Pa,Pa2bar);
    RpUnits::define(bar,atmosphere,bar2atm,atm2bar);
    RpUnits::define(bar,psi,bar2psi,psi2bar);
    RpUnits::define(bar,torr,bar2torr,torr2bar);
    RpUnits::define(pascal,atmosphere,Pa2atm,atm2Pa);
    RpUnits::define(pascal,torr,Pa2torr,torr2Pa);
    RpUnits::define(pascal,psi,Pa2psi,psi2Pa);
    RpUnits::define(torr,atmosphere,torr2atm,atm2torr);
    RpUnits::define(torr,psi,torr2psi,psi2torr);
    RpUnits::define(atmosphere,psi,atm2psi,psi2atm);

    RpUnits::define(torr,mmHg,torr2mmHg,mmHg2torr);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetConcentration()
/// Add concentration related units to the dictionary
/**
 *
 * Defines the following units:
 *   pH    (pH)
 *   pOH    (pOH)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetConcentration () {

    RpUnits* pH  = NULL;
    RpUnits* pOH = NULL;

    pH  = RpUnits::define("pH",  NULL, RP_TYPE_CONC);
    pOH = RpUnits::define("pOH", NULL, RP_TYPE_CONC);

    // add concentration definitions
    RpUnits::define(pH,pOH,pH2pOH,pOH2pH);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetForce()
/// Add concentration related units to the dictionary
/**
 * http://en.wikipedia.org/wiki/Newton
 *
 * Defines the following units:
 *   newton    (N)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetForce () {

    RpUnits::define("N",  NULL, RP_TYPE_FORCE, RPUNITS_METRIC);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetMagnetic()
/// Add concentration related units to the dictionary
/**
 *
 * http://en.wikipedia.org/wiki/Tesla_(unit)
 * http://en.wikipedia.org/wiki/Gauss_(unit)
 * http://en.wikipedia.org/wiki/Maxwell_(unit)
 * http://en.wikipedia.org/wiki/Weber_(unit)
 *
 * Defines the following units:
 *   tesla    (T)
 *   gauss    (G)
 *   maxwell  (Mx)
 *   weber    (Wb)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetMagnetic () {

    RpUnits* tesla = NULL;
    RpUnits* gauss = NULL;
    RpUnits* maxwell = NULL;
    RpUnits* weber = NULL;

    tesla = RpUnits::define("T",  NULL, RP_TYPE_MAGNETIC, RPUNITS_METRIC);
    gauss = RpUnits::define("G",  NULL, RP_TYPE_MAGNETIC, !RPUNITS_CASE_INSENSITIVE);
    maxwell = RpUnits::define("Mx",  NULL, RP_TYPE_MAGNETIC);
    weber = RpUnits::define("Wb", NULL, RP_TYPE_MAGNETIC, RPUNITS_METRIC);

    RpUnits::define(tesla,gauss,tesla2gauss,gauss2tesla);
    RpUnits::define(maxwell,weber,maxwell2weber,weber2maxwell);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetMisc()
/// Add Misc related units to the dictionary
/**
 * Defines the following units:
 *   volt  (V)
 *   mole  (mol)
 *   hertz (Hz)
 *   becquerel (Bq)
 *   amu
 *   bel (B)
 *   amp
 *   ohm
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetMisc () {

    RpUnits::define("V",  NULL, RP_TYPE_EPOT, RPUNITS_METRIC);
    RpUnits::define("mol",NULL, "quantity", RPUNITS_METRIC);
    RpUnits::define("Hz", NULL, "frequency", RPUNITS_METRIC);
    RpUnits::define("Bq", NULL, "radioactivity", RPUNITS_METRIC);
    RpUnits::define("amu", NULL, "mass_unit", !RPUNITS_METRIC);
    RpUnits::define("B", NULL, "audio_transmission", RPUNITS_METRIC);
    RpUnits::define("amp", NULL, "electric_current", RPUNITS_METRIC);
    RpUnits::define("ohm", NULL, "electric_resistance", RPUNITS_METRIC);

    // RpUnits* percent   = RpUnits::define("%",  NULL, RP_TYPE_MISC);

    return 0;
}

/**********************************************************************/
// METHOD: addPresetPower()
/// Add power related units to the dictionary
/**
 * Defines the following units:
 *   watt  (W)
 *
 * Return codes: 0 success, anything else is error
 */

int
RpUnitsPreset::addPresetPower () {

    // watts are derived units = J/s = kg*m2/s3 = Newton*m/s and Amps*Volt
    RpUnits::define("W",  NULL, RP_TYPE_POWER, RPUNITS_METRIC);

    return 0;
}

RpUnitsTypes::RpUnitsTypesHint
RpUnitsTypes::getTypeHint (std::string type) {

    if (type.compare(RP_TYPE_ENERGY) == 0) {
        return &RpUnitsTypes::hintTypeEnergy;
    }
    else if (type.compare(RP_TYPE_EPOT) == 0) {
        return &RpUnitsTypes::hintTypeEPot;
    }
    else if (type.compare(RP_TYPE_LENGTH) == 0) {
        return &RpUnitsTypes::hintTypeLength;
    }
    else if (type.compare(RP_TYPE_TEMP) == 0) {
        return &RpUnitsTypes::hintTypeTemp;
    }
    else if (type.compare(RP_TYPE_TIME) == 0) {
        return &RpUnitsTypes::hintTypeTime;
    }
    else if (type.compare(RP_TYPE_VOLUME) == 0) {
        return &RpUnitsTypes::hintTypeVolume;
    }
    else if (type.compare(RP_TYPE_ANGLE) == 0) {
        return &RpUnitsTypes::hintTypeAngle;
    }
    else if (type.compare(RP_TYPE_MASS) == 0) {
        return &RpUnitsTypes::hintTypeMass;
    }
    else if (type.compare(RP_TYPE_PREFIX) == 0) {
        return &RpUnitsTypes::hintTypePrefix;
    }
    else if (type.compare(RP_TYPE_PRESSURE) == 0) {
        return &RpUnitsTypes::hintTypePressure;
    }
    else if (type.compare(RP_TYPE_CONC) == 0) {
        return &RpUnitsTypes::hintTypeConc;
    }
    else if (type.compare(RP_TYPE_FORCE) == 0) {
        return &RpUnitsTypes::hintTypeForce;
    }
    else if (type.compare(RP_TYPE_MAGNETIC) == 0) {
        return &RpUnitsTypes::hintTypeMagnetic;
    }
    else if (type.compare(RP_TYPE_MISC) == 0) {
        return &RpUnitsTypes::hintTypeMisc;
    }
    else if (type.compare(RP_TYPE_POWER) == 0) {
        return &RpUnitsTypes::hintTypePower;
    }
    else {
        return NULL;
    }
};

bool
RpUnitsTypes::hintTypePrefix   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_PREFIX) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeNonPrefix    (   RpUnits* unitObj    ) {

    bool retVal = true;

    if ( (unitObj->getType()).compare(RP_TYPE_PREFIX) == 0 ) {
        retVal = false;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeEnergy   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_ENERGY) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeEPot   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_EPOT) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeLength   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_LENGTH) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeTemp   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_TEMP) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeTime   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_TIME) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeVolume   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_VOLUME) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeAngle   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_ANGLE) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeMass   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_MASS) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypePressure   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_PRESSURE) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeConc   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_CONC) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeForce   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_FORCE) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeMagnetic   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_MAGNETIC) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypeMisc   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_MISC) == 0 ) {
        retVal = true;
    }

    return retVal;
}

bool
RpUnitsTypes::hintTypePower   (   RpUnits* unitObj    ) {

    bool retVal = false;

    if ( (unitObj->getType()).compare(RP_TYPE_POWER) == 0 ) {
        retVal = true;
    }

    return retVal;
}

// -------------------------------------------------------------------- //

/**********************************************************************/
// FUNCTION: list2str()
/// Convert a std::list<std::string> into a comma delimited std::string
/**
 * Iterates through a std::list<std::string> and returns a comma
 * delimited std::string containing the elements of the inputted std::list.
 *
 * Returns 0 on success, anything else is error
 */

int
list2str (std::list<std::string>& inList, std::string& outString)
{
    int retVal = 1;  // return Value 0 is success, everything else is failure
    unsigned int counter = 0; // check if we hit all elements of inList
    std::list<std::string>::iterator inListIter; // list interator

    inListIter = inList.begin();

    while (inListIter != inList.end()) {
        if ( outString.empty() ) {
            outString = *inListIter;
        }
        else {
            outString =  outString + "," + *inListIter;
        }

        // increment the iterator and loop counter
        inListIter++;
        counter++;
    }

    if (counter == inList.size()) {
        retVal = 0;
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: unitSlice()
/// Convert a std::string into what might be the value and units
/**
 * Returns 0 on success, anything else is error
 */

int
unitSlice (std::string inStr, std::string& outUnits, double& outVal)
{
    int retVal      = 0;  // return val 0 is success, otherwise failure
    char *endptr    = NULL;

    outVal = strtod(inStr.c_str(), &endptr);

    if ( (outVal == 0) && (endptr == inStr.c_str()) ) {
        // no conversion performed
        retVal = 1;
    }

    outUnits = std::string(endptr);

    return retVal;
}
