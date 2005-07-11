 #ifndef _RpUNITS_H
     #include "../include/RpUnits.h"
 #endif

// dict pointer
// RpDict<std::string,RpUnits*> RpUnits::dict = RpDict<std::string,RpUnits*>();
RpDict<std::string,RpUnits*>* RpUnits::dict = new RpDict<std::string,RpUnits*>();


/************************************************************************
 *                                                                      
 * add RpUnits Object                                                   
 *                                                                      
 ************************************************************************/

RpUnits * RpUnits::define(const std::string units, RpUnits * basis)
{ 
    RpUnits * newRpUnit = NULL;

    if (units == "") {
        // raise error, user sent null units!
        return NULL;
    }

    // check to see if the user is trying to trick me!
    if ( (basis) && (units == basis->getUnits()) ) {
        // dont trick me!
        return NULL;
    }

    // char * cmpPtr = NULL;
    double exp = 0.0;
    double oldExponent = 0;
    double newExponent = 0;
    int digiSearch = 0; // flag to show we are searching digits
    int alphaSearch = 0; // flag to show we are searching chars
    std::string dbText = "";

    std::string cmpStr = "";
    int cmpIndex = 0;

    std::string::size_type length = units.length();
    int srchIndex = length;
    std::string srchStr = units;

    // unit* p = NULL;

    while ((srchStr.length() > 0)) {

        srchIndex--;
 
        if (srchIndex < 0) {
            break;
        }
 
        if     ( isdigit(srchStr[srchIndex]) && !digiSearch && !alphaSearch) {
            digiSearch = 1;
        }
        else if(!isdigit(srchStr[srchIndex]) &&  digiSearch && !alphaSearch) {
 
            // convert our exponent to integer
 
            // check to see if there is a + or - sign
            if (  ( srchStr[srchIndex] == '+' )
               || ( srchStr[srchIndex] == '-' ) ) {
 
                // evaluate a '+' or '-' sign with the value
                srchIndex--;
            }
 
            srchIndex++;
 
            exp = atoi(&srchStr[srchIndex]);
 
            // we are no longer in a digit search
            digiSearch = 0;
 
            // place null character where the number starts
            // so we know what we've already parsed
 
            srchStr.erase(srchIndex);
            length = srchStr.length();
 
            /*
            // shortcut so we dont have to go through an additional while 
            // loop iteration.
            if (isalpha(srchStr[srchIndex-1])) {
                dbText = "alphaSearch = 1";
                dbprint(1,dbText);
                alphaSearch = 1;
                srchIndex--;
            }
            */
        }
        else if( isalpha(srchStr[srchIndex]) && !digiSearch && !alphaSearch) {
            alphaSearch = 1;
        }
        else if(!isalpha(srchStr[srchIndex]) && !digiSearch && alphaSearch) {
            
            // adjust the exponent if none was provided 
            if (exp == 0) {
                exp = 1;
            }
 
            // compare unit string to see if it is a recognized system
 
 
            std::string cmpStr = srchStr.substr(srchIndex+1,length-srchIndex-1);
            cmpIndex = 0;
 
            if ( (unsigned)(cmpIndex = pre_compare(cmpStr,basis)) == 
                    std::string::npos ) {
                alphaSearch = 0;
 
                // there are units we did not recognize,
                // right now we ignore them,
                // we may want to deal with them differntly in the future
 
                // srchStr.erase(srchIndex+1);
                // erase only the last character and reprocess the string
                // because our precompare doesnt take care of this yet.
                srchStr.erase(srchStr.length()-1);
                length = srchStr.length();
                srchIndex = length;
                
 
                // continue parsing characters
                continue;
            }
 
            // the compare function was successful
            // move the search pointer to one value infront of 
            // where the units were found.
            //
            // cmpIndex tells us how far ahead of the srchIndex the matching
            // unit was found. so we add srchIndex to get the real index.
            cmpIndex += srchIndex+1;
            srchIndex = cmpIndex;
            std::string newUnitText = srchStr.substr(cmpIndex,length-cmpIndex);
 
            // call the function to create the unit object
 
            // we need pre-compare to return the basis of what it found.
            // addUnit( newUnit, exp, basis)
            if (newRpUnit) {
                 // newRpUnit->addUnit( newUnitText, exp, NULL);
                 newRpUnit->addUnit( newUnitText, exp, basis);
            }
            else {
                 // newRpUnit= new RpUnits(newUnitText, exp, NULL);
                 newRpUnit= new RpUnits(newUnitText, exp, basis);
            }
 
            // sprintf(dbText,"creating unit: %s,%d\n\0", newUnit.c_str(),exp);
 
            // place a null character at the end of the string
            // so we know what we've parsed so far.
         
            srchStr.erase(srchIndex);
            length = srchStr.length();
 
            // fix our searching flag
            alphaSearch = 0;

            // shortcut so we dont have to go through an additional while 
            // loop iteration.
            // this doesnt work out so well, maybe need to reconsider
            // if (isdigit(*(search--))) {
            //    dbprint(1,"digiSearch = 1");
            //    digiSearch = 1;
            // }
        }
        // else if ( *search == '/' ) {
        else if( srchStr[srchIndex] == '/' ) {
            // have to go back to all of the objects created and 
            // multiply their exponents by -1.
 
            if (newRpUnit) {
                unit* p = newRpUnit->head;
                while (p) {
                    oldExponent = p->getExponent();
                    newExponent = oldExponent*-1;
                    p->newExponent(newExponent);
                    p = p->next;
                }
            }

            // place a null character at the end of the string
            // so we know what we've parsed so far.
        
            /*
            end = (search);
            *end = '\0';
            */
 
            // srchStr.erase(srchIndex+1);
            srchStr.erase(srchIndex);
            length = srchStr.length();

        }
        else {
            continue;
        }


    } // end while loop
        

    // complete the last iteration
    if (srchIndex < 0) {


        if (digiSearch) {
            // convert whatever is left
            exp = atoi(&srchStr[srchIndex+1]);
           
            // if we get here, that means units name starts with a digit
            // normally we wont get here, but if we do, should we place
            // the unit into the dictionary? i think not since digits are
            // always considered exponents.
        }
        else if (alphaSearch) {
            // adjust the exponent if none was provided 
            if (exp == 0) {
                exp = 1;
            }

            // compare unit string to see if it is a recognized system

            std::string cmpStr = srchStr.substr(srchIndex+1,length-srchIndex-1);
            cmpIndex = 0;

            if ( (cmpIndex = pre_compare(cmpStr,basis)) < 0 ) {
                // no matches in the compare function

                // there are units we did not recognize,
                // right now we ignore them,
                // we may want to deal with them differntly in the future
                // ignoring unit

                // break out of creating the last unit.

                // create a new unit with basis of null to show its a 
                // fundamental type
                // newRpUnit = new RpUnits(cmpStr, exp, NULL);
                newRpUnit = new RpUnits(cmpStr, exp, basis);
                
                // put the unit into the dictionary
                // newRpUnit->insert(cmpStr);
                //
                newRpUnit->insert(newRpUnit->getUnitsName());
                
                // RpUnits::dict.set(cmpStr,newRpUnit);

            }
            else {

                // the compare function was successful
                // move the search pointer to one value infront of 
                // where the units were found.
                // adjusting the search pointer to point to the units
                std::string newUnitText = srchStr.substr(cmpIndex,length-cmpIndex); 

                // call the function to create the unit object

                // we need pre-compare to return the basis of what it found.
                // addUnit( newUnit, exp, basis)
                if (newRpUnit) {
                     // newRpUnit->addUnit( newUnitText, exp, NULL );
                     newRpUnit->addUnit( newUnitText, exp, basis );
                }
                else {
                     // newRpUnit = new RpUnits(newUnitText, exp, NULL);
                     newRpUnit = new RpUnits(newUnitText, exp, basis);
                }

                // creating unit
                //
                // putting unit into dictionary
                newRpUnit->insert(newRpUnit->getUnitsName());
            }

        }
    }

    // place the new object into the dictionary
    
    // return a copy of the new object to user
    return newRpUnit;
}


/************************************************************************
 *                                                                      
 * add relation rule                                                    
 *                                                                      
 ************************************************************************/
RpUnits * RpUnits::define(  RpUnits* from,
                            RpUnits* to,
                            double (*convForwFxnPtr)(double),
                            double (*convBackFxnPtr)(double))
{ 
    RpUnits* conv = new RpUnits(from,to,convForwFxnPtr,convBackFxnPtr,NULL,NULL);
    
    return conv; 
}

RpUnits * RpUnits::define(  RpUnits* from,
                            RpUnits* to,
                            void* (*convForwFxnPtr)(void*, void*),
                            void* convForwData,
                            void* (*convBackFxnPtr)(void*, void*),
                            void* convBackData)
{ 
    RpUnits* conv = new RpUnits(    from,
                                    to,
                                    convForwFxnPtr,
                                    convForwData,
                                    convBackFxnPtr,
                                    convBackData,
                                    NULL,
                                    NULL);
    
    return conv; 
}

/************************************************************************
 *                                                                      
 * report the units this object represents back to the user             
 *                                                                      
 * **********************************************************************/
std::string RpUnits::getUnits() 
{
    // return units; 

    std::stringstream unitText;
    unit* p = head;
    
    while (p) {
        // unitText << p->getUnits() << p->getExponent();
        unitText << p->getUnits() ;
        p = p->next;
    }
    
    return (unitText.str()); 
}

/************************************************************************
 *                                                                      
 * report the units this object represents back to the user             
 *                                                                      
 * **********************************************************************/
std::string RpUnits::getUnitsName() 
{
    // return units; 

    std::stringstream unitText;
    unit* p = head;
    double exponent;
    
    while (p) {

        exponent = p->getExponent();

        if (exponent == 1) {
            unitText << p->getUnits();
        }
        else {
            unitText << p->getUnits() << p->getExponent();
        }

        p = p->next;
    }
    
    return (unitText.str()); 
}

/************************************************************************
 *                                                                      
 * report the exponent of the units of this object back to the user
 *                                                                      
 * **********************************************************************/
double RpUnits::getExponent() 
{ 
    // return exponent; 

    return head->getExponent();

    return 0; 
}

/************************************************************************
 *                                                                      
 *  report the basis of this object to the user                         
 *                                                                      
 * **********************************************************************/
RpUnits * RpUnits::getBasis() 
{ 
    // return basis; 

    // check if head exists? 

    return head->getBasis();
}

/************************************************************************
 *                                                                      
 *  convert the current unit to its basis units
 *  
 *  Return Codes
 *      0) no error (could also mean or no prefix was found)
 *          in some cases, this means the value is in its basis format
 *      1) the prefix found does not have a built in factor associated.
 *                                                                      
 ************************************************************************/
double RpUnits::makeBasis(double value, int* result)
{ 

    RpUnits* basis = getBasis();
    double retVal = value;

    if (result) {
        *result = 0;
    }

    if (basis == NULL) {
        // this unit is a basis
        // do nothing
        
        if (result) {
            *result = 1;
        }
    }
    else {
        retVal = convert(basis,value,result);
    }

    return retVal;
}

RpUnits& RpUnits::makeBasis(double* value, int* result)
{
    RpUnits* basis = getBasis();
    double retVal = *value;
    int convResult = 0;

    if (basis == NULL) {
        // this unit is a basis
        // do nothing
        
        if (result) {
            *result = 1;
        }
    }
    else {
        retVal = convert(basis,retVal,&convResult);
    }

    if ( (convResult == 1) ) {
        *value = retVal;
    }

    if (result) {
        *result = convResult;
    }

    

    return *this;
}

/************************************************************************
 *                                                                      
 *  static int makeMetric(RpUnits * basis);                      
 *  create the metric attachments for the given basis.
 *  should only be used if this unit is of metric type                  
 *                                                                      
 * **********************************************************************/
//static int RpUnits::makeMetric(RpUnits * basis) {
int RpUnits::makeMetric(RpUnits * basis) {

    if (!basis) {
        return 0;
    }

    std::string basisName = basis->getUnits();
    std::string name;
    std::string forw, back;
    
    name = "c" + basisName;
    RpUnits * centi = RpUnits::define(name, basis);
    RpUnits::define(centi, basis, centi2base, base2centi);

    name = "m" + basisName;
    RpUnits * milli = RpUnits::define(name, basis);
    RpUnits::define(milli, basis, milli2base, base2milli);

    name = "u" + basisName;
    RpUnits * micro = RpUnits::define(name, basis);
    RpUnits::define(micro, basis, micro2base, base2micro);

    name = "n" + basisName;
    RpUnits * nano  = RpUnits::define(name, basis);
    RpUnits::define(nano, basis, nano2base, base2nano);
    
    name = "p" + basisName;
    RpUnits * pico  = RpUnits::define(name, basis);
    RpUnits::define(pico, basis, pico2base, base2pico);

    name = "f" + basisName;
    RpUnits * femto = RpUnits::define(name, basis);
    RpUnits::define(femto, basis, femto2base, base2femto);

    name = "a" + basisName;
    RpUnits * atto  = RpUnits::define(name, basis);
    RpUnits::define(atto, basis, atto2base, base2atto);

    name = "k" + basisName;
    RpUnits * kilo  = RpUnits::define(name, basis);
    RpUnits::define(kilo, basis, kilo2base, base2kilo);

    name = "M" + basisName;
    RpUnits * mega  = RpUnits::define(name, basis);
    RpUnits::define(mega, basis, mega2base, base2mega);

    name = "G" + basisName;
    RpUnits * giga  = RpUnits::define(name, basis);
    RpUnits::define(giga, basis, giga2base, base2giga);

    name = "T" + basisName;
    RpUnits * tera  = RpUnits::define(name, basis);
    RpUnits::define(tera, basis, tera2base, base2tera);

    name = "P" + basisName;
    RpUnits * peta  = RpUnits::define(name, basis);
    RpUnits::define(peta, basis, peta2base, base2peta);
    
    // 
    return (1);
}


std::string RpUnits::convert (   RpUnits* toUnits, 
                        double val, 
                        int showUnits, 
                        int* result )
{
    double retVal = convert(toUnits,val,result); 
    std::stringstream unitText;
    

    if (showUnits) {
        unitText << retVal << toUnits->getUnits();
    }
    else {
        unitText << retVal;
    }

    return (unitText.str()); 

}

// user function to convert a value to the provided RpUnits* toUnits
// if it exists as a conversion from the basis
// example
//      cm.convert(meter,10) 
//      cm.convert(angstrum,100)
double RpUnits::convert(RpUnits* toUnit, double val, int* result)
{

    // currently we convert this object to its basis and look for the
    // connection ot the toUnit object from the basis.

    double value = val;
    RpUnits* basis = this->getBasis();
    RpUnits* fromUnit = this;
    RpUnits* dictToUnit = NULL;
    convEntry *p;
    int my_result = 0;

    // set *result to a default value
    if (result) {
        *result = 0;
    }

    // guard against converting to the units you are converting from...
    // ie. meters->meters
    if (this->getUnits() == toUnit->getUnits()) {
        if (result) {
            *result = 1;
        }
        return val;
    }
    
    // convert unit to the basis
    // makeBasis(&value);
    // trying to avoid the recursive way of converting to the basis.
    // need to rethink this.
    // 
    if ( (basis) && (basis->getUnits() != toUnit->getUnits()) ) {
        value = convert(basis,value,&my_result);
        if (my_result) {
            fromUnit = basis;
        }    
    }

    // find the toUnit in our dictionary.

    dictToUnit = find(toUnit->getUnits());

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

            value = p->conv->convForwFxnPtr(value);
            if (result) {
                *result = 1;
            }
            break;
        }
        
        if ( (p->conv->toPtr == fromUnit) && (p->conv->fromPtr == dictToUnit) ) {
            // we found our conversion
            // call the function pointer with value

            value = p->conv->convBackFxnPtr(value);
            if (result) {
                *result = 1;
            }
            break;
        }

        p = p->next;

    } while (p != NULL);


    if ( p == NULL) {
        // we did not find the conversion
        return val;
    }

    // we found the conversion.
    // return the converted value.
    return value;

}


void* RpUnits::convert(RpUnits* toUnit, void* val, int* result)
{

    // currently we convert this object to its basis and look for the
    // connection ot the toUnit object from the basis.

    void* value = val;
    RpUnits* basis = this->getBasis();
    RpUnits* fromUnit = this;
    RpUnits* dictToUnit = NULL;
    convEntry *p;
    int my_result = 0;

    // set *result to a default value
    if (result) {
        *result = 0;
    }

    // guard against converting to the units you are converting from...
    // ie. meters->meters
    if (this->getUnits() == toUnit->getUnits()) {
        if (result) {
            *result = 1;
        }
        return val;
    }
    
    // convert unit to the basis
    // makeBasis(&value);
    // trying to avoid the recursive way of converting to the basis.
    // need to rethink this.
    // 
    if ( (basis) && (basis->getUnits() != toUnit->getUnits()) ) {
        value = convert(basis,value,&my_result);
        if (my_result) {
            fromUnit = basis;
        }    
    }

    // find the toUnit in our dictionary.

    dictToUnit = find(toUnit->getUnits());

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

            if (result) {
                *result = 1;
            }
            break;
        }
        
        if ( (p->conv->toPtr == fromUnit) && (p->conv->fromPtr == dictToUnit) ) {
            // we found our conversion
            // call the function pointer with value

            value = p->conv->convBackFxnPtrVoid(p->conv->convBackData,value);
            if (result) {
                *result = 1;
            }
            break;
        }

        p = p->next;

    } while (p != NULL);


    if ( p == NULL) {
        // we did not find the conversion
        return val;
    }

    // we found the conversion.
    // return the converted value.
    return value;

}

void RpUnits::addUnit( const std::string& units,
                       double&  exponent,
                       RpUnits* basis
                      )
{
    unit* p = NULL;

    // check if the list was created previously. if not, start the list
    if (head == 0) {
        head = new unit(units,exponent,basis,NULL);
        return;
    }

    // now add a new node at the beginning of the list:
    p = new unit(units,exponent,basis,head);
    head->prev = p;
    head = p;

}

int RpUnits::insert(std::string key)
{
    int newRecord = 0;
    RpUnits* val = this;
    // dict pointer
    // RpUnits::dict.set(key,val,&newRecord);
    RpUnits::dict->set(key,val,&newRecord);
    // std::cout << "key = :" << key << ":" << std::endl;
    return newRecord;
}


int RpUnits::pre_compare( std::string& units, RpUnits* basis )
{

    // compare the incomming units with the previously defined units. 
    // compareStr will hold a copy of the incomming string. 
    // first look for the units as they are listed in the incomming variable
    // next look move searchStr toward the end of the string,
    // each time the pointer is moved, searchStr should be compared to all of
    // the previously defined units.
    // if searchStr is at the end of the string, then searchStr will be moved
    // back to the beginning of the string.
    // next it will traverse the string again, changing the case of the char
    // it points to and the resultant string will be compared again to all 
    // of the previously defined units.

    int compareSuccess = 0;
    // std::string::size_type units_len = units.length();
    // char * retVal = NULL;
    int retIndex = std::string::npos;
    // std::string compareStr = units;
    std::string searchStr = units;
    std::string dbText = "";
    
    // pass 1: look for exact match of units as they came into the function
    //          move searchStr pointer through string to find match.
    while ( ! compareSuccess && 
            (searchStr.length() > 0) ) {

        // std::cout << "units = :" << units << ":" << std::endl;
        // std::cout << "searchStr = :" << searchStr << ":" << std::endl;
        
//         if (!compare(searchStr)) {
        // dict pointer
        // if (dict.find(searchStr) == dict.getNullEntry()) {
        if (dict->find(searchStr) == dict->getNullEntry()) {
            // the string was not found, 
            // keep incrementing the pointer
            // searchStr (pass1)  does not match";
        }
        else {
            // compare was successful, 
            // searchStr (pass1)  found a match
            compareSuccess++;
            break;
            // is it possible to create the unit here and 
            // keep looking for units fro mthe provided string?
        }

        searchStr.erase(0,1);

        if (basis) {
            if ( (searchStr == basis->getUnits()) &&
                 (searchStr.length() == basis->getUnits().length()) )
            {
                break;
            }
        }
    }


    if (compareSuccess == 0) {
        // refresh our searchStr var.
        searchStr = units;
    }

    // pass 2: capitolize the first letter of the search string and compare
    //          for each letter in the string
    while ( ! compareSuccess &&
            (searchStr.length() > 0) ) {

        if (islower((int)(searchStr[0]))) {
            searchStr[0] = (char) toupper((int)(searchStr[0]));
        }

//        if (!compare(searchStr)) {
        // dict pointer
        // if (dict.find(searchStr) == dict.getNullEntry()) {
        if (dict->find(searchStr) == dict->getNullEntry()) {
            // the string was not found, 
            // keep incrementing the pointer
            // searchStr (pass2)  does not match
        }
        else {
            // compare was successful, 
            // searchStr (pass2)  found a match
            compareSuccess++;
            break;
        }
        searchStr.erase(0,1);

        // check to see if we are at the basis.
        if (basis) {
            if ( (searchStr == basis->getUnits()) &&
                 (searchStr.length() == basis->getUnits().length()) )
            {
                break;
            }
            
        }

    }



    // if we found a match, find the first occurance of the string which
    // was used to get the match, in our original units string.
    // this gets dicey for capitolizations.
    if ( compareSuccess ) {
        // need to think about if we want to search from the start
        // or the end of the string (find or rfind)
        // i think its the start because in this fxn (above)
        // we start at the beginning of the string and work
        // our way to the end. so if there is a second match at the
        // end, we would have only seen the first match.
        retIndex = units.find(searchStr);
    }

    return retIndex;

    /*
    if (retIndex) {
        // if we found a match return a 
        // pointer to where the matching units 
        // string can be found.
        dbText = "pre_compare returning an index";
        dbprint(1, dbText);
        return retIndex;
    }
    else {
        // if there was no match, return a negative value.
        dbText = "pre_compare returning -1";
        dbprint(1, dbText);
        return -1;
    }
    */
}


void RpUnits::connectConversion(RpUnits* myRpUnit) 
{

    /*
    int index = 0;

    while   ( (myRpUnit->staticConvArr[index] != NULL) 
         &&   (index < myRpUnit->numBuckets) ){

        index++;
    }

    if      ( (myRpUnit->staticConvArr[index] == NULL)
        &&    (index < myRpUnit->numBuckets) ) {
            
        myRpUnit->staticConvArr[index] = this->conv;
    }
    */

    convEntry* p = myRpUnit->convList;

    if (p == NULL) {
        myRpUnit->convList = new convEntry (this->conv,NULL,NULL);
    }
    else {
        while (p->next != NULL) {
            p = p->next;
        }

        p->next = new convEntry (this->conv,p,NULL);
    }

}


// -------------------------------------------------------------------- //

