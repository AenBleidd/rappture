/*
 * ======================================================================
 *  RpDict
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <iostream>
#include <cassert>
#include <string>
#include <stdlib.h>
#include <errno.h>

#ifndef _RpDICT_H
#define _RpDICT_H

/**************************************************************************/
/**************************************************************************/

template <typename KeyType,
          typename ValType,
          class _Compare=std::equal_to<KeyType> >
    class RpDict;

template <typename KeyType,
          typename ValType,
          class _Compare=std::equal_to<KeyType> >
    class RpDictEntry;

template <typename KeyType,
          typename ValType,
          class _Compare=std::equal_to<KeyType> >
    class RpDictIterator;

/*
 * RpDictEntry should not depend on _Compare,
 * it was originally needed because RpDict is a friend class
 * then i needed to assign it a default value because RpUnits.cc
 * uses it in its find function. RpUnits::find should really not
 * have to use it.
 *
 * RpDictIterator has the same thing going on. It has a default value
 * because RpBindingsDict uses it.
 */

/**************************************************************************/

template <typename KeyType, typename ValType, class _Compare>
class RpDictIterator
{

    public:

        // retrieve the table the iterator is iterating
        // virtual RpDict<KeyType,ValType,_Compare>& getTable();

        // send the search iterator to the beginning of the hash table
        /*virtual*/ RpDictEntry<KeyType,ValType,_Compare>* first();

        // send the search iterator to the next element of the hash table
        /*virtual*/ RpDictEntry<KeyType,ValType,_Compare>* next();
/*
        RpDictIterator(RpDict* table_Ptr)
            : tablePtr( (RpDict&) *table_Ptr),
              srchNextEntryPtr(NULL),
              srchNextIndex(0)
        {
        }
*/
        RpDictIterator(RpDict<KeyType,ValType,_Compare>& table_Ptr)
            : tablePtr(table_Ptr),
              srchNextIndex(0),
              srchNextEntryPtr(NULL)
        {
        }

        // copy constructor
        RpDictIterator(RpDictIterator<KeyType,ValType,_Compare>& iterRef)
            : tablePtr(iterRef.tablePtr),
              srchNextIndex(iterRef.srchNextIndex),
              srchNextEntryPtr(iterRef.srchNextEntryPtr)
        {
        }

        // destructor

    private:

        RpDict<KeyType,ValType,_Compare>&
            tablePtr;                   /* pointer to the table we want to
                                         * iterate */
        int srchNextIndex;              /* Index of next bucket to be
                                         * enumerated after present one. */
        RpDictEntry<KeyType,ValType,_Compare>*
            srchNextEntryPtr;           /* Next entry to be enumerated in the
                                         * the current bucket. */

};

template <typename KeyType, typename ValType, class _Compare>
class RpDictEntry
{
    public:

        operator int() const;
        // operator==(const RpDictEntry& entry) const;
        //
        //operator!=(const RpDictEntry& lhs, const RpDictEntry& rhs) const
        //{
        //    if (lhs.key != rhs.key)
        //}
        const KeyType* getKey() const;
        const ValType* getValue() const;
        // const void* setValue(const void* value);
        const ValType* setValue(const ValType& value);
        bool isValid() const;

        // erases this entry from its table
        void erase();

        friend class RpDict<KeyType,ValType,_Compare>;
        friend class RpDictIterator<KeyType,ValType,_Compare>;

        // two-arg constructor
        RpDictEntry(KeyType newKey, ValType newVal)
           : nextPtr    (NULL),
             tablePtr   (NULL),
             hash       (0),
             clientData (newVal),
             key        (newKey),
             valid      (&clientData)
        {
        }

        // copy constructor
        RpDictEntry (const RpDictEntry<KeyType,ValType,_Compare>& entry)
        {
            nextPtr     = entry.nextPtr;
            tablePtr    = entry.tablePtr;
            hash        = entry.hash;

            if (entry.valid != NULL) {
                clientData  = (ValType) entry.getValue();
                key         = (KeyType) entry.getKey();
                valid       = &clientData;
            }
            else {
                valid = NULL;
            }

        }

    private:

        RpDictEntry<KeyType,ValType,_Compare>*
            nextPtr;                /* Pointer to next entry in this
                                     * hash bucket, or NULL for end of
                                     * chain. */

        RpDict<KeyType,ValType,_Compare>*
            tablePtr;              /* Pointer to table containing entry. */

        unsigned int hash;          /* Hash value. */

        ValType clientData;        /* Application stores something here
                                    * with Tcl_SetHashValue. */

        KeyType key;               /* entry key */

        ValType* valid;            /* is this a valid object */

        RpDictEntry()
           : nextPtr (NULL),
             tablePtr (NULL),
             hash (0),
             valid (NULL)
             // clientData (),
             // key ()
        {
        }

};

template <typename KeyType, typename ValType, class _Compare>
class RpDict
{
    public:

        typedef bool (*RpDictHint)(ValType);

        // functionality for the user to access/adjust data members

        // checks table size
        /*virtual*/ int size() const;

        // insert new object into table
        // returns 0 on success (object inserted or already exists)
        // returns !0 on failure (object cannot be inserted or dne)
        //
        /*virtual*/ RpDict<KeyType,ValType,_Compare>&
                        set(KeyType& key,
                            ValType& value,
                            RpDictHint hint=NULL,
                            int *newPtr=NULL,
                            bool ci=false);

        // find an RpUnits object that should exist in RpUnitsTable
        //
        /*virtual*/ RpDictEntry<KeyType,ValType,_Compare>&
                        find(KeyType& key,
                             RpDictHint hint = NULL,
                             bool ci=false);

        /*virtual*/ RpDictEntry<KeyType,ValType,_Compare>& operator[](KeyType& key)
        {
            return find(key,NULL);
        }

        RpDict<KeyType,ValType,_Compare>& setCI(bool val);
        bool getCI();
        RpDict<KeyType,ValType,_Compare>& toggleCI();

        // clear the entire table
        // iterate through the table and call erase on each element
        /*virtual*/ RpDict<KeyType,ValType,_Compare>& clear();

        // get the nullEntry hash entry for initialization of references
        /*virtual*/ RpDictEntry<KeyType,ValType,_Compare>& getNullEntry();

        // template <KeyType, ValType> friend class RpDictEntry;
        // template <KeyType, ValType> friend class RpDictIterator;

        friend class RpDictEntry<KeyType,ValType,_Compare>;
        friend class RpDictIterator<KeyType,ValType,_Compare>;

        // default constructor
        RpDict (bool ci=false)
            : SMALL_RP_DICT_SIZE(4),
              REBUILD_MULTIPLIER(3),
              buckets(staticBuckets),
              numBuckets(SMALL_RP_DICT_SIZE),
              numEntries(0),
              rebuildSize(SMALL_RP_DICT_SIZE*REBUILD_MULTIPLIER),
              downShift(28),
              mask(3),
              caseInsensitive(ci)
        {
            staticBuckets[0] = staticBuckets[1] = 0;
            staticBuckets[2] = staticBuckets[3] = 0;

            // setup a dummy entry of NULL
            nullEntry = new RpDictEntry<KeyType,ValType,_Compare>();

            assert(nullEntry != NULL);
        }

        // copy constructor
        // RpDict (const RpDict& dict);

        // assignment operator
        // RpDict& operator=(const RpDict& dict);

        // destructor
        /*virtual*/ ~RpDict()
        {
            // probably need to delete all the entries as well
            delete nullEntry;
        }
        // make sure to go through the hash table and free all RpDictEntries
        // because the space is malloc'd in RpDict::set()


    private:
        const int SMALL_RP_DICT_SIZE;
        const int REBUILD_MULTIPLIER;

        RpDictEntry<KeyType,ValType,_Compare>
                    **buckets;        /* Pointer to bucket array.  Each
                                       * element points to first entry in
                                       * bucket's hash chain, or NULL. */
        RpDictEntry<KeyType,ValType,_Compare>
                    *staticBuckets[4];
                                      /* Bucket array used for small tables
                                       * (to avoid mallocs and frees). */
        int numBuckets;               /* Total number of buckets allocated
                                       * at **bucketPtr. */
        int numEntries;               /* Total number of entries present
                                       * in table. */
        int rebuildSize;              /* Enlarge table when numEntries gets
                                       * to be this large. */
        int downShift;                /* Shift count used in hashing
                                       * function.  Designed to use high-
                                       * order bits of randomized keys. */
        int mask;                     /* Mask value used in hashing
                                       * function. */
        bool caseInsensitive;         /* When set to true, dictionary uses
                                       * case insensitive functions,
                                       * translating chars to uppercase */

        RpDictEntry<KeyType,ValType,_Compare>
                    *nullEntry;   /* if not const, compiler complains*/



        // private member fxns

        RpDictEntry<KeyType,ValType,_Compare>*
            search(KeyType& key, RpDictHint hint = NULL, bool ci = false);

        // static void RpDict::RebuildTable ();
        void RebuildTable ();

        unsigned int hashFxn(const void* keyPtr) const;
        unsigned int hashFxn(std::string* keyPtr) const;
        unsigned int hashFxn(char* keyPtr) const;

        int randomIndex(unsigned int hash);
};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

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
template <typename KeyType, typename ValType, class _Compare>
int
RpDict<KeyType,ValType,_Compare>::size() const
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
template <typename KeyType, typename ValType, class _Compare>
RpDict<KeyType,ValType,_Compare>&
RpDict<KeyType,ValType,_Compare>::set(KeyType& key,
                                      ValType& value,
                                      RpDictHint hint,
                                      int* newPtr,
                                      bool ci)
{
    RpDictEntry<KeyType,ValType,_Compare> *hPtr = NULL;
    unsigned int hash = 0;
    int index = 0;
    bool oldCI = caseInsensitive;

    assert(&key);
    assert(&value);

    hPtr = search(key,hint,ci);
    if (hPtr != NULL) {
        // adjust the new flag if it was provided
        if (newPtr) {
            *newPtr = 0;
        }

        // adjust the value if it was provided
        // memory management is left as an exercise for the caller
        if (&value) {
            hPtr->setValue(value);
        }

        // return a reference to the dictionary object
        return *this;
    }

    /*
     * Entry not found.  Add a new one to the bucket.
     */

    if (&key) {
        if (ci != oldCI) {
            setCI(ci);
        }
        hash = (unsigned int) hashFxn(&key);
        if (ci != oldCI) {
            setCI(oldCI);
        }
    }
    else {
        // we are creating a NULL key entry.
        hash = 0;
    }

    index = randomIndex(hash);

    hPtr = new RpDictEntry<KeyType,ValType,_Compare>(key,value);
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

    if (newPtr) {
        *newPtr = 1;
    }

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
 *  RpDict::find(KeyType& key, RpDictHint hint)
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
template <typename KeyType, typename ValType, class _Compare>
RpDictEntry<KeyType,ValType,_Compare>&
RpDict<KeyType,ValType,_Compare>::find(KeyType& key,
                                       RpDictHint hint,
                                       bool ci)
{
    RpDictEntry<KeyType,ValType,_Compare> *hPtr = NULL;

    hPtr = search(key,hint,ci);

    if (hPtr != NULL) {
        return *hPtr;
    }

    // return a reference to the null object
    // find is not supposed to return a const, but i dont want the user
    // changing this entry's data members... what to do?
    return *nullEntry;
}

/*
 *----------------------------------------------------------------------
 *
 *  RpDict::search(KeyType& key, RpDictHint hint)
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
template <typename KeyType, typename ValType, class _Compare>
RpDictEntry<KeyType,ValType,_Compare>*
RpDict<KeyType,ValType,_Compare>::search(KeyType& key,
                                         RpDictHint hint,
                                         bool ci)
                                         // bool ci,
                                         // RpDictEntryList* entryList)
{
    RpDictEntry<KeyType,ValType,_Compare> *hPtr = NULL;
    unsigned int hash = 0;
    int index = 0;
    bool oldCI = caseInsensitive;

    assert(&key);

    // take care of the case where we are creating a NULL key entry.
    if (&key) {
        if (ci != oldCI) {
            // toggle the case insensitivity of the dictionary
            setCI(ci);
        }

        hash = (unsigned int) hashFxn(&key);

        if (ci != oldCI) {
            // reset the case insensitivity of the dictionary
            setCI(oldCI);
        }
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
        if (_Compare()(key, *(hPtr->getKey()))) {
            // check to see if the user provided a hint
            if (hint != NULL ) {
                // if there is a hint, run the potential return value
                // throught the hint function.
                if (hint(*(hPtr->getValue())) == true) {
                    // the hint approves of our choice of return values
                    // return a reference to the found object
                    return hPtr;
                }
            }
            else {
                // return a reference to the found object
                return hPtr;
            }
        }
    }

    // return a reference to the null object
    // find is not supposed to return a const, but i dont want the user
    // changing this entry's data members... what to do?
    return hPtr;

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
template <typename KeyType,typename ValType,class _Compare>
RpDict<KeyType,ValType,_Compare>&
RpDictIterator<KeyType,ValType,_Compare>::getTable()
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
template <typename KeyType,typename ValType,class _Compare>
RpDictEntry<KeyType,ValType,_Compare>*
RpDictIterator<KeyType,ValType,_Compare>::first()
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
template <typename KeyType,typename ValType,class _Compare>
RpDictEntry<KeyType,ValType,_Compare>*
RpDictIterator<KeyType,ValType,_Compare>::next()
{
    RpDictEntry<KeyType,ValType,_Compare>* hPtr = NULL;

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
 * RpDict & setCI(bool val)
 *  Use case insensitive functions where applicable within the dictionary
 *
 * Results:
 *  sets the dictionary objects caseInsensitive variable to the boolean val
 *
 * Side Effects:
 *  mostly find and set functions will begin to execute their functions
 *  with case insensitivity in mind.
 *
 *
 *************************************************************************/
template <typename KeyType, typename ValType, class _Compare>
RpDict<KeyType,ValType,_Compare>&
RpDict<KeyType,ValType,_Compare>::setCI(bool val)
{
    caseInsensitive = val;
    return *this;
}

/**************************************************************************
 *
 * bool getCI()
 *  Retrieve the case insensitivity of this dictionary object
 *
 * Results:
 *  returns the dictionary object's caseInsensitive variable to the user
 *
 * Side Effects:
 *  None
 *
 *************************************************************************/
template <typename KeyType, typename ValType, class _Compare>
bool
RpDict<KeyType,ValType,_Compare>::getCI()
{
    return caseInsensitive;
}

/**************************************************************************
 *
 * RpDict & toggleCI()
 *  Toggle the case insensitivity of this dictionary object
 *
 * Results:
 *  returns the dictionary object's caseInsensitive variable to the user
 *
 * Side Effects:
 *  None
 *
 *************************************************************************/
template <typename KeyType, typename ValType, class _Compare>
RpDict<KeyType,ValType,_Compare>&
RpDict<KeyType,ValType,_Compare>::toggleCI()
{
    caseInsensitive = !caseInsensitive;
    return *this;
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
template <typename KeyType, typename ValType, class _Compare>
RpDict<KeyType,ValType,_Compare>&
RpDict<KeyType,ValType,_Compare>::clear()
{
    RpDictEntry<KeyType,ValType,_Compare> *hPtr = NULL;
    RpDictIterator<KeyType,ValType,_Compare> iter((RpDict&)*this);

    hPtr = iter.first();

    while (hPtr) {
        hPtr->erase();
        hPtr = iter.next();
    }

    return *this;
}

/**************************************************************************
 *
 * RpDictEntry & getNullEntry()
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
template <typename KeyType, typename ValType, class _Compare>
RpDictEntry<KeyType,ValType,_Compare>&
RpDict<KeyType,ValType,_Compare>::getNullEntry()
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
template <typename KeyType, typename ValType, class _Compare>
void
RpDictEntry<KeyType,ValType,_Compare>::erase()
{
    RpDictEntry<KeyType,ValType,_Compare> *prevPtr = NULL;
    RpDictEntry<KeyType,ValType,_Compare> **bucketPtr = NULL;
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
    valid = NULL;

    // delete the object.
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
template <typename KeyType, typename ValType, class _Compare>
const KeyType*
RpDictEntry<KeyType,ValType,_Compare>::getKey() const
{
    return (const KeyType*) &key;
}

/*
 *----------------------------------------------------------------------
 *
 * const char* RpDictEntry::getValue() const
 *
 *  retrieve the value of the current object
 *  it is the caller responsibility to check isValid() to see if the
 *  object actually holds a valid value.
 *
 * Results:
 *  the value is returned to the caller
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
template <typename KeyType, typename ValType, class _Compare>
const ValType*
RpDictEntry<KeyType,ValType,_Compare>::getValue() const
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
template <typename KeyType, typename ValType, class _Compare>
const ValType*
RpDictEntry<KeyType,ValType,_Compare>::setValue(const ValType& value)
{
    clientData = value;
    valid = &clientData;
    return (const ValType*) &clientData;
}

template <typename KeyType, typename ValType, class _Compare>
RpDictEntry<KeyType,ValType,_Compare>::operator int() const
{

    if (!tablePtr && hash == 0)
        return 0;
    else
        return 1;

//    return (key);
}

/*
 *----------------------------------------------------------------------
 *
 * bool RpDictEntry::isValid()
 *
 *  is this a valid object, return true or false
 *
 * Results:
 *  tells the user if the object is valid true or false
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
template <typename KeyType, typename ValType, class _Compare>
bool
RpDictEntry<KeyType,ValType,_Compare>::isValid() const
{
    if (valid) {
        return true;
    }
    return false;
}

/*************************************************************************/
/*************************************************************************/

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
template <typename KeyType, typename ValType, class _Compare>
void
RpDict<KeyType,ValType,_Compare>::RebuildTable()
{
    int oldSize=0, count=0, index=0;
    RpDictEntry<KeyType,ValType,_Compare> **oldBuckets = NULL;
    RpDictEntry<KeyType,ValType,_Compare> **oldChainPtr = NULL, **newChainPtr = NULL;
    RpDictEntry<KeyType,ValType,_Compare> *hPtr = NULL;

    oldSize = numBuckets;
    oldBuckets = buckets;

    /*
     * Allocate and initialize the new bucket array, and set up
     * hashing constants for new array size.
     */

    numBuckets *= 4;

    buckets = (RpDictEntry<KeyType,ValType,_Compare> **) malloc((unsigned)
        (numBuckets * sizeof(RpDictEntry<KeyType,ValType,_Compare> *)));

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
template <typename KeyType, typename ValType, class _Compare>
unsigned int
RpDict<KeyType,ValType,_Compare>::hashFxn(const void *keyPtr) const
{
    const char *stopAddr = (const char *) keyPtr + sizeof(&keyPtr) - 1 ;
    const char *str = (const char *) keyPtr;
    unsigned int result = 0;
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
 * Quote from Tcl's hash table code
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
template <typename KeyType, typename ValType, class _Compare>
unsigned int
RpDict<KeyType,ValType,_Compare>::hashFxn(std::string* keyPtr) const
{
    const char *str = (const char *) (keyPtr->c_str());
    unsigned int result = 0;
    int c = 0;

    result = 0;

    while (1) {
        // c = *str;

        if (caseInsensitive == true) {
            c = toupper(static_cast<unsigned char>(*str));
        }
        else {
            c = *str;
        }

        if (c == 0) {
            break;
        }
        result += (result<<3) + c;
        str++;
    }

    return result;
}

template <typename KeyType, typename ValType, class _Compare>
unsigned int
RpDict<KeyType,ValType,_Compare>::hashFxn(char* keyPtr) const
{
    const char *str = (const char *) (keyPtr);
    unsigned int result = 0;
    int c = 0;

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
template <typename KeyType, typename ValType, class _Compare>
int
RpDict<KeyType,ValType,_Compare>::randomIndex(unsigned int hash)
{
    return (((((long) (hash))*1103515245) >> downShift) & mask);
}

#endif
