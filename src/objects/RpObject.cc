/*
 * ----------------------------------------------------------------------
 *  RpObject.cc
 *
 *   Rappture 2.0 Object member functions
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpObject.h"
#include "RpHashHelper.h"
#include <cstring>

using namespace Rappture;

Object::Object()
{
    __init();
}

Object::Object (
    const char *name,
    const char *path,
    const char *label,
    const char *desc,
    const char *hints,
    const char *color   )
{
    this->name(name);
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->hints(hints);
    this->color(color);
}

Object::Object(const Object& o)
{
    name(o.name());
    path(o.path());
    label(o.label());
    desc(o.desc());
    hints(o.hints());
    color(o.color());
    // icon(o.icon());

    if (o._h != NULL) {
        Rp_HashCopy(_h,o._h,charCpyFxn);
    }
}

Object::~Object()
{
    __clear();
}

const char*
Object::name() const
{
    return propstr("name");
}

void
Object::name(const char *p)
{
    propstr("name",p);
}

const char*
Object::path() const
{
    return propstr("path");
}

void
Object::path(const char *p)
{
    propstr("path",p);
}

const char*
Object::label() const
{
    return propstr("label");
}

void
Object::label(
    const char *p)
{
    propstr("label",p);
}

const char*
Object::desc() const
{
    return propstr("desc");
}

void
Object::desc(const char *p)
{
    propstr("desc",p);
}

const char*
Object::hints() const
{
    return propstr("hints");
}

void
Object::hints(const char *p)
{
    propstr("hints",p);
}

const char*
Object::color() const
{
    return propstr("color");
}

void
Object::color(const char *p)
{
    propstr("color",p);
}

// get a general property
const void *
Object::property(const char *key) const
{
    if (key == NULL) {
        // no key to search for
        return NULL;
    }

    if (_h == NULL) {
        // hash table does not exist, value is not in table
        return NULL;
    }

    // get the value
    return Rp_HashSearchNode(_h,key);
}

// set a general property
void
Object::property(const char *key, const void *val, size_t nbytes)
{
    if (key == NULL) {
        // no key for value
        return;
    }

    if (_h == NULL) {
        // hash table does not exist, create it
        _h = (Rp_HashTable*) malloc(sizeof(Rp_HashTable));
        Rp_InitHashTable(_h,RP_STRING_KEYS);
    }

    // FIXME: this is suspect,
    // want to use void's and void*'s but c++ wont let me
    // g++ says there is no delete[] for void*
    void *tmp = new char[nbytes];
    memcpy(tmp,val,nbytes);

    // set the value
    char *old = (char *) Rp_HashRemoveNode(_h,key);
    delete[] old;
    old = NULL;
    Rp_HashAddNode(_h,key,tmp);

    return;
}

// get a const char* property
const char *
Object::propstr(const char *key) const
{
    if (key == NULL) {
        // no key for value
        return NULL;
    }

    if (_h == NULL) {
        // hash table does not exist, value does not exist in table
        return NULL;
    }

    // get the value
    return (const char *) Rp_HashSearchNode(_h,key);
}

// set a const char* property
void
Object::propstr(const char *key, const char *val)
{
    char *str = NULL;

    if (key == NULL) {
        // no key for value
        return;
    }

    if (_h == NULL) {
        // hash table does not exist, create it
        _h = (Rp_HashTable*) malloc(sizeof(Rp_HashTable));
        Rp_InitHashTable(_h,RP_STRING_KEYS);
    }

    if (val == NULL) {
        // no value provided, no value stored.
        // FIXME: raise error.
        return;
    }

    // set the value
    // FIXME: this is suspect,
    // want to use void's and void*'s but c++ wont let me
    // g++ says there is no delete[] for void*
    // FIXME: can probably just get the length and use property() fxn
    size_t l = strlen(val);
    str = new char[l+1];
    strcpy(str,val);

    char *old = (char *) Rp_HashRemoveNode(_h,key);
    delete[] old;
    old = NULL;
    Rp_HashAddNode(_h,key,str);

    return;
}

// remove a property from the hash table
void
Object::propremove(const char *key)
{
    if ((key == NULL) || (_h == NULL)) {
        // key or hash table does not exist
        return;
    }

    char *tmp = (char *) Rp_HashRemoveNode(_h,key);
    delete[] tmp;
    tmp = NULL;
    return;
}

void
Object::__hintParser(
    char *hint,
    const char **hintKey,
    const char **hintVal) const
{

    // hints are null terminated strings with a hint key
    // and hint value separated by an equal sign.
    // they take the following form:
    // hintKey=hintVal
    // this function does change the original hint string.

    char *v = NULL;

    if (hint == NULL) {
        return;
    }

    v = strchr(hint,'=');
    *hintKey = hint;
    if ((v == NULL) || (*v == '\0') || (*(v+1) == '\0')) {
        // incomplete hint string
        *hintVal = NULL;
    } else {
        *v = '\0';
        *hintVal = v+1;
    }

    return;
}

void
Object::vvalue(void *storage, size_t numHints, va_list arg) const
{
    // bland objects take no hints
    char buf[1024];
    char *hintCopy = NULL;
    size_t hintLen = 0;

    char *hint = NULL;
    const char *hintKey = NULL;
    const char *hintVal = NULL;

    while (numHints > 0) {
        numHints--;
        hint = va_arg(arg, char *);
        hintLen = strlen(hint);
        if (hintLen < 1024) {
            hintCopy = buf;
        } else {
            hintCopy = new char[hintLen+1];
        }
        strcpy(hintCopy,hint);
        __hintParser(hintCopy,&hintKey,&hintVal);
        if (hintCopy != buf) {
            delete hintCopy;
        }
    }
    return;
}

void
Object::random()
{
    // bland objects cannot make random values
    return;
}

Rp_Chain *
Object::diff(const Object &o)
{
    return NULL;
}

void
Object::__init()
{
    _h = NULL;
    label("");
    desc("");
    hints("");
    color("");
    // icon(NULL);
    path("");

    _tmpBuf.clear();

    _h = (Rp_HashTable*) malloc(sizeof(Rp_HashTable));
    Rp_InitHashTable(_h,RP_STRING_KEYS);
}

void
Object::__clear()
{
    _tmpBuf.clear();

    // traverse the hash table to delete values, then delete hash table
    if (_h != NULL) {
        Rp_HashEntry *hEntry = NULL;
        Rp_HashSearch hSearch;

        hEntry = Rp_FirstHashEntry(_h,&hSearch);
        while (hEntry != NULL) {
            // FIXME: this is suspect,
            // want to use void's and void*'s but c++ wont let me
            // g++ says there is no delete[] for void*
            char *v = (char *) Rp_GetHashValue(hEntry);
            delete[] v;
            v = NULL;
            hEntry = Rp_NextHashEntry(&hSearch);
        }

        Rp_DeleteHashTable(_h);
        _h = NULL;
    }
}

/*
const char *
Object::xml(size_t indent, size_t tabstop) const
{
    return NULL;
}
*/

/*
void
Object::xml(const char *xmltext)
{
    return;
}
*/

/**********************************************************************/
// METHOD: configure(ClientData c)
/// construct an object from the provided tree
/**
 * construct an object from the provided tree
 */

void
Object::configure(size_t as, ClientData c)
{
    if (as == RPCONFIG_XML) {
        __configureFromXml(c);
    } else if (as == RPCONFIG_TREE) {
        __configureFromTree(c);
    }
}

/**********************************************************************/
// METHOD: configureFromXml(const char *xmltext)
/// configure the object based on Rappture1.1 xmltext
/**
 * Configure the object based on the provided xml
 */

void
Object::__configureFromXml(ClientData c)
{
    const char *xmltext = (const char *)c;
    if (xmltext == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_ParserXml *p = Rp_ParserXmlCreate();
    Rp_ParserXmlParse(p, xmltext);
    __configureFromTree(p);

    return;
}

void
Object::__configureFromTree(ClientData c)
{
    Rp_ParserXml *p = (Rp_ParserXml *)c;
    if (p == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_TreeNode node = Rp_ParserXmlElement(p,NULL);

    Rappture::Path pathObj(Rp_ParserXmlNodePath(p,node));

    path(pathObj.parent());
    name(Rp_ParserXmlNodeId(p,node));

    pathObj.clear();
    pathObj.add("about");
    pathObj.add("label");
    label(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.type("description");
    desc(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.type("hints");
    hints(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.type("color");
    color(Rp_ParserXmlGet(p,pathObj.path()));

    return;
}

/**********************************************************************/
// METHOD: dump(size_t as, void *p)
/// construct a number object from the provided tree
/**
 * construct a number object from the provided tree
 */

void
Object::dump(size_t as, ClientData p)
{
    if (as == RPCONFIG_XML) {
        __dumpToXml(p);
    } else if (as == RPCONFIG_TREE) {
        __dumpToTree(p);
    }
}

/**********************************************************************/
// METHOD: dumpToXml(ClientData p)
/// configure the object based on Rappture1.1 xmltext
/**
 * Configure the object based on the provided xml
 */

void
Object::__dumpToXml(ClientData c)
{
    if (c == NULL) {
        // FIXME: setup error
        return;
    }

    ClientDataXml *d = (ClientDataXml *)c;
    Rp_ParserXml *parser = Rp_ParserXmlCreate();
    __dumpToTree(parser);
    _tmpBuf.appendf("%s",Rp_ParserXmlXml(parser));
    d->retStr = _tmpBuf.bytes();
    Rp_ParserXmlDestroy(&parser);
}

/**********************************************************************/
// METHOD: dumpToTree(ClientData p)
/// dump the object to a Rappture1.1 based tree
/**
 * Dump the object to a Rappture1.1 based tree
 */

void
Object::__dumpToTree(ClientData c)
{
    if (c == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_ParserXml *parser = (Rp_ParserXml *)c;

    Path p;

    p.parent(path());
    p.last();

    p.add("object");
    p.id(name());

    p.add("about");

    p.add("label");
    Rp_ParserXmlPutF(parser,p.path(),"%s",label());

    p.type("description");
    Rp_ParserXmlPutF(parser,p.path(),"%s",desc());

    p.type("hints");
    Rp_ParserXmlPutF(parser,p.path(),"%s", hints());

    p.type("color");
    Rp_ParserXmlPutF(parser,p.path(),"%s", color());

    return;
}

Outcome &
Object::outcome() const
{
    return _status;
}

const int
Object::is() const
{
    // return "var" in hex
    return 0x766172;
}

// -------------------------------------------------------------------- //

