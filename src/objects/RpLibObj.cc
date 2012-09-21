/*
 * ----------------------------------------------------------------------
 *  Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
#include <fstream>

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
    _status.addContext("Rappture::Library::__libInit");
    return;
}

void
Library::__libFree()
{
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

    //_objStorage.backup();
    _objStorage.clear();
    __parseTree2ObjectList(p);
    /*
    if (!_status) {
        _objStorage.save();
    } else {
        _objStorage.restore()
    }
    */

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

    // null terminate string holding file
    fileBuf.append("\0",1);

    loadXml(fileBuf.bytes());

    return _status;
}

Library &
Library::value (
    const char *key,
    void *storage,
    size_t numHints,
    ...)
{
    _status.addContext("Rappture::Library::value");
    Rappture::Object *o = _objStorage.find(key);

    if (o == NULL) {
        _status.addError("Error while retrieving object "
            "with key \"%s\": object does not exist", key);
        return *this;
    }

    va_list arg;

    va_start(arg,numHints);
    o->vvalue(storage,numHints,arg);
    _status = o->outcome();
    va_end(arg);

    return *this;
}

const char *
Library::xml() const
{
    _status.addContext("Rappture::Library::xml");
    Rp_ParserXml *p = Rp_ParserXmlCreate();
    Rp_ChainLink *l = NULL;

    const Rp_Chain *objList = _objStorage.contains();

    l = Rp_ChainFirstLink(objList);

    while (l != NULL) {
        Rappture::Object *o = (Rappture::Object *) Rp_ChainGetValue(l);
        if (o != NULL) {
            // FIXME: we can remove the if statement when
            // LibraryStorage is fixed to clean up holes
            // in the object chain.
            o->dump(RPCONFIG_TREE,(ClientData)p);
        }
        l = Rp_ChainNextLink(l);
    }

    return Rp_ParserXmlXml(p);
}

void
Library::__parseTree2ObjectList(Rp_ParserXml *p)
{
    _status.addContext("Rappture::Library::__parseTree2ObjectList");

    if (p == NULL) {
        _status.addError("parser is NULL");
        return;
    }

    Rp_Chain *children = Rp_ChainCreate();

    // parse out the tool information
    // Rp_ParserXmlChildren(p, "tool", NULL, children);
    // if there is a child called tool, grab its
    //      title                   char *
    //      about                   char *
    //      command                 char *
    //      limits.cputime          double
    //      layout                  char *
    //      control                 char *
    //      analyzer                char *
    //      reportJobFailures       int

    // get the children nodes of "input", and "output"
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
            _objStorage.store(obj->name(),obj);
            _objStorage.link(obj->name(),obj->path());
            // Rp_ChainAppend(retObjList,(void*)obj);

        // FIXME: add code to check for strings, choices,
        //        groups, curves, notes, images, molecules...
        //        all of the rappture object types
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

int
Library::error() const
{
    return (int) _status;
}

/*
Outcome &
Library::result(int status)
{
    Rappture::SimpleCharBuffer tmpBuf;
    std::fstream file;
    const char *xmlText = NULL;
    time_t t = 0;
    struct tm* timeinfo = NULL;
    timestamp = "";
    std::string username = "";
    std::string hostname = "";
    char *user = NULL;

    tmpBuf.appendf("run%i.xml",(int)time(&t));
    file.open(tmpBuf.bytes(),std::ios::out);

//    put("tool.version.rappture.revision",
//        "$LastChangedRevision: 1527 $");
//    put("tool.version.rappture.modified",
//        "$LastChangedDate: 2009-06-22 15:38:49 -0400"
//        " (Mon, 22 Jun 2009) $");
//    if ( "" == get("tool.version.rappture.language") ) {
//        put("tool.version.rappture.language","c++");
//    }

    // generate a timestamp for the run file
    timeinfo = localtime(&t);
    timestamp = std::string(ctime(&t));
    // erase the 24th character because it is a newline
    timestamp.erase(24);
    // concatinate the timezone
    timestamp.append(" ");
#ifdef _WIN32
    timestamp.append(_tzname[_daylight]);
    // username is left blank for windows because i dont know
    // how to retrieve username on win32 environment.
    username = "";
    hostname = "";
#else
    timestamp.append(timeinfo->tm_zone);
    user = getenv("USER");
    if (user != NULL) {
        username = std::string(user);
    } else {
        user = getenv("LOGNAME");
        if (user != NULL) {
            username = std::string(user);
        }
    }
#endif

    // add the timestamp to the run file
//    put("output.time", timestamp);
//    put("output.status",exitStatus);
//    put("output.user",username);
//    put("output.host",hostname);

    if ( file.is_open() ) {
        xmlText = xml();
        if (xmlText == NULL) {
        }
        file << xmlText;
        // check to make sure there were no
        // errors while writing the run.xml file.
        if (   (!file.good())
            || (strlen(xmlText) != ((long)file.tellp()-(long)1))
           ) {
             status.error("Error while writing run file");
             status.addContext("RpLibrary::result()");
        }
        file.close();
    }
    else {
        status.error("Error while opening run file");
        status.addContext("RpLibrary::result()");
    }
    std::printf("=RAPPTURE-RUN=>%s\n",outputFile.bytes());
}
*/

const Rp_Chain *
Library::contains() const
{
    return _objStorage.contains();
}

