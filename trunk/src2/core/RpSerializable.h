/*
 * ----------------------------------------------------------------------
 *  Rappture::Serializable
 *    Base class for any object that can be serialized into a stream,
 *    then saved to a file or written to a socket, and reconstituted
 *    into its original form.  Serializable objects can be added to
 *    a serializer, which handles the overall conversion.
 *
 * ======================================================================
 *  AUTHOR:  Carol X Song, Purdue University
 *           Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RAPPTURE_SERIALIZABLE_H
#define RAPPTURE_SERIALIZABLE_H

#include <string>
#include <map>
#include "RpSerialBuffer.h"

namespace Rappture {

class SerialConversion;  // see below

class Serializable {
public:
    Serializable();
    virtual ~Serializable();

    virtual const char* serializerType() = 0;
    virtual char serializerVersion() = 0;

    virtual void serialize(SerialBuffer& bufferPtr) const;
    static Outcome deserialize(SerialBuffer& buffer, Ptr<Serializable>& objPtr);

private:
    friend class SerialConversion;

    typedef void serializeObjectMethod(SerialBuffer& buffer) const;
    typedef static Ptr<Serializable> createObjectFunc();
    typedef Outcome deserializeObjectMethod(SerialBuffer& buffer);

    struct ConversionFuncs {
        char version;
        serializeObjectMethod serialMethod;
        createObjectFunc createFunc;
        deserializeObjectMethod deserialMethod;
    };

    typedef map<std::string, ConversionFuncs> Name2ConvFuncsMap;
    static Name2ConvFuncsMap *_name2convFuncs;
};

class SerialConversion {
public:
    SerialConversion(const char *class, char version,
        Serializable::serializeObjectMethod,
        Serializable::createObjectFunc,
        Serializable::deserializeObjectMethod);
};

} // namespace Rappture

#endif
