/*
#ifndef _RpDICT_H
    #include "../include/RpDict.h"
#endif
*/

/**************************************************************************/


// public RpDict member functions


/**************************************************************************
 *
 * int RpDict::size()
 *  retrieve the size of the structure
 *
 * Results:
 *  Returns size of the hash table
 *
 * Side Effects:
 *  none.
 * 
 *
 *************************************************************************/

template <typename KeyType, typename ValType> 
const int 
RpDict<KeyType,ValType>::size() const
{
    return numEntries;
}


/**************************************************************************
 *
 * RpDict::set()
 *  checks to make sure the table exists. 
 *  places a key/value pair into the hash table
 *
 * Results:
 *  Returns a reference to the RpDict object allowing the user to chain 
 *  together different commands such as 
 *      rpdict_obj.set(key).find(a).erase(a);
 *
 * Side Effects:
 *  if successful, the hash table will have a new entry
 * 
 *
 *************************************************************************/
template <typename KeyType, typename ValType> 
RpDict<KeyType,ValType>& 
RpDict<KeyType,ValType>::set(   KeyType& key, 
                                ValType& value, 
                                int* newPtr)
{
    RpDictEntry<KeyType,ValType> *hPtr;
    unsigned int hash;
    int index;

    assert(&key);
    assert(&value);
    
    // take care of the case where we are creating a NULL key entry.
    if (&key) {
        hash = (unsigned int) hashFxn(&key);
    }
    else {
        hash = 0;
    }

    index = randomIndex(hash);

    /*
     * Search all of the entries in the appropriate bucket.
     */
    for (hPtr = buckets[index]; hPtr != NULL; hPtr = hPtr->nextPtr) {
        if (hash != (unsigned int) hPtr->hash) {
            continue;
        }
        // if element already exists in hash, it should not be re-entered 
        if (key == *(hPtr->getKey())){

            // adjust the new flag if it was provided
            if (newPtr) {
                *newPtr = 0;
            }

            // adjust the value if it was provided
            // memory management is left as an exercize for the caller
            if (&value) {
                hPtr->setValue(value);
            }
            
            // return a reference to the dictionary object
            return *this;
        }
    }

    /*
     * Entry not found.  Add a new one to the bucket.
     */

    if (newPtr) {
        *newPtr = 1;
    }

    hPtr = new RpDictEntry<KeyType,ValType>(key,value);
    // hPtr->setValue(value);
    //
    // this is just a pointer that was allocated on the heap
    // it wont stick with the RpDictEntry after the fxn exits...
    // need to fix still.
    hPtr->tablePtr = this;
    hPtr->hash = hash;
    hPtr->nextPtr = buckets[index];
    buckets[index] = hPtr;
    numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many
     * more buckets.
     */

    if (numEntries >= rebuildSize) {
        RebuildTable();
    }

    // return a reference to the original object
    return *this;
}


/*  
 *----------------------------------------------------------------------
 *  
 *  RpDict::find(const char *key)
 *  
 *  Given a hash table find the entry with a matching key.
 *  
 * Results:
 *  The return value is a token for the matching entry in the
 *  hash table, or NULL if there was no matching entry.
 *  
 * Side effects:
 *  None.
 *          
 *----------------------------------------------------------------------
 */     
        
template <typename KeyType, typename ValType>
RpDictEntry<KeyType,ValType>& 
RpDict<KeyType,ValType>::find(KeyType& key)
{       
    RpDictEntry<KeyType,ValType> *hPtr;
    unsigned int hash;
    int index;

    assert(&key);

    // take care of the case where we are creating a NULL key entry.
    if (&key) {
        hash = (unsigned int) hashFxn(&key);
    }
    else {
        hash = 0;
    }

    index = randomIndex(hash);

    /*
     * Search all of the entries in the appropriate bucket.
     */

    for (hPtr = buckets[index]; hPtr != NULL; hPtr = hPtr->nextPtr) {
        if (hash != (unsigned int) hPtr->hash) {
            continue;
        }
        if (key == *(hPtr->getKey())) {
            // return a reference to the found object
            return *hPtr;
        }
    }

    // return a reference to the null object
    // find is not supposed to return a const, but i dont want the user
    // changing this entry's data members... what to do?
    return *nullEntry;

}



/**************************************************************************
 *
 * virtual RpDict& RpDictIterator::getTable()
 *  send the search iterator to the beginning of the hash table
 *
 * Results:
 *  returns pointer to the first hash entry of the hash table.
 *
 * Side Effects:
 *  moves iterator to the beginning of the hash table.
 * 
 *
 *************************************************************************/
/*
template <typename KeyType,typename ValType>
RpDict<KeyType,ValType>& 
RpDictIterator<KeyType,ValType>::getTable() 
{
    return tablePtr;
}
*/

/**************************************************************************
 *
 * virtual RpDictEntry& RpDict::first()
 *  send the search iterator to the beginning of the hash table
 *
 * Results:
 *  returns pointer to the first hash entry of the hash table.
 *
 * Side Effects:
 *  moves iterator to the beginning of the hash table.
 * 
 *
 *************************************************************************/
template <typename KeyType,typename ValType>
RpDictEntry<KeyType,ValType>* 
RpDictIterator<KeyType,ValType>::first()
{
    srchNextIndex = 0;
    srchNextEntryPtr = NULL;
    return next();
}

/**************************************************************************
 *
 * Tcl_HashEntry * RpDict::next()
 *  send the search iterator to the next entry of the hash table
 *
 * Results:
 *  returns pointer to the next hash entry of the hash table.
 *  if iterator is at the end of the hash table, NULL is returned
 *  and the iterator is left at the end of the hash table.
 *
 * Side Effects:
 *  moves iterator to the next entry of the hash table if it exists.
 * 
 *
 *************************************************************************/

template <typename KeyType,typename ValType>
RpDictEntry<KeyType,ValType>* 
RpDictIterator<KeyType,ValType>::next() 
{
    RpDictEntry<KeyType,ValType>* hPtr;

    while (srchNextEntryPtr == NULL) {
        if (srchNextIndex >= tablePtr.numBuckets) {
            return NULL;
        }
        srchNextEntryPtr = tablePtr.buckets[srchNextIndex];
        srchNextIndex++;
    }
    hPtr = srchNextEntryPtr;
    srchNextEntryPtr = hPtr->nextPtr;

    return hPtr;
}

/**************************************************************************
 *
 * RpDict & clear()
 *  iterate through the table and call erase on each element
 *
 * Results:
 *  empty hash table
 *
 * Side Effects:
 *  every element of the hash table will be erased.
 * 
 *
 *************************************************************************/
template <typename KeyType, typename ValType>
RpDict<KeyType,ValType>& 
RpDict<KeyType,ValType>::clear() 
{
    RpDictEntry<KeyType,ValType> *hPtr;
    RpDictIterator<KeyType,ValType> iter((RpDict&)*this);

    hPtr = iter.first();

    while (hPtr) {
        hPtr->erase();
        hPtr = iter.next();
    }

    return *this;
}

/**************************************************************************
 *
 * RpDict & getNullEntry()
 *  get the nullEntry hash entry for initialization of references
 *  
 *
 * Results:
 *  nullEntry RpDictEntry related to this dictionary is returned
 *
 * Side Effects:
 *  none
 * 
 *
 *************************************************************************/
template <typename KeyType, typename ValType>
RpDictEntry<KeyType,ValType>& 
RpDict<KeyType,ValType>::getNullEntry() 
{
    return *nullEntry;
}


/*
 *----------------------------------------------------------------------
 *
 * void RpDictEntry::erase()
 *
 *  Remove a single entry from a hash table.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  The entry given by entryPtr is deleted from its table and
 *  should never again be used by the caller.  It is up to the
 *  caller to free the clientData field of the entry, if that
 *  is relevant.
 *
 *----------------------------------------------------------------------
 */

template <typename KeyType, typename ValType>
void  
RpDictEntry<KeyType,ValType>::erase()
{   
    RpDictEntry<KeyType,ValType> *prevPtr;
    RpDictEntry<KeyType,ValType> **bucketPtr;
    int index = 0;

    // check to see if the object is associated with a table
    // if object is not associated with a table, there is no 
    // need to try to remove it from the table.
    if (tablePtr) {

        index = tablePtr->randomIndex(hash);
    
        // calculate which bucket the entry should be in.
        bucketPtr = &(tablePtr->buckets[index]);

        // remove the entry from the buckets
        // 
        // if entry is the first entry in the bucket
        // move the bucket to point to the next entry
        if ((*bucketPtr)->key == this->key) {
            *bucketPtr = nextPtr;
        }
        else {
            // if the entry is not the first entry in the bucket
            // search for the entry 
            for (prevPtr = *bucketPtr; ; prevPtr = prevPtr->nextPtr) {

                // printf("malformed bucket chain in RpDictEntry::erase()");
                assert(prevPtr != NULL);

                if (prevPtr->nextPtr == this) {
                    prevPtr->nextPtr = nextPtr;
                    break;
                } 
            } // end for loop
        } // end else

        // update our table's information
        tablePtr->numEntries--;

    } // end if tablePtr

    // invalidate the object
    nextPtr = NULL;
    tablePtr = NULL;
    hash = 0;
    // clientData = NULL;
    // key = NULL;
    
    // delete the object.
    // printf("delete %x\n", (unsigned) this);
    delete this;
    
}


/*
 *----------------------------------------------------------------------
 *
 * const char* RpDictEntry::getKey() const
 *
 *  retrieve the key of the current object
 *
 * Results:
 *  the key is returned to the caller
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

template <typename KeyType, typename ValType>
const KeyType*
RpDictEntry<KeyType,ValType>::getKey() const
{   
    return (const KeyType*) &key;
}

/*
 *----------------------------------------------------------------------
 *
 * const char* RpDictEntry::getValue() const
 *
 *  retrieve the value of the current object
 *
 * Results:
 *  the value is returned to the caller
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

template <typename KeyType, typename ValType>
const ValType* 
RpDictEntry<KeyType,ValType>::getValue() const
{   
    return (const ValType*) &clientData;
}

/*
 *----------------------------------------------------------------------
 *
 * const void* RpDictEntry::setValue()
 *
 *  retrieve the value of the current object
 *
 * Results:
 *  the value is returned to the caller
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

template <typename KeyType, typename ValType>
const ValType*
RpDictEntry<KeyType,ValType>::setValue(const ValType& value)
{   
    clientData = value;
    return (const ValType*) &clientData;
}

template <typename KeyType, typename ValType>
RpDictEntry<KeyType,ValType>::operator int() const
{

    if (!tablePtr && hash == 0)
        return 0;
    else
        return 1;

//    return (key);
}


/*************************************************************************/
/*************************************************************************/

// private member functions

/*
 *----------------------------------------------------------------------
 *
 * RebuildTable --
 *
 *  This procedure is invoked when the ratio of entries to hash
 *  buckets becomes too large.  It creates a new table with a
 *  larger bucket array and moves all of the entries into the
 *  new table.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Memory gets reallocated and entries get re-hashed to new
 *  buckets.
 *
 *----------------------------------------------------------------------
 */

template <typename KeyType, typename ValType>
void 
RpDict<KeyType,ValType>::RebuildTable()
{
    int oldSize=0, count=0, index=0; 
    RpDictEntry<KeyType,ValType> **oldBuckets;
    RpDictEntry<KeyType,ValType> **oldChainPtr, **newChainPtr;
    RpDictEntry<KeyType,ValType> *hPtr;
    
    void *key;

    oldSize = numBuckets;
    oldBuckets = buckets;

    /*
     * Allocate and initialize the new bucket array, and set up
     * hashing constants for new array size.
     */


    numBuckets *= 4;

    buckets = (RpDictEntry<KeyType,ValType> **) malloc((unsigned)
        (numBuckets * sizeof(RpDictEntry<KeyType,ValType> *)));

    for (count = numBuckets, newChainPtr = buckets;
        count > 0; 
        count--, newChainPtr++) {

        *newChainPtr = NULL;
    }

    rebuildSize *= 4;
    downShift -= 2;
    mask = (mask << 2) + 3;

    /*
     * Rehash all of the existing entries into the new bucket array.
     */

    for (oldChainPtr = oldBuckets; oldSize > 0; oldSize--, oldChainPtr++) {
        for (hPtr = *oldChainPtr; hPtr != NULL; hPtr = *oldChainPtr) {
            *oldChainPtr = hPtr->nextPtr;

            key = (void *) hPtr->getKey();

            index = randomIndex(hPtr->hash);

            hPtr->nextPtr = buckets[index];
            buckets[index] = hPtr;
        }
    }

    /*
     * Free up the old bucket array, if it was dynamically allocated.
     */

    if (oldBuckets != staticBuckets) {
        free((char *) oldBuckets);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * hashFxn --
 *
 *  Compute a one-word summary of a text string, which can be
 *  used to generate a hash index.
 *
 * Results:
 *  The return value is a one-word summary of the information in
 *  string.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

template <typename KeyType, typename ValType>
unsigned int 
RpDict<KeyType,ValType>::hashFxn(const void *keyPtr) const
{
    const char *stopAddr = (const char *) keyPtr + sizeof(&keyPtr) - 1 ;
    const char *str = (const char *) keyPtr;
    unsigned int result;
    int c;

    result = 0;

    while (str != stopAddr) {
        c = *str;
        result += (result<<3) + c;
        str++;
    }
    
    return result;
}

/*
 * I tried a zillion different hash functions and asked many other
 * people for advice.  Many people had their own favorite functions,
 * all different, but no-one had much idea why they were good ones.
 * I chose the one below (multiply by 9 and add new character)
 * because of the following reasons:
 * 
 * 1. Multiplying by 10 is perfect for keys that are decimal strings,
 *    and multiplying by 9 is just about as good.
 * 2. Times-9 is (shift-left-3) plus (old).  This means that each
 *    character's bits hang around in the low-order bits of the
 *    hash value for ever, plus they spread fairly rapidly up to
 *    the high-order bits to fill out the hash value.  This seems
 *    works well both for decimal and non-decimal strings.
 */ 


template <typename KeyType, typename ValType>
unsigned int 
RpDict<KeyType,ValType>::hashFxn(std::string* keyPtr) const
{
    const char *str = (const char *) (keyPtr->c_str());
    unsigned int result;
    int c;

    result = 0;

    while (1) {
        c = *str;
        if (c == 0) {
            break;
        }
        result += (result<<3) + c;
        str++;
    }
    
    return result;
}

template <typename KeyType, typename ValType>
unsigned int 
RpDict<KeyType,ValType>::hashFxn(char* keyPtr) const
{
    const char *str = (const char *) (keyPtr);
    unsigned int result;
    int c;

    result = 0;

    while (1) {
        c = *str;
        if (c == 0) {
            break;
        }
        result += (result<<3) + c;
        str++;
    }
    
    return result;
}

/*
 * ---------------------------------------------------------------------
 *
 * int RpDict::randomIndex(hash) 
 *
 * The following macro takes a preliminary integer hash value and
 * produces an index into a hash tables bucket list.  The idea is
 * to make it so that preliminary values that are arbitrarily similar
 * will end up in different buckets.  The hash function was taken
 * from a random-number generator.
 *
 * ---------------------------------------------------------------------
 */

template <typename KeyType, typename ValType>
int 
RpDict<KeyType,ValType>::randomIndex(unsigned int hash) 
{
    return (((((long) (hash))*1103515245) >> downShift) & mask);
}


