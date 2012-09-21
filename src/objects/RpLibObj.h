/*
 * ----------------------------------------------------------------------
 *  Rappture Library Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

#include "RpOutcome.h"
#include "RpInt.h"
#include "RpChain.h"
#include "RpParserXML.h"
#include "RpLibStorage.h"
#include <cstdarg>

namespace Rappture {

class Library
{
    public:
        Library();
        // Library(const Library& o);
        virtual ~Library();

        Outcome &loadXml(const char *xmltext);
        Outcome &loadFile(const char *filename);


        Library &value(const char *key, void *storage,
            size_t numHints, ...);

        // Rp_Chain *diff(Library *lib);

        // Library &remove (const char *key);

        const char *xml() const;

        Outcome &outcome() const;
        int error() const;

        // Outcome &result(int status);

        const Rp_Chain *contains() const;
    private:
        LibraryStorage _objStorage;
        mutable Outcome _status;

        void __libInit();
        void __libFree();
        void __parseTree2ObjectList(Rp_ParserXml *p);

}; // end class Library

} // end namespace Rappture


#endif // ifdef __cplusplus

#endif // ifndef _RP_LIBRARY_H
