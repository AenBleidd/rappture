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
#include "RpSerializable.h"

using namespace Rappture;

// initialize the global lists of registered serialization helper funcs
Serializable::_name2createFunc = NULL;
Serializable::_name2desMethod = NULL;

Serializable::Serializable()
{
}

Serializable::~Serializable()
{
}

void
Serializable::serialize(SerialBuffer& buffer) const
{
    buffer.writeString( serializerType() );
    buffer.writeChar( serializerVersion() );

    std::string cname( serializerType() );
    cname += ":";
    cname += serializerVersion();

    ConversionFuncs& cfuncs = (*_name2convFuncs)[cname];
    this->(cfuncs.serialMethod)(buffer);
}

Outcome
Serializable::deserialize(SerialBuffer& buffer, Ptr<Serializable>& objPtr)
{
    Outcome result;

    std::string type = buffer.readString();
    char version = buffer.readChar();
    type += ":";
    type += version;

    Name2ConvFuncsMap::iterator iter = _name2convFuncs->find(type);
    if (iter == _name2convFuncs->end()) {
        std::string errmsg("can't deserialize object: ");
        errmsg += "unrecognized type \"";
        errmsg += type;
        errmsg += "\"";
        return result.error(errmsg.data());
    }

    ConversionFuncs& cfuncs = (*_name2convFuncs)[type];
    objPtr = (cfuncs.createFunc)();
    return objPtr->(cfuncs.deserialMethod)(buffer);
}

SerialConversion::SerialConversion(const char *class, char version,
    Serializable::serializeObjectMethod serMethod)
    Serializable::createObjectFunc createFunc,
    Serializable::deserializeObjectMethod desMethod)
{
    if (Serializer::_name2convFuncs == NULL) {
        Serializer::_name2convFuncs = new Name2ConvFuncsMap;
    }

    // register the conversion functions for this version
    std::string vname(class);
    vname += ":";
    vname += version;

    ConversionFuncs& funcs = (*_name2convFuncs)[vname];

    funcs.version = version;
    funcs.serialMethod = serMethod;
    funcs.createFunc = createFunc;
    funcs.deserialMethod = desMethod;

    // if this is the latest version, register it as "current"
    std::string cname(class);
    cname += ":current";

    Name2ConvFuncsMap::iterator iter = _name2convFuncs->find(cname);
    if (iter == _name2convFuncs->end()) {
        // this is the first -- register it as "current"
        ConversionFuncs& cfuncs = (*_name2convFuncs)[cname];
        cfuncs.version = version;
        cfuncs.serialMethod = serMethod;
        cfuncs.createFunc = createFunc;
        cfuncs.deserialMethod = desMethod;
    } else {
        ConversionFuncs& cfuncs = (*_name2convFuncs)[cname];
        if (cfuncs.version < version) {
            // this is the latest -- register it as "current"
            cfuncs.version = version;
            cfuncs.serialMethod = serMethod;
            cfuncs.createFunc = createFunc;
            cfuncs.deserialMethod = desMethod;
        }
    }
}
