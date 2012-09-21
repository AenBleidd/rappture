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
#ifndef RAPPTURE_DICTIONARY_H
#define RAPPTURE_DICTIONARY_H

#include "rappture.h"
#include <stddef.h>

#define RP_DICT_MIN_SIZE 4

namespace Rappture {

class LookupCore;  // foward declaration

/**
 * Represents one entry within a lookup object, cast in terms
 * of void pointers and data sizes, so that the official, templated
 * version of the class can be lean and mean.
 */
struct LookupEntryCore {
    LookupEntryCore *nextPtr;    // pointer to next entry in this bucket
    LookupCore *dictPtr;         // pointer to lookup containing entry
    LookupEntryCore **bucketPtr; // pointer to bucket containing this entry

    void *valuePtr;              // value within this hash entry

    union {
        void *ptrValue;          // size of a pointer
        int words[1];            // multiple integer words for the key
        char string[4];          // first part of string value for key
    } key;                       // memory can be oversized if more
                                 // space is needed for words/string
                                 // MUST BE THE LAST FIELD IN THIS RECORD!
};

/**
 * This is the functional core of a lookup object, cast in terms
 * of void pointers and data sizes, so that the official, templated
 * version of the class can be lean and mean.
 */
class LookupCore {
public:
    LookupCore(size_t keySize);
    ~LookupCore();

    LookupEntryCore* get(void* key, int* newEntryPtr);
    LookupEntryCore* find(void* key);
    LookupEntryCore* erase(LookupEntryCore* entryPtr);
    LookupEntryCore* next(LookupEntryCore* entryPtr, int& bucketNum);
    std::string stats();

private:
    /// Not allowed -- copy Lookup at higher level
    LookupCore(const LookupCore& dcore) { assert(0); }

    /// Not allowed -- copy Lookup at higher level
    LookupCore& operator=(const LookupCore& dcore) { assert(0); }

    // utility functions
    void _rebuildBuckets();
    unsigned int _hashIndex(void* key);

    /// Size of the key in words, or 0 for (const char*) string keys.
    int _keySize;

    /// Pointer to array of hash buckets.
    LookupEntryCore **_buckets;

    /// Built-in storage for small hash tables.
    LookupEntryCore *_staticBuckets[RP_DICT_MIN_SIZE];

    /// Total number of entries in the lookup.
    int _numEntries;

    /// Number of buckets currently allocated at _buckets.
    int _numBuckets;

    /// Enlarge table when numEntries gets to be this large.
    int _rebuildSize;

    /// Shift count used in hashing function.
    int _downShift;

    /// Mask value used in hashing function.
    int _mask;
};

/**
 * This represents a single entry within a lookup object.  It is
 * used to access values within the lookup, or as an iterator
 * that traverses the lookup.
 */
template <class KeyType, class ValType>
class LookupEntry2 {
public:
    LookupEntry2(LookupCore* corePtr, LookupEntryCore* entryPtr=NULL);
    LookupEntry2(const LookupEntry2& entry);
    LookupEntry2& operator=(const LookupEntry2& entry);
    ~LookupEntry2();

    int isNull() const;
    KeyType& key();
    ValType& value();
    LookupEntry2& next();
    LookupEntry2& erase();

private:
    LookupCore* _corePtr;
    LookupEntryCore* _entryPtr;
    int _bucketNum;
};

template <class KeyType, class ValType>
LookupEntry2<KeyType,ValType>::LookupEntry2(LookupCore* corePtr,
    LookupEntryCore* entryPtr)
    : _corePtr(corePtr),
      _entryPtr(entryPtr),
      _bucketNum(0)
{
}

template <class KeyType, class ValType>
LookupEntry2<KeyType,ValType>::LookupEntry2(const LookupEntry2 &entry)
    : _corePtr(entry._corePtr),
      _entryPtr(entry._entryPtr),
      _bucketNum(entry._bucketNum)
{
}

template <class KeyType, class ValType>
LookupEntry2<KeyType,ValType>&
LookupEntry2<KeyType,ValType>::operator=(const LookupEntry2 &entry)
{
    _corePtr = entry._corePtr;
    _entryPtr = entry._entryPtr;
    _bucketNum = entry._bucketNum;
    return *this;
}

template <class KeyType, class ValType>
LookupEntry2<KeyType,ValType>::~LookupEntry2()
{
}

template <class KeyType, class ValType>
int
LookupEntry2<KeyType,ValType>::isNull() const
{
    return (_entryPtr == NULL);
}

template <class KeyType, class ValType>
KeyType&
LookupEntry2<KeyType,ValType>::key()
{
    return *(KeyType*)(_entryPtr->key.string);
}

template <class KeyType, class ValType>
ValType&
LookupEntry2<KeyType,ValType>::value()
{
    if (sizeof(ValType) > sizeof(_entryPtr->valuePtr)) {
        return *static_cast<ValType*>(_entryPtr->valuePtr);
    }
    return *(ValType*)(&_entryPtr->valuePtr);
}

template <class KeyType, class ValType>
LookupEntry2<KeyType,ValType>&
LookupEntry2<KeyType,ValType>::next()
{
    _entryPtr = _corePtr->next(_entryPtr, _bucketNum);
    return *this;
}

template <class KeyType, class ValType>
LookupEntry2<KeyType,ValType>&
LookupEntry2<KeyType,ValType>::erase()
{
    if (_entryPtr != NULL) {
        // delete the value
        if (_entryPtr->valuePtr
              && sizeof(ValType) > sizeof(_entryPtr->valuePtr)) {
            delete static_cast<ValType*>(_entryPtr->valuePtr);
        }
        // delete the slot
        _entryPtr = _corePtr->erase(_entryPtr);
    }

    if (_entryPtr == NULL) {
        // at the end of this bucket? then move on to next one
        _corePtr->next(_entryPtr, _bucketNum);
    }
    return *this;
}

/**
 * This is a lookup object or associative array.  It is a hash
 * table that uniquely maps a key to a value.  The memory for values
 * added to lookup is managed by the lookup.  When a lookup
 * is deleted, its internal values are cleaned up automatically.
 */
template <class KeyType, class ValType>
class Lookup2 {
public:
    Lookup2(int size=-1);
    Lookup2(const Lookup2& dict);
    Lookup2& operator=(const Lookup2& dict);
    ~Lookup2();

    LookupEntry2<KeyType,ValType> get(const KeyType& key, int* newEntryPtr);
    LookupEntry2<KeyType,ValType> find(const KeyType& key) const;
    ValType& operator[](const KeyType& key);
    LookupEntry2<KeyType,ValType> erase(const KeyType& key);
    void clear();

    LookupEntry2<KeyType,ValType> first();

    std::string stats();

private:
    LookupCore* _corePtr;
};

template <class KeyType, class ValType>
Lookup2<KeyType,ValType>::Lookup2(int size)
{
    _corePtr = new LookupCore( (size >= 0) ? size : sizeof(KeyType) );
}

template <class KeyType, class ValType>
Lookup2<KeyType,ValType>::Lookup2(const Lookup2<KeyType,ValType>& dict)
{
    _corePtr = new LookupCore(sizeof(KeyType));

    LookupEntry2<KeyType,ValType> entry;
    for (entry=dict.first(); !entry.isNull(); entry.next()) {
        get(entry.key(), NULL).value() = entry.value();
    }
}

template <class KeyType, class ValType>
Lookup2<KeyType,ValType>&
Lookup2<KeyType,ValType>::operator=(const Lookup2<KeyType,ValType>& dict)
{
#ifdef notdef
	/* How did this ever compile? --gah */
    _corePtr->clear();
#endif

    LookupEntry2<KeyType,ValType> entry;
    for (entry=dict.first(); !entry.isNull(); entry.next()) {
        get(entry.key(), NULL).value() = entry.value();
    }
}

template <class KeyType, class ValType>
Lookup2<KeyType,ValType>::~Lookup2()
{
    clear();
    delete _corePtr;
}

template <class KeyType, class ValType>
void
Lookup2<KeyType,ValType>::clear()
{
    LookupEntry2<KeyType,ValType> entry = first();
    while (!entry.isNull()) {
        entry.erase();
    }
}

template <class KeyType, class ValType>
LookupEntry2<KeyType,ValType>
Lookup2<KeyType,ValType>::get(const KeyType& key, int* newEntryPtr)
{
    LookupEntryCore* entryPtr = _corePtr->get((void*)&key, newEntryPtr);
    if (entryPtr->valuePtr == NULL
          && sizeof(ValType) > sizeof(entryPtr->valuePtr)) {
        entryPtr->valuePtr = new ValType;
    }
    return LookupEntry2<KeyType,ValType>(_corePtr, entryPtr);
}

template <class KeyType, class ValType>
ValType&
Lookup2<KeyType,ValType>::operator[](const KeyType& key)
{
    LookupEntry2<KeyType,ValType> entry = get(key, NULL);
    return entry.value();
}

template <class KeyType, class ValType>
LookupEntry2<KeyType,ValType>
Lookup2<KeyType,ValType>::first()
{
    LookupEntry2<KeyType,ValType> entry(_corePtr);
    return entry.next();
}

template <class KeyType, class ValType>
std::string
Lookup2<KeyType,ValType>::stats()
{
    return _corePtr->stats();
}

template <class ValType>
class LookupEntry : public LookupEntry2<const char*,ValType> {
public:
    LookupEntry(LookupCore* corePtr, LookupEntryCore* entryPtr=NULL);
    LookupEntry(const LookupEntry2<const char*,ValType>& entry);
};

template <class ValType>
LookupEntry<ValType>::LookupEntry(LookupCore* corePtr,
    LookupEntryCore* entryPtr)
    : LookupEntry2<const char*,ValType>(corePtr, entryPtr)
{
}

template <class ValType>
LookupEntry<ValType>::LookupEntry(
    const LookupEntry2<const char*,ValType>& entry)
    : LookupEntry2<const char*,ValType>(entry)
{
}

template <class ValType>
class Lookup : public Lookup2<const char*,ValType> {
public:
    Lookup();
};

template <class ValType>
Lookup<ValType>::Lookup()
    : Lookup2<const char*,ValType>(0)
{
}

} // namespace Rappture

#endif
