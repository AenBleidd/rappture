/*
 * ----------------------------------------------------------------------
 *  Rappture::Serializer
 *    The object collects a series of other objects together and
 *    serializes them into a single byte stream.  That stream can
 *    be written to a file, send over a socket, and so forth.  The
 *    resulting stream can be loaded by a Serializer and the objects
 *    can be reconstituted to their original form.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <assert.h>
#include <sstream>
#include "RpSerializer.h"

using namespace Rappture;

Serializer::Serializer()
{
}

Serializer::~Serializer()
{
}

Outcome
Serializer::deserialize(const char* bytes, int nbytes)
{
    Outcome result;
    std::string token;

    clear();
    SerialBuffer buffer(bytes, nbytes);

    // read the format and make sure we've got the right reader
    token = buffer.readString();
    if (token != "RpSerial:A") {
        std::string errmsg("can't deserialize stream: ");
        errmsg += "bad marker \"";
        errmsg += token;
        errmsg += "\" (should be RpSerial:A)";
        return result.error(errmsg.data());
    }

    // get the number of objects to read
    int nobj = buffer.readInt();
    if (nobj < 0) {
        std::string errmsg("can't deserialize stream: ");
        errmsg += "corrupt data (number of objects is negative)";
        return result.error(errmsg.data());
    }

    // read out the expected number of objects from the list
    for (int i=0; i < nobj && !buffer.atEnd(); i++) {
        token = buffer.readString();
        if (token != "RpObj:") {
            std::string errmsg("can't deserialize stream: ");
            errmsg += "bad marker \"";
            errmsg += token;
            errmsg += "\" (should be RpObj:)";
            return result.error(errmsg.data());
        }
        std::string id = buffer.readString();

        Ptr<Serializable> objPtr;
        result = Serializable::deserialize(buffer, &objPtr);

        // if there was an error, then bail out
        if (result != 0) {
            std::ostringstream context;
            context << "while deserializing object #" << i+1
              << " of " << nobj;
            return result.addContext(context.str().data());
        }

        // add the object to the known list
        _id2obj[id] = objPtr;
        SerializerId2Obj::iterator iter = _id2obj.find(id);
        const char* key = (*iter).first.data();
        _idlist.push_back(key);
    }

    return result;
}

Ptr<SerialBuffer>
Serializer::serialize()
{
    Ptr<SerialBuffer> bufferPtr( new SerialBuffer() );
    bufferPtr->writeString("RpSerial:A");

    // write out number of objects in the stream
    bufferPtr->writeInt(_idlist.size());

    // write out each object in the list, in order
    for (unsigned int i=0; i < _idlist.size(); i++) {
        const char* id = _idlist.at(i);
        Ptr<Serializable> objPtr = _id2obj[id];

        bufferPtr->writeString("RpObj:");
        bufferPtr->writeString(id);
        objPtr->serialize(*bufferPtr.pointer());
    }
    return bufferPtr;
}

int
Serializer::size() const
{
    return _idlist.size();
}

Ptr<Serializable>
Serializer::get(int pos) const
{
    Serializer *nonconst = (Serializer*)this;
    assert(pos >= 0 && (unsigned int)pos < _idlist.size());

    std::string id(_idlist[pos]);
    return nonconst->_id2obj[id];
}

Serializer&
Serializer::clear()
{
    _id2obj.clear();
    _idlist.clear();
    return *this;
}

const char*
Serializer::add(Serializable* objPtr)
{
    const char* key = NULL;

    // build a unique string that represents this object
    std::ostringstream idbuffer;
    idbuffer << (void*)objPtr;
    std::string id(idbuffer.str());

    // have we already registered this object?
    SerializerId2Obj::iterator iter = _id2obj.find(id);
    if (iter == _id2obj.end()) {
        // if not, then add to map and also to list of all objects
        _id2obj[id] = Ptr<Serializable>(objPtr);
        iter = _id2obj.find(id);
        key = (*iter).first.data();
        _idlist.push_back(key);
    } else {
        // if so, then update map
        key = (*iter).first.data();
        _id2obj[id] = Ptr<Serializable>(objPtr);
    }
    return key;
}
