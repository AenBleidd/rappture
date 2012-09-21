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
#ifndef RPSERIALIZER_H
#define RPSERIALIZER_H

#include <map>
#include <RpPtr.h>
#include <RpOutcome.h>
#include "RpSerialBuffer.h"
#include "RpSerializable.h"

namespace Rappture {

class Serializer {
public:
    Serializer();
    virtual ~Serializer();

    virtual Ptr<SerialBuffer> serialize();
    virtual Outcome deserialize(const char* bytes, int nbytes);

    virtual int size() const;
    virtual Ptr<Serializable> get(int pos) const;

    virtual Serializer& clear();
    virtual const char* add(Serializable* objPtr);

private:
    // disallow these operations
    Serializer(const Serializer& status) { assert(0); }
    Serializer& operator=(const Serializer& status) { assert(0); }

    // list of objects being serialized or deserialized
    std::vector<const char*> _idlist;

    // map each id in _idlist to the corresponding object
    typedef std::map<std::string, Ptr<Serializable> > SerializerId2Obj;
    SerializerId2Obj _id2obj;
};

} // namespace Rappture

#endif
