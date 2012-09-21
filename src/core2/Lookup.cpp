/*
 * ======================================================================
 *  Rappture::Lookup<valType>
 *  Rappture::Lookup2<keyType,valType>
 *
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 *
 *  This code is based on the Tcl_HashTable facility included in the
 *  Tcl source release, which includes the following copyright:
 *
 *  Copyright (c) 1987-1994 The Regents of the University of California.
 *  Copyright (c) 1993-1996 Lucent Technologies.
 *  Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *  Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 *  This software is copyrighted by the Regents of the University of
 *  California, Sun Microsystems, Inc., Scriptics Corporation,
 *  and other parties.  The following terms apply to all files associated
 *  with the software unless explicitly disclaimed in individual files.
 *
 *  The authors hereby grant permission to use, copy, modify, distribute,
 *  and license this software and its documentation for any purpose, provided
 *  that existing copyright notices are retained in all copies and that this
 *  notice is included verbatim in any distributions. No written agreement,
 *  license, or royalty fee is required for any of the authorized uses.
 *  Modifications to this software may be copyrighted by their authors
 *  and need not follow the licensing terms described here, provided that
 *  the new terms are clearly indicated on the first page of each file where
 *  they apply.
 *
 *  IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *  FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *  ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *  DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
 *  IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
 *  NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 *  MODIFICATIONS.
 *
 *  GOVERNMENT USE: If you are acquiring this software on behalf of the
 *  U.S. government, the Government shall have only "Restricted Rights"
 *  in the software and related documentation as defined in the Federal
 *  Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *  are acquiring the software on behalf of the Department of Defense, the
 *  software shall be classified as "Commercial Computer Software" and the
 *  Government shall have only "Restricted Rights" as defined in Clause
 *  252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 *  authors grant the U.S. Government and others acting in its behalf
 *  permission to use and distribute the software in accordance with the
 *  terms specified in this license.
 * ======================================================================
 */
#include <sstream>
#include "Lookup.h"

using namespace Rappture;

/*
 * When there are this many entries per bucket, on average, rebuild
 * the hash table to make it larger.
 */
#define REBUILD_MULTIPLIER  3


LookupCore::LookupCore(size_t keySize)
{
    // convert key size in bytes to size in words
    _keySize = keySize/4 + ((keySize%4 > 0) ? 1 : 0);

    _numEntries = 0;
    _numBuckets = RP_DICT_MIN_SIZE;
    _buckets = _staticBuckets;
    for (int i=0; i < RP_DICT_MIN_SIZE; i++) {
        _staticBuckets[i] = NULL;
    }
    _rebuildSize = RP_DICT_MIN_SIZE*REBUILD_MULTIPLIER;
    _downShift = 28;
    _mask = 3;
}

LookupCore::~LookupCore()
{
    // free up all entries in the lookup table
    for (int i = 0; i < _numBuckets; i++) {
        LookupEntryCore *entryPtr = _buckets[i];
        while (entryPtr != NULL) {
            LookupEntryCore *nextPtr = entryPtr->nextPtr;
            delete entryPtr;
            entryPtr = nextPtr;
        }
    }

    // free up the bucket array, as long as it's not the built-in one
    if (_buckets != _staticBuckets) {
        delete [] _buckets;
    }
}

/**
 * Finds a lookup entry with the specified key.  Returns
 * the entry, or NULL if there was no matching entry.
 */
LookupEntryCore*
LookupCore::find(void* key)
{
    unsigned int index = _hashIndex(key);

    //
    // Search all of the entries in the appropriate bucket.
    //
    LookupEntryCore *hPtr;
    if (_keySize == 0) {
        for (hPtr=_buckets[index]; hPtr != NULL; hPtr=hPtr->nextPtr) {
            char* p1 = (char*)key;
            char* p2 = hPtr->key.string;
            while (1) {
                if (*p1 != *p2) {
                    break;
                }
                if (*p1 == '\0') {
                    return hPtr;
                }
                p1++, p2++;
            }
        }
    } else {
        for (hPtr=_buckets[index]; hPtr != NULL; hPtr=hPtr->nextPtr) {
            int *iPtr1 = (int*)key;
            int *iPtr2 = hPtr->key.words;
            for (int count = _keySize; ; count--, iPtr1++, iPtr2++) {
                if (count == 0) {
                    return hPtr;
                }
                if (*iPtr1 != *iPtr2) {
                    break;
                }
            }
        }
    }
    return NULL;
}

/**
 * Finds a lookup entry with the specified key.  Returns
 * the entry, or NULL if there was no matching entry.
 */
LookupEntryCore*
LookupCore::get(void* key, int* newEntryPtr)
{
    if (newEntryPtr) {
        *newEntryPtr = 0;
    }

    unsigned int index = _hashIndex(key);

    //
    // Search all of the entries in the appropriate bucket.
    //
    LookupEntryCore *hPtr;
    if (_keySize == 0) {
        for (hPtr=_buckets[index]; hPtr != NULL; hPtr=hPtr->nextPtr) {
            char* p1 = (char*)key;
            char* p2 = hPtr->key.string;
            while (1) {
                if (*p1 != *p2) {
                    break;
                }
                if (*p1 == '\0') {
                    return hPtr;
                }
                p1++, p2++;
            }
        }
    } else {
        for (hPtr=_buckets[index]; hPtr != NULL; hPtr=hPtr->nextPtr) {
            int *iPtr1 = (int*)key;
            int *iPtr2 = hPtr->key.words;
            for (int count = _keySize; ; count--, iPtr1++, iPtr2++) {
                if (count == 0) {
                    return hPtr;
                }
                if (*iPtr1 != *iPtr2) {
                    break;
                }
            }
        }
    }

    //
    // Entry not found.  Add a new one to the bucket.
    //
    if (newEntryPtr) {
        *newEntryPtr = 1;
    }
    if (_keySize == 0) {
        char* mem = new char[sizeof(LookupEntryCore)
            + strlen((char*)key) - sizeof(hPtr->key) + 1];
        hPtr = new (mem) LookupEntryCore;
    } else {
        char* mem = new char[sizeof(LookupEntryCore) + _keySize-1];
        hPtr = new (mem) LookupEntryCore;
    }
    hPtr->dictPtr = this;
    hPtr->bucketPtr = &(_buckets[index]);
    hPtr->nextPtr = *hPtr->bucketPtr;
    hPtr->valuePtr = NULL;
    *hPtr->bucketPtr = hPtr;
    _numEntries++;

    if (_keySize == 0) {
        strcpy(hPtr->key.string, (char*)key);
    } else {
        memcpy(hPtr->key.words, (int*)key, _keySize*4);
    }

    //
    // If the table has exceeded a decent size, rebuild it with many
    // more buckets.
    //
    if (_numEntries >= _rebuildSize) {
        _rebuildBuckets();
    }
    return hPtr;
}

/**
 * Removes an entry from the lookup table.  The data associated
 * with the entry is freed at the Lookup level before calling
 * this core method to clean up the slot.
 */
LookupEntryCore*
LookupCore::erase(LookupEntryCore* entryPtr)
{
    LookupEntryCore *prevPtr;

    if (*entryPtr->bucketPtr == entryPtr) {
        *entryPtr->bucketPtr = entryPtr->nextPtr;
    } else {
        for (prevPtr = *entryPtr->bucketPtr; ; prevPtr = prevPtr->nextPtr) {
            assert(prevPtr != NULL);
            if (prevPtr->nextPtr == entryPtr) {
                prevPtr->nextPtr = entryPtr->nextPtr;
                break;
            }
        }
    }
    entryPtr->dictPtr->_numEntries--;
    LookupEntryCore *nextPtr = entryPtr->nextPtr;
    delete entryPtr;

    return nextPtr;
}

/**
 * Returns the next entry after the given entry.  If then entry
 * is NULL and bucketNum is 0, it returns the very first entry.
 * When there are no more entries, it returns NULL.  The bucketNum
 * helps keep track of the next bucket in the table.
 */
LookupEntryCore*
LookupCore::next(LookupEntryCore *entryPtr, int& bucketNum)
{
    if (entryPtr) {
        entryPtr = entryPtr->nextPtr;
    }

    // hit the end of a bucket, or just starting? then go to next bucket
    while (entryPtr == NULL) {
	if (bucketNum >= _numBuckets) {
	    return NULL;
	}
	entryPtr = _buckets[bucketNum];
	bucketNum++;
    }
    return entryPtr;
}

/**
 * Returns a description of all entries in the lookup table.
 * Useful for debugging.
 */
std::string
LookupCore::stats()
{
#define NUM_COUNTERS 10
    int i, count[NUM_COUNTERS], overflow;
    double average, tmp;

    //
    // Compute a histogram of bucket usage.
    //
    for (i=0; i < NUM_COUNTERS; i++) {
        count[i] = 0;
    }
    overflow = 0;
    average = 0.0;
    for (i=0; i < _numBuckets; i++) {
        int j=0;
        for (LookupEntryCore *entryPtr=_buckets[i];
             entryPtr != NULL;
             entryPtr = entryPtr->nextPtr) {
            j++;
        }
        if (j < NUM_COUNTERS) {
            count[j]++;
        } else {
            overflow++;
        }
        tmp = j;
        average += 0.5*(tmp+1.0)*(tmp/_numEntries);
    }

    //
    // Send back the histogram and other stats.
    //
    std::ostringstream result;
    result << _numEntries << " entries in lookup table, ";
    result << _numBuckets << " buckets" << std::endl;

    for (i=0; i < NUM_COUNTERS; i++) {
        result << "number of buckets with " << i
               << " entries: " << count[i] << std::endl;
    }
    result << "number of buckets with " << NUM_COUNTERS
               << " or more entries: " << overflow << std::endl;
    result << "average search distance for entry: " << average << std::endl;
    return result.str();
}

/**
 * Invoked when the ratio of entries to hash buckets becomes too
 * large.  It creates a new layout with a larger bucket array
 * and moves all of the entries into the new table.
 */
void
LookupCore::_rebuildBuckets()
{
    int oldSize = _numBuckets;
    LookupEntryCore** oldBuckets = _buckets;

    //
    // Allocate and initialize the new bucket array, and set up
    // hashing constants for new array size.
    //
    _numBuckets *= 4;
    _buckets = new (LookupEntryCore*)[_numBuckets];
    LookupEntryCore **newChainPtr = _buckets;
    for (int count=_numBuckets; count > 0; count--, newChainPtr++) {
        *newChainPtr = NULL;
    }
    _rebuildSize *= 4;
    _downShift -= 2;
    _mask = (_mask << 2) + 3;

    //
    // Rehash all of the existing entries into the new bucket array.
    //
    for (LookupEntryCore** oldChainPtr=oldBuckets;
          oldSize > 0; oldSize--, oldChainPtr++) {

        for (LookupEntryCore* hPtr=*oldChainPtr;
              hPtr != NULL; hPtr=*oldChainPtr) {

            *oldChainPtr = hPtr->nextPtr;
            unsigned int index = _hashIndex((void*)hPtr->key.string);
            hPtr->bucketPtr = &(_buckets[index]);
            hPtr->nextPtr = *hPtr->bucketPtr;
            *hPtr->bucketPtr = hPtr;
        }
    }

    //
    // Free up the old bucket array, if it was dynamically allocated.
    //
    if (oldBuckets != _staticBuckets) {
        delete [] oldBuckets;
    }
}

/**
 * Computes the hash value for the specified key.
 */
unsigned int
LookupCore::_hashIndex(void* key)
{
    unsigned int result = 0;
    if (_keySize == 0) {
        char* string = (char*)key;
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
        while (1) {
            int c = *string;
            string++;
            if (c == 0) {
                break;
            }
            result += (result<<3) + c;
        }
        result &= _mask;
    } else {
        int *iPtr = (int*)key;
        for (int count=_keySize; count > 0; count--, iPtr++) {
            result += *iPtr;
        }
        result = (((((long)(result))*1103515245) >> _downShift) & _mask);
    }
    return result;
}
