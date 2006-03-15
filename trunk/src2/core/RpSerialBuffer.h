/*
 * ----------------------------------------------------------------------
 *  Rappture::SerialBuffer
 *    Used by the Serializer to build up the buffer of serialized
 *    data.  Similar to a string, but it handles nulls and other
 *    control characters.  Also handles big/little endian order
 *    properly.
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
#ifndef RAPPTURE_SERIALBUFFER_H
#define RAPPTURE_SERIALBUFFER_H

#include <string>
#include <vector>

namespace Rappture {

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
    std::vector<char> _buffer;  // data within this buffer
    int _pos;                   // position for read functions
};

} // namespace Rappture

#endif
