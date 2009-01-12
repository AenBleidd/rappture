/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RpVARIABLE_H
#define _RpVARIABLE_H

#include "RpInt.h"
#include "RpHash.h"

class RpVariable
{
    public:
        RpVariable ();
        RpVariable (const RpVariable& o);
        virtual ~RpVariable();

        virtual const char *label           (const char *val);
        virtual const char *desc            (const char *val);
        virtual const char *hints           (const char *val);
        virtual const char *color           (const char *val);
        virtual const char *icon            (const char *val);
        virtual const char *path            (const char *val);
        virtual const void *property        (const char *key, const void *val);

    private:

        /// path of the object in the xml tree and key in hash tables
        const char *_path;

        /// hash table holding other object properties
        Rp_HashTable *_h;

        /// initialize the object, set the data members to 0 or null
        void __init();

        /// close out the object, freeing its memory
        void __close();
};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
