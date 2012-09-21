/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_OBJECT_H
#define RAPPTURE_OBJECT_H

#include "RpInt.h"
#include "RpHash.h"
#include "RpOutcome.h"
#include "RpAccessor.h"
#include "RpBuffer.h"
#include "RpPath.h"
#include "RpParserXML.h"
#include "RpObjConfig.h"
#include <cstdarg>

namespace Rappture {

class Object
{
    public:
        Object ();
        Object (const char *name,
                const char *path,
                const char *label,
                const char *desc,
                const char *hints,
                const char *color);
        Object (const Object& o);
        virtual ~Object();

        const char *name(void) const;
        void name(const char *p);

        const char *path(void) const;
        void path(const char *p);

        const char *label(void) const;
        void label(const char *p);

        const char *desc(void) const;
        void desc(const char *p);

        const char *hints(void) const;
        void hints(const char *p);

        const char *color(void) const;
        void color(const char *p);

        // void *icon(void) const;
        // void icon(void *p, size_t nbytes);

        // these functions are not completely improper use proof
        // user is responsible for calling propremove() on any item
        // they put into the hash table.

        // get/set void* property
        const void *property(const char *key) const;
        void property(const char *key, const void *val, size_t nbytes);

        // get/set const char * property
        const char *propstr(const char *key) const;
        void propstr(const char *key, const char *val);

        // remove property from hash table
        void propremove (const char *key);

        // return the value of object based on provided hints
        virtual void vvalue(void *storage, size_t numHints, va_list arg) const;
        // populate the object with a random value
        virtual void random();
        // return the difference between this object and o
        virtual Rp_Chain *diff(const Object& o);

        // get the Rappture1.1 xml text for this object
        // virtual const char *xml(size_t indent, size_t tabstop) const;

        // configure the object properties based on Rappture1.1 xml text
        virtual void configure(size_t as, ClientData c);
        virtual void dump(size_t as, ClientData c);

        virtual Outcome &outcome() const;
        virtual const int is() const;

    protected:

        /// temprorary buffer for returning text to the user
        SimpleCharBuffer _tmpBuf;

        /// status of the object
        mutable Rappture::Outcome _status;

        virtual void __hintParser(char *hint,
            const char **hintKey, const char **hintVal) const;

        virtual void __configureFromXml(ClientData c);
        virtual void __configureFromTree(ClientData c);
        virtual void __dumpToXml(ClientData c);
        virtual void __dumpToTree(ClientData c);
    private:

        /// hash table holding other object properties
        Rp_HashTable *_h;

        /// initialize the object, set the data members to 0 or null
        void __init();

        /// close out the object, freeing its memory
        void __clear();
};

} // RAPPTURE_OBJECT_H

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
