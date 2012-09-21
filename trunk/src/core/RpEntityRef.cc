/*
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *  Also see below text for additional information on usage and redistribution
 *
 * ======================================================================
 * ----------------------------------------------------------------------
 *  Rappture Library Entity Reference Translation Header
 *
 *  Begin Character Entity Translator
 *
 *  The next section of code implements routines used to translate
 *  character entity references into their corresponding strings.
 *
 *  Examples:
 *
 *        &amp;          "&"
 *        &lt;           "<"
 *        &gt;           ">"
 *        &quot;         "\""
 *        &apos;         "'"
 *
 */

#include "RpEntityRef.h"
#include <cctype>
#include <cstring>

using namespace Rappture;

typedef struct {
    const char *replacement;                /* Replacement string. */
    size_t length;                        /* Length of replacement string,
                                         * including the ampersand. */
    const char *entity;                        /* Single character entity. */
} PredefEntityRef;

static PredefEntityRef predef[] = {
    { "&quot;",  6,  "\"" },
    { "&amp;",   5,  "&"  },
    { "&lt;",    4,  "<"  },
    { "&gt;",    4,  ">"  },
    { "&apos;",  6,  "'"  }
};
static int nPredefs = sizeof(predef) / sizeof (PredefEntityRef);

/*
 * EntityRef::decode --
 *
 *        Convert XML character data into the original text.  The trick
 *        here determine the runs of characters that do not require 
 *        replacements and to append the substring in one shot.  
 */
const char*
EntityRef::decode (const char* string, unsigned int len)
{
    if (string == NULL) {
        // Don't do anything with NULL strings.
        return NULL;
    }
    _bout.clear();
    if (len == 0) {
        len = strlen(string);
    }
    const char *p, *start, *pend;
    start = string;                     /* Mark the start of a run of
                                         * characters that contain no
                                         * replacements. */
    for (p = string, pend = p + len; p < pend; /*empty*/) {
        if (*p == '&') {
            PredefEntityRef *ep, *epend;
            for (ep = predef, epend = ep + nPredefs; ep < epend; ep++) {
                size_t length;
                length = pend - p;        /* Get the # bytes left. */
                if ((length >= ep->length) && (ep->replacement[1] == *(p+1)) &&
                    (strncmp(ep->replacement, p, ep->length) == 0)) {
                    /* Found entity replacement. Append any preceding
                     * characters into the buffer before the entity itself. */
                    if (p > start) {
                        _bout.append(start, p - start);
                    }
                    start = p + ep->length;
                    _bout.append(ep->entity, 1);
                    p += ep->length;
                    goto next;
                }
            }
        }
        p++;
    next:
        ;
    }
    if (p > start) {
        /* Append any left over characters into the buffer. */
        _bout.append(start, p - start);
    }
    _bout.append("\0", 1);
    return _bout.bytes();
}

const char*
EntityRef::encode (const char* string, unsigned int len)
{
    if (string == NULL) {
        return NULL;                   /* Don't do anything with NULL
                                        * strings. */
    }
    _bout.clear();
    if (len == 0) {
        len = strlen(string);
    }
    const char *p, *start, *pend;
    start = string;                     /* Mark the start of a run of
                                         * characters that contain no
                                         * replacements. */
    for (p = string, pend = p + len; p < pend; p++) {
        PredefEntityRef *ep, *epend;
        for (ep = predef, epend = ep + nPredefs; ep < epend; ep++) {
            if (ep->entity[0] == *p) {
                /* Found entity requiring replacement. Append any preceding
                 * characters into the buffer before the entity itself. */
                if (p > start) {
                    _bout.append(start, p - start);
                }
                start = p + 1;
                _bout.append(ep->replacement, ep->length);
                break;
            }
        }
    }
    if (p > start) {
        /* Append any left over characters into the buffer. */
        _bout.append(start, p - start);
    }
    _bout.append("\0", 1);
    return _bout.bytes();
}

int
EntityRef::size () {
    return _bout.size();
}


