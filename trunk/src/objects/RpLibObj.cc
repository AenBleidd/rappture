/*
 * ----------------------------------------------------------------------
 *  Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "expat.h"
#include "RpParserXML.h"
#include "RpLibObj.h"
#include "RpBuffer.h"
#include "RpEntityRef.h"
#include "RpEncode.h"
#include "RpTree.h"

#include "RpNumber.h"

#include <stdlib.h>
#include <time.h>
#include <cctype>

using namespace Rappture;

Library::Library ()
{
    __libInit();
}

Library::~Library ()
{
    __libFree();
}

void
Library::__libInit()
{
    _objList = Rp_ChainCreate();
    return;
}

void
Library::__libFree()
{
    if (_objList != NULL) {
        Rp_ChainLink *l = NULL;
        Rappture::Object *objVal = NULL;
        l = Rp_ChainFirstLink(_objList);
        while(l) {
            objVal = (Rappture::Object *) Rp_ChainGetValue(l);
            delete objVal;
            objVal = NULL;;
            l = Rp_ChainNextLink(l);
        }
        Rp_ChainDestroy(_objList);
        _objList = NULL;
    }
    return;
}

Outcome &
Library::loadXml (const char *xmltext)
{
    Rp_ParserXml *p = NULL;

    _status.addContext("Rappture::Library::loadXml");

    if (xmltext == NULL) {
        _status.addError("xmltext was NULL");
        return _status;
    }

    // parse xml file
    p = Rp_ParserXmlCreate();
    if (p == NULL) {
        _status.addError("error while creating xml parser");
        return _status;
    }
    Rp_ParserXmlParse(p, xmltext);
    // FIXME: add error check for Rp_ParserXmlParse

    // convert xml tree into chain of rappture objects
    Rp_Chain *tmpObjList = Rp_ChainCreate();
    __parseTree2ObjectList(p,tmpObjList);
    if (!_status) {
        __libFree();
        _objList = tmpObjList;
    }

    return _status;
}

Outcome &
Library::loadFile (const char *filename)
{
    _status.addContext("Rappture::Library::loadFile");

    // load xml file
    Rappture::Buffer fileBuf;
    if (!fileBuf.load(_status, filename)) {
        return _status;
    }

    loadXml(fileBuf.bytes());

    return _status;
}

const char *
Library::xml() const
{
    Rp_ParserXml *p = Rp_ParserXmlCreate();
    Rp_ChainLink *l = NULL;

    l = Rp_ChainFirstLink(_objList);

    while (l != NULL) {
        Rappture::Object *o = (Rappture::Object *) Rp_ChainGetValue(l);
        o->dump(RPCONFIG_TREE,(ClientData)p);
        l = Rp_ChainNextLink(l);
    }

    return Rp_ParserXmlXml(p);
}

void
Library::__parseTree2ObjectList(
    Rp_ParserXml *p,
    Rp_Chain *retObjList)
{
    _status.addContext("Rappture::Library::__parseTree2ObjectList");

    if (p == NULL) {
        _status.addError("parser is NULL");
        return;
    }

    if (retObjList == NULL) {
        _status.addError("return object list is NULL");
        return;
    }

    // get the children nodes of "tool", "input", and "output"
    Rp_Chain *children = Rp_ChainCreate();
    // Rp_ParserXmlChildren(p, "tool", NULL, children);
    Rp_ParserXmlChildren(p, "input", NULL, children);
    Rp_ParserXmlChildren(p, "output", NULL, children);

    Rp_ChainLink *l = Rp_ChainFirstLink(children);
    while (l != NULL) {
        Rp_TreeNode child = (Rp_TreeNode) Rp_ChainGetValue(l);
        const char *label = Rp_TreeNodeLabel(child);

        Rp_ParserXmlBaseNode(p,child);

        // find the correct object to create
        // Rappture::Object *nobj = NULL;
        if (('n' == *label) && (strcmp(label,"number") == 0)) {
            Rappture::Number *obj = new Rappture::Number();
            obj->configure(RPCONFIG_TREE,(void*)p);
            Rp_ChainAppend(retObjList,(void*)obj);
        } else {
            _status.addError("unrecognized object type: %s",label);
        }

        l = Rp_ChainNextLink(l);
    }

    // FIXME: should return the base node to the previous base
    Rp_ParserXmlBaseNode(p,NULL);

    return;
}

Outcome &
Library::outcome() const
{
    return _status;
}

const Rp_Chain *
Library::contains() const
{
    return _objList;
}
