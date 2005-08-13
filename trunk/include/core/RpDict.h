#include <iostream>
#include <cassert>
#include <string>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>

#ifndef _RpDICT_H 
#define _RpDICT_H

/**************************************************************************/


/**************************************************************************/

template <typename KeyType, typename ValType> class RpDict;
template <typename KeyType, typename ValType> class RpDictEntry;

/**************************************************************************/

template <typename KeyType, typename ValType> class RpDictIterator
{

    public:

        // public data members

        // public member functions

        // retrieve the table the iterator is iterating
        // virtual RpDict<KeyType,ValType>& getTable();

        // send the search iterator to the beginning of the hash table
        /*virtual*/ RpDictEntry<KeyType,ValType>* first();

        // send the search iterator to the next element of the hash table
        /*virtual*/ RpDictEntry<KeyType,ValType>* next();
/*
        RpDictIterator(RpDict* table_Ptr)
            : tablePtr( (RpDict&) *table_Ptr),
              srchNextEntryPtr(NULL),
              srchNextIndex(0)
        {
        }
*/
        RpDictIterator(RpDict<KeyType,ValType>& table_Ptr)
            : tablePtr(table_Ptr),
              srchNextIndex(0),
              srchNextEntryPtr(NULL)
        {
        }

        // copy constructor
        RpDictIterator(RpDictIterator<KeyType,ValType>& iterRef)
            : tablePtr(iterRef.tablePtr),
              srchNextIndex(iterRef.srchNextIndex),
              srchNextEntryPtr(iterRef.srchNextEntryPtr)
        {
        }

        // destructor

    private:

        RpDict<KeyType,ValType>& 
            tablePtr;                   /* pointer to the table we want to 
                                         * iterate */
        int srchNextIndex;              /* Index of next bucket to be
                                         * enumerated after present one. */
        RpDictEntry<KeyType,ValType>* 
            srchNextEntryPtr;           /* Next entry to be enumerated in the
                                         * the current bucket. */

};


template <typename KeyType, typename ValType> class RpDictEntry
{
    public:

        // public member functions 

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

        // erases this entry from its table
        void erase();

        // template <KeyType,ValType> friend class RpDict;
        // template <KeyType,ValType> friend class RpDictIterator;

        friend class RpDict<KeyType,ValType>;
        friend class RpDictIterator<KeyType,ValType>;

        // no_arg constructor
        // use the key and clientData's default [no-arg] constructor
        RpDictEntry()
           : nextPtr (NULL), 
             tablePtr (NULL), 
             hash (0) // , 
             // clientData (),
             // key ()
        {
        }

        // one-arg constructor
        // maybe get rid of this one?
        RpDictEntry(KeyType newKey)
           : nextPtr    (NULL), 
             tablePtr   (NULL), 
             hash       (0), 
             clientData (NULL),
             key        (newKey)
        {
        }

        // two-arg constructor
        RpDictEntry(KeyType newKey, ValType newVal)
           : nextPtr    (NULL), 
             tablePtr   (NULL), 
             hash       (0), 
             clientData (newVal),
             key        (newKey)
        {
        }

/*
        RpDictEntry(KeyType* newKey, ValType* newVal)
           : nextPtr    (NULL), 
             tablePtr   (NULL), 
             hash       (0), 
             clientData (*newVal),
             key        (*newKey)
        {
        }

        RpDictEntry(KeyType newKey, RpDict* table)
           : nextPtr    (NULL), 
             tablePtr   (table), 
             hash       (0), 
             // clientData (NULL),
             key        (newKey)
        {
        }
*/
        // copy constructor
        RpDictEntry (const RpDictEntry<KeyType,ValType>& entry)
        {
            nextPtr     = entry.nextPtr;
            tablePtr    = entry.tablePtr;
            hash        = entry.hash;
            clientData  = (ValType) entry.getValue();
            key         = (KeyType) entry.getKey();
        }

    private:

        // private data members
        RpDictEntry<KeyType,ValType>*
            nextPtr;                /* Pointer to next entry in this
                                     * hash bucket, or NULL for end of
                                     * chain. */

        RpDict<KeyType,ValType> 
            *tablePtr;              /* Pointer to table containing entry. */

        unsigned int hash;          /* Hash value. */

        ValType clientData;        /* Application stores something here
                                     * with Tcl_SetHashValue. */

        KeyType key;               /* entry key */



        
};


template <typename KeyType, typename ValType> class RpDict
{
    public:

        
        // functionality for the user to access/adjust data members
        
        // checks table size
        /*virtual*/ const int size() const;

        // insert new object into table
        // returns 0 on success (object inserted or already exists)
        // returns !0 on failure (object cannot be inserted or dne)
        //
        /*virtual*/ RpDict<KeyType,ValType>& 
                        set(    KeyType& key, 
                                ValType& value, 
                                int *newPtr=NULL );   

        // find an RpUnits object that should exist in RpUnitsTable
        // 
        /*virtual*/ RpDictEntry<KeyType,ValType>& 
                        find( KeyType& key );

        /*virtual*/ RpDictEntry<KeyType,ValType>& operator[]( KeyType& key) 
        { 
            return find(key); 
        }

        // clear the entire table
        // iterate through the table and call erase on each element
        /*virtual*/ RpDict<KeyType,ValType>& clear();

        // get the nullEntry hash entry for initialization of references
        /*virtual*/ RpDictEntry<KeyType,ValType>& getNullEntry();

        // template <KeyType, ValType> friend class RpDictEntry;
        // template <KeyType, ValType> friend class RpDictIterator;

        friend class RpDictEntry<KeyType, ValType>;
        friend class RpDictIterator<KeyType, ValType>;

        // default constructor
        RpDict () 
            : SMALL_RP_DICT_SIZE(4),
              REBUILD_MULTIPLIER(3),
              buckets(staticBuckets),
              numBuckets(SMALL_RP_DICT_SIZE),
              numEntries(0),
              rebuildSize(SMALL_RP_DICT_SIZE*REBUILD_MULTIPLIER),
              downShift(28),
              mask(3)
        { 

            staticBuckets[0] = staticBuckets[1] = 0;
            staticBuckets[2] = staticBuckets[3] = 0;

            // setup a dummy entry of NULL
            nullEntry = new RpDictEntry<KeyType,ValType>();

            // std::cout << "inside RpDict Constructor" << std::endl;

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
        
        RpDictEntry<KeyType,ValType> 
                    **buckets;        /* Pointer to bucket array.  Each
                                       * element points to first entry in
                                       * bucket's hash chain, or NULL. */
        // RpDictEntry *staticBuckets[SMALL_RP_DICT_SIZE];
        RpDictEntry<KeyType,ValType>
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
        RpDictEntry<KeyType,ValType>
                    *nullEntry;   /* if not const, compiler complains*/



        // private member fxns

        // static void RpDict::RebuildTable ();
        void RebuildTable ();

        unsigned int hashFxn(const void* keyPtr) const;
        unsigned int hashFxn(std::string* keyPtr) const;
        unsigned int hashFxn(char* keyPtr) const;

        int randomIndex(unsigned int hash);
};


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include "../../src/core/RpDict.cc"

#endif
