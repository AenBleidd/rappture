/*
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
 *        &nbsp;         " "
 *
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *  Also see below text for additional information on usage and redistribution
 *
 * ======================================================================
 */

#include "RpEntityRef.h"
#include <cctype>
#include <cstring>

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

using namespace Rappture;

const char*
EntityRef::decode (
    const char* value,
    unsigned int len
)
{
    unsigned int pos = 0;

    if (value == NULL) {
        // empty string, noting to do
        return NULL;
    }

    _bout.clear();
    if (len == 0) {
        len = strlen(value);
    }

    while (pos < len) {
        if (value[pos] == '&') {
            pos++;
            if ((pos < len)) {
                if (value[pos] && isalpha(value[pos])) {
                    if (        (value[pos] == 'q')
                            &&  (strncmp("quot;",value+pos,5) == 0) ) {
                        _bout.append("\"");
                        pos += 5;
                    }
                    else if (   (value[pos] == 'a')
                            &&  (strncmp("amp;",value+pos,4) == 0) ) {
                        _bout.append("&");
                        pos += 4;
                    }
                    else if (   (value[pos] == 'l')
                            &&  (strncmp("lt;",value+pos,3) == 0) ) {
                        _bout.append("<");
                        pos += 3;
                    }
                    else if (   (value[pos] == 'g')
                            &&  (strncmp("gt;",value+pos,3) == 0) ) {
                        _bout.append(">");
                        pos += 3;
                    }
                    else {
                        // unrecognized
                        _bout.append(value+pos,1);
                        pos++;
                    }
                }
                else {
                    _bout.append(value+pos,1);
                    pos++;
                }
            }
            else {
                // last character was really an ampersand
                // add it to the buffer
                pos--;
                _bout.append(value+pos,1);
                pos++;
            }
        }
        else
        {
            // non entity ref character
            _bout.append(value+pos,1);
            pos++;
        }
    }

    _bout.append("\0",1);
    return _bout.bytes();
}

const char*
EntityRef::encode (
    const char* value,
    unsigned int len
)
{
    unsigned int pos = 0;


    if (value == NULL) {
        // empty string, noting to do
        return NULL;
    }

    _bout.clear();
    if (len == 0) {
        len = strlen(value);
    }

    while (pos < len) {
        if (*(value+pos) == '"') {
            _bout.append("&quot;");
        }
        else if (*(value+pos) == '&') {
            _bout.append("&amp;");
        }
        else if (*(value+pos) == '<') {
            _bout.append("&lt;");
        }
        else if (*(value+pos) == '>') {
            _bout.append("&gt;");
        }
        /*
        else if (*(value+pos) == '\n') {
            _bout.append("&#xA;");
        }
        */
        else
        {
            _bout.append(value+pos,1);
        }
        pos++;
    }

    _bout.append("\0",1);
    return _bout.bytes();
}

int
EntityRef::size () {
    return _bout.size();
}


#ifdef __cplusplus
    } // extern c
#endif // ifdef __cplusplus
