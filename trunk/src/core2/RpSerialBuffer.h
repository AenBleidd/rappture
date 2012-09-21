/*
 * ======================================================================
 *  Rappture::SerialBuffer
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
#ifndef RPSERIALBUFFER_H
#define RPSERIALBUFFER_H

#include <string>
#include <vector>

namespace Rappture {

/**
 * Used by the Serializer to build up the buffer of serialized
 * data.  Similar to a string, but it handles nulls and other
 * control characters.  Also handles big/little endian order
 * properly.
 */
class SerialBuffer {
public:
    SerialBuffer();
    SerialBuffer(const char* bytes, int nbytes);
    SerialBuffer(const SerialBuffer& buffer);
    SerialBuffer& operator=(const SerialBuffer& buffer);
    virtual ~SerialBuffer();

    const char* bytes() const;
    int size() const;

    SerialBuffer& clear();
    SerialBuffer& writeChar(char cval);
    SerialBuffer& writeInt(int ival);
    SerialBuffer& writeDouble(double dval);
    SerialBuffer& writeString(const char* sval);
    SerialBuffer& writeBytes(const char* bval, int nbytes);

    void rewind();
    int atEnd() const;
    char readChar();
    int readInt();
    double readDouble();
    std::string readString();
    std::vector<char> readBytes();

private:
    /// Contains the actual data within this buffer.
    std::vector<char> _buffer;

    /// Position for the various readXyz() functions.
    int _pos;
};

} // namespace Rappture

#endif /*RPSERIALBUFFER_H*/
