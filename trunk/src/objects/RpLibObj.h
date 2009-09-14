/*
 * ----------------------------------------------------------------------
 *  Rappture Library Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RP_LIBRARY_H
#define _RP_LIBRARY_H

enum RP_LIBRARY_CONSTS {
    RPLIB_OVERWRITE     = 0,
    RPLIB_APPEND        = 1,
    RPLIB_NO_TRANSLATE  = 0,
    RPLIB_TRANSLATE     = 1,
    RPLIB_NO_COMPRESS   = 0,
    RPLIB_COMPRESS      = 1,
};


#ifdef __cplusplus

#include "RpBuffer.h"
#include "RpOutcome.h"
#include "RpInt.h"
#include "RpChain.h"
#include "RpParserXML.h"

namespace Rappture {

class Library
{
    public:
        Library();
        // Library(const Library& o);
        virtual ~Library();

        Outcome &loadXml(const char *xmltext);
        Outcome &loadFile(const char *filename);


        // Library *value(const char *key, void *storage);

        // Rp_Chain *diff(Library *lib);

        // Library *remove (const char *key);

        const char *xml() const;

        Outcome &outcome() const;

        // Library *result(int status);

        const Rp_Chain *contains() const;
    private:
        Rp_Chain *_objList;
        // Rp_HashTable _objHash;
        mutable Rappture::Outcome _status;

        void __libInit();
        void __libFree();
        void __parseTree2ObjectList(Rp_ParserXml *p, Rp_Chain *retObjList);

}; // end class Library

} // end namespace Rappture


#endif // ifdef __cplusplus

#endif // ifndef _RP_LIBRARY_H
