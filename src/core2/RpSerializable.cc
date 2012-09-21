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
#include "RpSerializable.h"

using namespace Rappture;

// initialize the global lists of registered serialization helper funcs
Serializable::Name2ConvFuncsMap* Serializable::_name2convFuncs = NULL;

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

    Serializable::ConversionFuncs& cfuncs = (*_name2convFuncs)[cname];
    (this->*(cfuncs.serialMethod))(buffer);
}

Outcome
Serializable::deserialize(SerialBuffer& buffer, Ptr<Serializable>* objPtrPtr)
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
    *objPtrPtr = (cfuncs.createFunc)();

    Serializable *objPtr = objPtrPtr->pointer();
    return (objPtr->*(cfuncs.deserialMethod))(buffer);
}

SerialConversion::SerialConversion(const char *className, char version,
    Serializable::serializeObjectMethod serMethod,
    Serializable::createObjectFunc createFunc,
    Serializable::deserializeObjectMethod desMethod)
{
    if (Serializable::_name2convFuncs == NULL) {
        Serializable::_name2convFuncs = new Serializable::Name2ConvFuncsMap();
    }

    // register the conversion functions for this version
    std::string vname(className);
    vname += ":";
    vname += version;

    Serializable::ConversionFuncs& funcs
        = (*Serializable::_name2convFuncs)[vname];

    funcs.version = version;
    funcs.serialMethod = serMethod;
    funcs.createFunc = createFunc;
    funcs.deserialMethod = desMethod;

    // if this is the latest version, register it as "current"
    std::string cname(className);
    cname += ":current";

    Serializable::Name2ConvFuncsMap::iterator iter
        = Serializable::_name2convFuncs->find(cname);

    if (iter == Serializable::_name2convFuncs->end()) {
        // this is the first -- register it as "current"
        Serializable::ConversionFuncs& cfuncs
            = (*Serializable::_name2convFuncs)[cname];
        cfuncs.version = version;
        cfuncs.serialMethod = serMethod;
        cfuncs.createFunc = createFunc;
        cfuncs.deserialMethod = desMethod;
    } else {
        Serializable::ConversionFuncs& cfuncs
            = (*Serializable::_name2convFuncs)[cname];

        if (cfuncs.version < version) {
            // this is the latest -- register it as "current"
            cfuncs.version = version;
            cfuncs.serialMethod = serMethod;
            cfuncs.createFunc = createFunc;
            cfuncs.deserialMethod = desMethod;
        }
    }
}
