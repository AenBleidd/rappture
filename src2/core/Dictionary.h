/*
 * ======================================================================
 *  Rappture::Dictionary<keyType,valType>
 *
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
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

class DictionaryCore;  // foward declaration

/**
 * Represents one entry within a dictionary object, cast in terms
 * of void pointers and data sizes, so that the official, templated
 * version of the class can be lean and mean.
 */
struct DictEntryCore {
    DictEntryCore *nextPtr;    // pointer to next entry in this bucket
    DictionaryCore *dictPtr;   // pointer to dictionary containing entry
    DictEntryCore **bucketPtr; // pointer to bucket containing this entry

    void *valuePtr;            // value within this hash entry

    union {
        void *ptrValue;        // size of a pointer
        int words[1];          // multiple integer words for the key
        char string[4];        // first part of string value for key
    } key;                     // memory can be oversized if more
                               // space is needed for words/string
                               // MUST BE THE LAST FIELD IN THIS RECORD!
};

/**
 * This is the functional core of a dictionary object, cast in terms
 * of void pointers and data sizes, so that the official, templated
 * version of the class can be lean and mean.
 */
class DictionaryCore {
public:
    DictionaryCore(size_t keySize);
    ~DictionaryCore();

    DictEntryCore* get(void* key, int* newEntryPtr);
    DictEntryCore* find(void* key);
    DictEntryCore* erase(DictEntryCore* entryPtr);
    DictEntryCore* next(DictEntryCore* entryPtr, int& bucketNum);
    std::string stats();

private:
    /// Not allowed -- copy Dictionary at higher level
    DictionaryCore(const DictionaryCore& dcore) { assert(0); }

    /// Not allowed -- copy Dictionary at higher level
    DictionaryCore& operator=(const DictionaryCore& dcore) { assert(0); }

    // utility functions
    void _rebuildBuckets();
    unsigned int _hashIndex(void* key);

    /// Size of the key in words, or 0 for (const char*) string keys.
    int _keySize;

    /// Pointer to array of hash buckets.
    DictEntryCore **_buckets;

    /// Built-in storage for small hash tables.
    DictEntryCore *_staticBuckets[RP_DICT_MIN_SIZE];

    /// Total number of entries in the dictionary.
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
 * This represents a single entry within a dictionary object.  It is
 * used to access values within the dictionary, or as an iterator
 * that traverses the dictionary.
 */
template <class KeyType, class ValType>
class DictEntry {
public:
    DictEntry(Ptr<DictionaryCore> corePtr, DictEntryCore *entryPtr=NULL);
    DictEntry(const DictEntry& entry);
    DictEntry& operator=(const DictEntry& entry);
    ~DictEntry();

    int isNull() const;
    KeyType& key();
    ValType& value();
    DictEntry& next();
    DictEntry& erase();

private:
    Ptr<DictionaryCore> _corePtr;
    DictEntryCore *_entryPtr;
    int _bucketNum;
};

template <class KeyType, class ValType>
DictEntry<KeyType,ValType>::DictEntry(Ptr<DictionaryCore> corePtr,
    DictEntryCore *entryPtr)
    : _corePtr(corePtr),
      _entryPtr(entryPtr),
      _bucketNum(0)
{
}

template <class KeyType, class ValType>
DictEntry<KeyType,ValType>::DictEntry(const DictEntry &entry)
    : _corePtr(entry._corePtr),
      _entryPtr(entry._entryPtr),
      _bucketNum(entry._bucketNum)
{
}

template <class KeyType, class ValType>
DictEntry<KeyType,ValType>&
DictEntry<KeyType,ValType>::operator=(const DictEntry &entry)
{
    _corePtr = entry._corePtr;
    _entryPtr = entry._entryPtr;
    _bucketNum = entry._bucketNum;
    return *this;
}

template <class KeyType, class ValType>
DictEntry<KeyType,ValType>::~DictEntry()
{
}

template <class KeyType, class ValType>
int
DictEntry<KeyType,ValType>::isNull() const
{
    return (_entryPtr == NULL);
}

template <class KeyType, class ValType>
KeyType&
DictEntry<KeyType,ValType>::key()
{
    return *(KeyType*)(_entryPtr->key.string);
}

template <class KeyType, class ValType>
ValType&
DictEntry<KeyType,ValType>::value()
{
    return *static_cast<ValType*>(_entryPtr->valuePtr);
}

template <class KeyType, class ValType>
DictEntry<KeyType,ValType>&
DictEntry<KeyType,ValType>::next()
{
    _entryPtr = _corePtr->next(_entryPtr, _bucketNum);
    return *this;
}

template <class KeyType, class ValType>
DictEntry<KeyType,ValType>&
DictEntry<KeyType,ValType>::erase()
{
    _entryPtr = _corePtr->erase(_entryPtr);

    if (_entryPtr == NULL) {
        _corePtr->next(_entryPtr, _bucketNum);
    }
    return *this;
}

/**
 * This is a dictionary object or associative array.  It is a hash
 * table that uniquely maps a key to a value.  The memory for values
 * added to dictionary is managed by the dictionary.  When a dictionary
 * is deleted, its internal values are cleaned up automatically.
 */
template <class KeyType, class ValType>
class Dictionary {
public:
    Dictionary();
    Dictionary(const Dictionary& dict);
    Dictionary& operator=(const Dictionary& dict);
    ~Dictionary();

    DictEntry<KeyType,ValType> get(const KeyType& key, int* newEntryPtr);
    DictEntry<KeyType,ValType> find(const KeyType& key) const;
    DictEntry<KeyType,ValType> erase(const KeyType& key);
    void clear();

    DictEntry<KeyType,ValType> first();

    std::string stats();

private:
    Ptr<DictionaryCore> _corePtr;
};

template <class KeyType, class ValType>
Dictionary<KeyType,ValType>::Dictionary()
{
    _corePtr = Ptr<DictionaryCore>( new DictionaryCore(sizeof(KeyType)) );
}

template <class KeyType, class ValType>
Dictionary<KeyType,ValType>::Dictionary(const Dictionary<KeyType,ValType>& dict)
{
    _corePtr = Ptr<DictionaryCore>( new DictionaryCore(sizeof(KeyType)) );

    DictEntry<KeyType,ValType> entry;
    for (entry=dict.first(); !entry.isNull(); entry.next()) {
        get(entry.key(), NULL).value() = entry.value();
    }
}

template <class KeyType, class ValType>
Dictionary<KeyType,ValType>&
Dictionary<KeyType,ValType>::operator=(const Dictionary<KeyType,ValType>& dict)
{
    *_corePtr = *dict._corePtr;

    DictEntry<KeyType,ValType> entry;
    for (entry=dict.first(); !entry.isNull(); entry.next()) {
        get(entry.key(), NULL).value() = entry.value();
    }
}

template <class KeyType, class ValType>
Dictionary<KeyType,ValType>::~Dictionary()
{
    clear();
}

template <class KeyType, class ValType>
void
Dictionary<KeyType,ValType>::clear()
{
    DictEntry<KeyType,ValType> entry = first();
    while (!entry.isNull()) {
        entry.erase();
    }
}

template <class KeyType, class ValType>
DictEntry<KeyType,ValType>
Dictionary<KeyType,ValType>::get(const KeyType& key, int* newEntryPtr)
{
    DictEntryCore* entryPtr = _corePtr->get((void*)&key, newEntryPtr);
    if (entryPtr->valuePtr == NULL) {
        entryPtr->valuePtr = new ValType;
    }
    return DictEntry<KeyType,ValType>(_corePtr, entryPtr);
}

template <class KeyType, class ValType>
DictEntry<KeyType,ValType>
Dictionary<KeyType,ValType>::first()
{
    DictEntry<KeyType,ValType> entry(_corePtr);
    return entry.next();
}

template <class KeyType, class ValType>
std::string
Dictionary<KeyType,ValType>::stats()
{
    return _corePtr->stats();
}


} // namespace Rappture

#endif
