/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_OBJECT_H
#define RAPPTURE_OBJECT_H

#include "RpInt.h"
#include "RpHash.h"
#include "RpAccessor.h"
#include "RpBuffer.h"
#include "RpPath.h"
#include "RpParserXML.h"
#include "RpObjConfig.h"

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
        const void *property (const char *key) const;
        void property (const char *key, const void *val, size_t nbytes);

        // get/set const char * property
        const char *propstr (const char *key) const;
        void propstr (const char *key, const char *val);

        // remove property from hash table
        void propremove (const char *key);

        // get the Rappture1.1 xml text for this object
        virtual const char *xml(size_t indent, size_t tabstop) const;

        // set the object properties based on Rappture1.1 xml text
        // virtual void xml(const char *xmltext);

        // configure the object properties based on Rappture1.1 xml text
        virtual void configure(size_t as, void *p);
        virtual void dump(size_t as, void *p);

        virtual const int is() const;

    protected:

        /// temprorary buffer for returning text to the user
        SimpleCharBuffer _tmpBuf;

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
