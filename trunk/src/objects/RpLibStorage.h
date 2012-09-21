/*
 * ----------------------------------------------------------------------
 *  Rappture Library Storage Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RP_LIBRARY_STORAGE_H
#define _RP_LIBRARY_STORAGE_H

#ifdef __cplusplus

#include "RpObject.h"
#include "RpOutcome.h"
#include "RpInt.h"
#include "RpChain.h"
#include "RpHash.h"

namespace Rappture {

class LibraryStorage
{
    public:
        LibraryStorage();
        virtual ~LibraryStorage();

        /*
        Outcome &backup();
        Outcome &restore();
        */

        Outcome &store(const char *key, Rappture::Object *o);
        Outcome &link(const char *oldkey, const char *newkey);
        Rappture::Object *find(const char *key);
        Rappture::Object *remove(const char *key);
        Outcome &clear();

        Outcome &outcome() const;
        const Rp_Chain *contains() const;
    private:

        // use a chain because the rappture gui says that order
        // of the objects in the input and output sections of
        // the xml file matters.  use a hash table to easily
        // find items by name or path the hash table holds name
        // and paths of the objects and points to the
        // Rp_ChainLink so we can easily remove a value from
        // the hash table and the chain.  watchout! the link()
        // function allows multiple hash entries to point to
        // the same Rp_ChainLink.  this means the remove()
        // function can delete the Rappture::Object held by a
        // Rp_ChainLink, but the Rp_ChainLink will not be
        // deleted. _objList can have holes in it. there is
        // currently no way to tell how many things point to
        // it. also, the hash table never deletes it's
        // ClientData (the Rp_ChainLink). this object does not
        // store NULL objects. NULL objects look to close to
        // things that have already been deleted. if the user
        // calls find() and the result is Rp_ChainLink that
        // points to a NULL object, the user will get NULL back
        // and the Rp_ChainLink will be removed along with the
        // corresponding hash entry. bad Rp_ChainLinks are also
        // cleaned up when the user calls the contains()
        // function.

        Rp_Chain *_objList;
        Rp_HashTable *_objNameHash;
        mutable Rappture::Outcome _status;

        void __libStoreInit();
        void __libStoreFree();
        Rappture::Object *__find(const char *key, size_t flags);

}; // end class LibrarayStorage

} // end namespace Rappture


#endif // ifdef __cplusplus

#endif // ifndef _RP_LIBRARY_H
