/*
 * ----------------------------------------------------------------------
 *  Rappture Library Storage Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpLibStorage.h"
#include "RpHashHelper.h"

#include <stdlib.h>

using namespace Rappture;

#define RP_LIBSTORE_FIND    0
#define RP_LIBSTORE_REMOVE  1

LibraryStorage::LibraryStorage ()
{
    _objList = NULL;
    _objNameHash = NULL;

    __libStoreInit();
}

LibraryStorage::~LibraryStorage ()
{
    __libStoreFree();

    _objList = NULL;
    _objNameHash = NULL;
}

void
LibraryStorage::__libStoreInit()
{
    _status.addContext("Rappture::LibraryStorage::__libStoreInit");

    _objList = Rp_ChainCreate();
    if (_objList == NULL) {
        _status.addError("Error while allocating space for list");
    }

    _objNameHash = (Rp_HashTable*) malloc(sizeof(Rp_HashTable));
    if (_objNameHash == NULL) {
        _status.addError("Error while allocating space for hash table");
    } else {
        Rp_InitHashTable(_objNameHash,RP_STRING_KEYS);
    }

    return;
}

void
LibraryStorage::__libStoreFree()
{
    _status.addContext("Rappture::LibraryStorage::__libStoreFree");

    if (_objList != NULL) {
        Rp_ChainLink *l = NULL;
        Rappture::Object *objVal = NULL;
        l = Rp_ChainFirstLink(_objList);
        while(l) {
            objVal = (Rappture::Object *) Rp_ChainGetValue(l);
            delete objVal;
            objVal = NULL;;
            l = Rp_ChainNextLink(l);
        }
        Rp_ChainDestroy(_objList);
        _objList = NULL;
    }

    if (_objNameHash != NULL) {
        // we dont need to delete the hash table entries because
        // they point into the chain and the chain has already
        // deleted the objects.

        Rp_DeleteHashTable(_objNameHash);
        free(_objNameHash);
        _objNameHash = NULL;
    }
    return;
}

/*
Outcome &
LibraryStorage::backup ()
{
    _status.addContext("Rappture::LibraryStorage::backup");

    return _status;
}

Outcome &
LibraryStorage::restore ()
{
    _status.addContext("Rappture::LibraryStorage::restore");

    return _status;
}
*/

Outcome &
LibraryStorage::store (const char *key, Rappture::Object *o)
{
    _status.addContext("Rappture::LibraryStorage::store");

    if (key == NULL) {
        return _status;
    }

    if (o == NULL) {
        return _status;
    }

    Rp_ChainLink *l = Rp_ChainAppend(_objList, (ClientData)o);
    if (l == NULL) {
        _status.addError("Error appending object to list");
        return _status;
    }

    Rp_HashAddNode(_objNameHash, key, (ClientData)l);

    return _status;
}

Outcome &
LibraryStorage::link (const char *oldkey, const char *newkey)
{
    _status.addContext("Rappture::LibraryStorage::link");

    if (oldkey == NULL) {
        return _status;
    }

    if (newkey == NULL) {
        return _status;
    }

    Rp_HashEntry *hEntry = Rp_FindHashEntry(_objNameHash,oldkey);
    if (hEntry != NULL) {
        Rp_ChainLink *l = (Rp_ChainLink *) Rp_GetHashValue(hEntry);
        if (l != NULL) {
            Rp_HashAddNode(_objNameHash, newkey, (ClientData)l);
        }
    }

    return _status;
}

Rappture::Object *
LibraryStorage::find (const char *key)
{
    _status.addContext("Rappture::LibraryStorage::find");
    return __find(key,RP_LIBSTORE_FIND);
}

Rappture::Object *
LibraryStorage::remove (const char *key)
{
    _status.addContext("Rappture::LibraryStorage::remove");
    return __find(key,RP_LIBSTORE_REMOVE);
}

Rappture::Object *
LibraryStorage::__find(const char *key, size_t flags)
{
    _status.addContext("Rappture::LibraryStorage::__find");

    if (key == NULL) {
        return NULL;
    }

    Rappture::Object *o = NULL;

    Rp_HashEntry *hEntry = Rp_FindHashEntry(_objNameHash,key);
    if (hEntry != NULL) {
        Rp_ChainLink *l = (Rp_ChainLink *) Rp_GetHashValue(hEntry);
        if (l != NULL) {
            // chain link exists, try to return its data
            o = (Rappture::Object *) Rp_ChainGetValue(l);
            if (o == NULL) {
                // invalid data in the chain, set remove flag
                flags |= RP_LIBSTORE_REMOVE;
            }

            if (RP_LIBSTORE_REMOVE & flags) {
                // remove flag set
                // remove it from the chain and hash
                Rp_ChainDeleteLink(_objList,l);
                Rp_DeleteHashEntry(_objNameHash,hEntry);
            }
        } else {
            // chain link does not exist, remove hash entry
            Rp_DeleteHashEntry(_objNameHash,hEntry);
        }
    }

    return o;
}

Outcome &
LibraryStorage::outcome() const
{
    _status.addContext("Rappture::LibraryStorage::outcome");
    return _status;
}

const Rp_Chain *
LibraryStorage::contains() const
{
    _status.addContext("Rappture::LibraryStorage::contains");
    //FIXME: run through hash table and remove invalid entries
    //       and holes from list before sending it to user
    return _objList;
}

Outcome &
LibraryStorage::clear ()
{
    _status.addContext("Rappture::LibraryStorage::clear");
    __libStoreFree();
    __libStoreInit();
    return _status;
}
