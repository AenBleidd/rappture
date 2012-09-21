/*
 * ======================================================================
 *  Rappture::Serializable
 *
 *  AUTHOR:  Michael McLennan, Purdue University
 *           Carol X Song, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RPSERIALIZABLE_H
#define RPSERIALIZABLE_H

#include <string>
#include <map>
#include <RpOutcome.h>
#include <RpPtr.h>
#include <RpSerialBuffer.h>

namespace Rappture {

class SerialConversion;  // see below

/**
 * Base class for any object that can be serialized into a stream,
 * then saved to a file or written to a socket, and reconstituted
 * into its original form.  Serializable objects can be added to
 * a serializer, which handles the overall conversion.
 */
class Serializable {
public:
    Serializable();
    virtual ~Serializable();

    virtual const char* serializerType() const = 0;
    virtual char serializerVersion() const = 0;

    virtual void serialize(SerialBuffer& bufferPtr) const;
    static Outcome deserialize(SerialBuffer& buffer, Ptr<Serializable>* objPtrPtr);

    typedef void (Serializable::*serializeObjectMethod)(SerialBuffer& buffer) const;
    typedef Ptr<Serializable> (*createObjectFunc)();
    typedef Outcome (Serializable::*deserializeObjectMethod)(SerialBuffer& buffer);

private:
    friend class SerialConversion;

    class ConversionFuncs {
    public:
        char version;
        serializeObjectMethod serialMethod;
        createObjectFunc createFunc;
        deserializeObjectMethod deserialMethod;
    };

    typedef std::map<std::string, ConversionFuncs> Name2ConvFuncsMap;
    static Name2ConvFuncsMap *_name2convFuncs;
};

/**
 * Each class derived from Serializable should have a SerialConversion
 * stored as a static data member.  This declares information needed
 * to serialize/deserialize the class, making objects in that class
 * serializable.
 */
class SerialConversion {
public:
    SerialConversion(const char *className, char version,
        Serializable::serializeObjectMethod,
        Serializable::createObjectFunc,
        Serializable::deserializeObjectMethod);
};

} // namespace Rappture

#endif /*RPSERIALIZABLE_H*/
