/*
 * ======================================================================
 *  Rappture::Path
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpPath.h"

#include <fstream>
#include <assert.h>

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

#include "RpChainHelper.h"
#include <cstring>
#include <cstdlib>

using namespace Rappture;

/**
 * Construct an empty Path.
 */
Path::Path() :
    _ifs(0),
    _pathList(NULL)
{
    __pathInit();
}


/**
 * Construct a Path loaded with initial data.
 *
 * @param bytes pointer to bytes being stored.
 */
Path::Path(const char* bytes) :
    _ifs(0),
    _pathList(NULL)
{
    __pathInit();
    path(bytes);
}

/**
 * Copy constructor
 * @param Path object to copy
 */
Path::Path(const Path& b)
{
    __pathInit();

    _ifs = b._ifs;

    //FIXME: i think this is broken
    Rp_ChainCopy (_pathList, b._pathList, Rp_ChainCharCpyFxn);
}

/**
 * Assignment operator
 * @param Path object to copy
 */
/*
Path&
Path::operator=(const Path& b)
{
    b = bSimpleCharBuffer::operator=(b);
    return *this;
}


Path
Path::operator+(const Path& b) const
{
    Path newBuffer(*this);
    newBuffer.operator+=(b);
    return newBuffer;
}


Path&
Path::operator+=(const Path& b)
{
    SimpleCharBuffer::operator+=(b);
    return *this;
}
*/


Path::~Path()
{
    __pathFree();
}


void
Path::__pathInit()
{
    _ifs = '.';
    _pathList = Rp_ChainCreate();
    _currLink = Rp_ChainFirstLink(_pathList);
    return;
}

void
Path::__pathFree()
{
    if (_pathList) {
        Rp_ChainLink *l = NULL;
        componentStruct *compVal = NULL;
        l = Rp_ChainFirstLink(_pathList);
        while(l) {
            compVal = (componentStruct *) Rp_ChainGetValue(l);
            __deleteComponent(compVal);
            compVal = NULL;;
            l = Rp_ChainNextLink(l);
        }
        Rp_ChainDestroy(_pathList);
        _pathList = NULL;
    }
    b.clear();
    return;
}

Path::componentStruct *
Path::__createComponent(
    const char *p,
    int typeStart,
    int typeEnd,
    int idOpenParen,
    int idCloseParen,
    size_t degree)
{
    componentStruct *c = new componentStruct();
    int typeLen = -1;
    int idLen = -1;
    char *tmp = NULL;

    c->type = NULL;
    c->id = NULL;
    c->degree = degree;

    typeLen = typeEnd - typeStart;
    if (idOpenParen < idCloseParen) {
        // user specified an id
        idLen = idCloseParen - idOpenParen - 1;
    }

    /*
        typeLen and idLen are initialized to -1
        if the user specified a value, the length
        is set to the length of the specified value.
        if the user specified a blank string, the
        length is set to 0 and we still create a char array
        for the type or id field.
        we differentiate a blank entry from no entry so we
        can accept paths like this:
        input.group(tabs).().current
        input.group(tabs).()
        and correctly return the id,type,component, and parent
        versions of the path. these paths are corner cases that
        should never be used, but we handle them incase they are.
        if an id is not specified (no value, not even blank),
        the id pointer is set to NULL. in the above example paths
        the type and id of the third component are both blank.
        for the above example paths, the id field of the first
        components, the input component, is NULL.
        i don't think there is a way to get a NULL type right now.
    */


    if (typeLen >= 0) {
        tmp = new char[typeLen+1];
        strncpy(tmp,(p+typeStart),typeLen);
        tmp[typeLen] = '\0';
        c->type = tmp;
    }

    if (idLen >= 0) {
        tmp = new char[idLen+1];
        strncpy(tmp,(p+idOpenParen+1),idLen);
        tmp[idLen] = '\0';
        c->id = tmp;
    }

    return c;
}

void
Path::__deleteComponent(componentStruct *c)
{
    if (c) {
        if (c->type) {
            delete[] c->type;
        }
        if (c->id) {
            delete[] c->id;
        }
        delete c;
    }
}

Rp_Chain *
Path::__parse(const char *p)
{
    int typeStart = 0;
    int typeEnd = -1;       // typeEnd must be less than typeStart
                            // so we can check if there was a type
    int curr = 0;
    int idOpenParen = -1;
    int idCloseParen = -1;
    componentStruct *c = NULL;
    size_t degree = 1;
    char *newEnd = NULL;

    Rp_Chain *compList = Rp_ChainCreate();

    if (p == NULL) {
        return 0;
    }

    while (p[curr] != '\0') {
        if (p[curr] == '(') {
            idOpenParen = curr;
            typeEnd = curr;
        } else if (p[curr] == ')') {
            idCloseParen = curr;
        } else if ( (idOpenParen <= idCloseParen) &&
                    (curr != 0) &&
                    (p[curr] >= '0') &&
                    (p[curr] <= '9') ) {
            // we are not inside the parens
            // we are not at the start of the string
            // the value is a digit
            if (idOpenParen == idCloseParen) {
                // we are before any possible parens
                // and after the type
                typeEnd = curr;
            }
            degree = (size_t) strtod(p+curr, &newEnd);
            if (degree == 0) {
                // interpret degree of 0 same as degree of 1
                degree = 1;
            }
            // check if we consumed the _ifs
            if (*(newEnd-1) == _ifs) {
                newEnd -= 2;
            }
            curr += newEnd - (p + curr);
        } else if ( (p[curr] == _ifs) &&
                    ((idOpenParen == idCloseParen) ||
                     (idOpenParen < idCloseParen)) ) {
            // we see the _ifs
            // we are not inside of the parens for the id tag
            if (typeEnd < typeStart) {
                // we didn't come across the end
                // of the type string, so set it here.
                typeEnd = curr;
            }
            c = __createComponent(p,typeStart,typeEnd,idOpenParen,idCloseParen,degree);
            if (c != NULL) {
                Rp_ChainAppend(compList,c);
            }

            typeStart = curr+1;
            idOpenParen = -1;
            idCloseParen = -1;
            degree = 1;
        }
        curr++;
    }

    if (typeEnd < typeStart) {
        // < takes care of cases where there is no
        // id and no degree, but there is still a type
        typeEnd = curr;
    }
    c = __createComponent(p,typeStart,typeEnd,idOpenParen,idCloseParen,degree);
    if (c != NULL) {
        Rp_ChainAppend(compList,c);
    }

    return compList;
}

void
Path::__updateBuffer()
{
    Rp_ChainLink *l = NULL;
    size_t len = 0;

    b.clear();

    l = Rp_ChainFirstLink(_pathList);
    while (l != NULL) {

        if (b.nmemb() != 0) {
            b.append(&_ifs,1);
        }

        componentStruct *c = (componentStruct *) Rp_ChainGetValue(l);

        if (c->type != NULL) {
            len = strlen(c->type);
            if (len > 0) {
                b.append(c->type,len);
            }
        }

        if (c->degree > 1) {
            b.appendf("%i",c->degree);
        }

        if (c->id != NULL) {
            b.append("(",1);
            len = strlen(c->id);
            if (len > 0) {
                b.append(c->id,len);
            }
            b.append(")",1);
        }

        l = Rp_ChainNextLink(l);
    }

    b.append("\0",1);
}

const char *
Path::ifs (const char *el)
{
    // ifs cannot be NULL, \0, (, or )

    if ((el != NULL) &&
        (*el != '\0') &&
        (*el != '(') &&
        (*el != ')')) {
        _ifs = *el;
        __updateBuffer();
    }

    return (const char *)&_ifs;
}

Path&
Path::add (const char *el)
{
    Rp_Chain *addList = NULL;
    Rp_ChainLink *tmpLink = NULL;

    addList = __parse(el);
    tmpLink = Rp_ChainLastLink(addList);
    Rp_ChainInsertChainAfter(_pathList,addList,_currLink);
    _currLink = tmpLink;

    __updateBuffer();

    return *this;
}

Path&
Path::del ()
{
    Rp_ChainLink *l = NULL;

    // l = Rp_ChainLastLink(_pathList);

    if (_currLink != NULL) {
        l = _currLink;
    } else {
        l = Rp_ChainLastLink(_pathList);
    }

    if (l != NULL) {
        componentStruct *c = (componentStruct *) Rp_ChainGetValue(l);
        __deleteComponent(c);
        c = NULL;
        _currLink = Rp_ChainPrevLink(l);
        if (_currLink == NULL) {
            // don't go off the beginning of the list
            _currLink = Rp_ChainNextLink(l);
            // if _currLink is still NULL, it means the list is blank
        }
        Rp_ChainDeleteLink(_pathList,l);
    }

    __updateBuffer();

    return *this;
}

bool
Path::eof ()
{
    return (_currLink == NULL);
}

Path&
Path::first ()
{
    _currLink = Rp_ChainFirstLink(_pathList);
    return *this;
}

Path&
Path::prev ()
{
    if (_currLink) {
        _currLink = Rp_ChainPrevLink(_currLink);
    }
    return *this;
}

Path&
Path::next ()
{
    if (_currLink) {
        _currLink = Rp_ChainNextLink(_currLink);
    }
    return *this;
}

Path&
Path::last ()
{
    _currLink = Rp_ChainLastLink(_pathList);
    return *this;
}

Path&
Path::clear ()
{
    __pathFree();
    __pathInit();
    return *this;
}

size_t
Path::count ()
{
    return Rp_ChainGetLength(_pathList);
}

const char *
Path::component(void)
{
    Rp_ChainLink *l = NULL;

    if (_pathList == NULL) {
        return NULL;
    }

    l = _currLink;

    if (l == NULL) {
        return NULL;
    }

    componentStruct *c = (componentStruct *) Rp_ChainGetValue(l);

    if (c == NULL) {
        return NULL;
    }

    tmpBuf.clear();

    if (c->type != NULL) {
        tmpBuf.appendf("%s",c->type);
    }

    if (c->degree != 1) {
        tmpBuf.appendf("%zu",c->degree);
    }

    if (c->id != NULL) {
        tmpBuf.appendf("(%s)",c->id);
    }

    // incase none of the above if statements are hit.
    tmpBuf.append("\0",1);

    return tmpBuf.bytes();
}

// if you give more than a component,
// we only take the last component
// the whole component (type and id)
// are used to replace the current component.

void
Path::component(const char *p)
{
    if (p == NULL) {
        return;
    }

    Rp_Chain *addList = NULL;
    Rp_ChainLink *l = NULL;

    addList = __parse(p);

    if (addList == NULL) {
        // nothing to add
        return;
    }

    // get the last component of addList
    l = Rp_ChainLastLink(addList);

    if (l == NULL) {
        // nothing to add
        Rp_ChainDestroy(addList);
        return;
    }

    componentStruct *aLcomp = (componentStruct *) Rp_ChainGetValue(l);
    Rp_ChainDeleteLink(_pathList,l);

    // delete addList
    l = Rp_ChainFirstLink(addList);
    while (l != NULL) {
        componentStruct *c = (componentStruct *) Rp_ChainGetValue(l);
        delete c;
        l = Rp_ChainNextLink(l);
    }
    Rp_ChainDestroy(addList);

    // swap the current component from the current path
    // with the component from the provided path.
    l = _currLink;

    if (l != NULL) {
        componentStruct *last = (componentStruct *) Rp_ChainGetValue(l);
        delete last;
        Rp_ChainSetValue(l,aLcomp);
    } else {
        // add a new component as a link
        // point _currLink to the new component
        _currLink = Rp_ChainAppend(_pathList,aLcomp);
    }

    __updateBuffer();

    return;
}

const char *
Path::id()
{
    Rp_ChainLink *l = NULL;

    if (_pathList == NULL) {
        return NULL;
    }

    l = _currLink;

    if (l == NULL) {
        return NULL;
    }

    componentStruct *c = (componentStruct *) Rp_ChainGetValue(l);

    if (c == NULL) {
        return NULL;
    }

    return c->id;
}

void
Path::id(const char *p)
{
    if (p == NULL) {
        return;
    }

    Rp_ChainLink *l = NULL;
    char *tmp = NULL;
    size_t len = strlen(p);
    componentStruct *last = NULL;

    // update the current component from the current path
    l = _currLink;

    if (l != NULL) {
        last = (componentStruct *) Rp_ChainGetValue(l);
        delete[] last->id;
    } else {
        // append the new component onto the current path
        last = new componentStruct;
        _currLink = Rp_ChainAppend(_pathList,last);
        tmp = new char[1];
        *tmp = '\0';
        last->type = tmp;
    }

    // adjust the id field
    tmp = new char[len+1];
    strncpy(tmp,p,len+1);
    last->id = tmp;

    __updateBuffer();

    return;
}

const char *
Path::type()
{
    Rp_ChainLink *l = NULL;

    if (_pathList == NULL) {
        return NULL;
    }

    l = _currLink;

    if (l == NULL) {
        return NULL;
    }

    componentStruct *c = (componentStruct *) Rp_ChainGetValue(l);

    if (c == NULL) {
        return NULL;
    }

    return c->type;
}

void
Path::type(const char *p)
{
    if (p == NULL) {
        return;
    }

    Rp_ChainLink *l = NULL;
    char *tmp = NULL;
    size_t len = strlen(p);
    componentStruct *last = NULL;

    // update the last component from the current path
    l = _currLink;

    if (l != NULL) {
        last = (componentStruct *) Rp_ChainGetValue(l);
        delete[] last->type;
    } else {
        // append the new component onto the current path
        last = new componentStruct;
        _currLink = Rp_ChainAppend(_pathList,last);
        last->id = NULL;
    }

    // adjust the type field
    tmp = new char[len+1];
    strncpy(tmp,p,len+1);
    last->type = tmp;

    __updateBuffer();

    return;
}

const char *
Path::parent()
{
    Rp_ChainLink *l = NULL;
    Rp_ChainLink *last = NULL;

    tmpBuf.clear();

    last = _currLink;
    l = Rp_ChainFirstLink(_pathList);
    while (l != last) {

        if (tmpBuf.nmemb() != 0) {
            tmpBuf.append(&_ifs,1);
        }

        componentStruct *c = (componentStruct *) Rp_ChainGetValue(l);

        if (c->type) {
            tmpBuf.append(c->type);
        }

        if (c->id) {
            tmpBuf.append("(",1);
            tmpBuf.append(c->id);
            tmpBuf.append(")",1);
        }

        // what happens if type and id are both null?

        l = Rp_ChainNextLink(l);
    }

    tmpBuf.append("\0",1);

    return tmpBuf.bytes();
}

void
Path::parent(const char *p)
{
    Rp_Chain *addList = NULL;
    Rp_ChainLink *tmpLink = NULL;

    addList = __parse(p);
    // maybe make this an input flag, FirstLink (default) or LastLink
    tmpLink = Rp_ChainFirstLink(addList);
    Rp_ChainInsertChainBefore(_pathList,addList,_currLink);
    _currLink = tmpLink;

    __updateBuffer();

    return;
}

size_t
Path::degree()
{
    Rp_ChainLink *l = NULL;

    l = _currLink;

    if (l == NULL) {
        return 0;
    }

    componentStruct *c = (componentStruct *) Rp_ChainGetValue(l);

    if (c == NULL) {
        return 0;
    }

    return c->degree;
}

void
Path::degree(size_t d)
{
    if (d == 0) {
        d = 1;
    }

    Rp_ChainLink *l = NULL;
    componentStruct *last = NULL;

    // update the current component from the current path
    l = _currLink;

    if (l != NULL) {
        last = (componentStruct *) Rp_ChainGetValue(l);
    } else {
        // append the new component onto the current path
        last = new componentStruct;
        _currLink = Rp_ChainAppend(_pathList,last);
    }

    // adjust the degree field
    last->degree = d;

    __updateBuffer();

    return;
}

const char *
Path::path(void)
{
    return b.bytes();
}

void
Path::path(const char *p)
{
    if (p != NULL) {
        _pathList = __parse(p);
        _currLink = Rp_ChainLastLink(_pathList);
        __updateBuffer();
    }
    return;
}

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

