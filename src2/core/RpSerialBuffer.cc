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
#include "RpSerialBuffer.h"

#ifdef BIGENDIAN
#  define ENDIAN_FOR_LOOP(var,size)  \
     for (int var=size-1; var >= 0; var--)
#else
#  define ENDIAN_FOR_LOOP(var,size)  \
     for (int var=0; var < size; var++)
#endif

using namespace Rappture;

SerialBuffer::SerialBuffer()
  : _buffer(),
    _pos(0)
{
}

SerialBuffer::SerialBuffer(const char* bytes, int nbytes)
  : _buffer(),
    _pos(0)
{
    _buffer.reserve(nbytes);
    while (nbytes-- > 0) {
        _buffer.push_back( *bytes++ );
    }
}

SerialBuffer::SerialBuffer(const SerialBuffer& sb)
  : _buffer(sb._buffer),
    _pos(0)  // auto-rewind
{
}

SerialBuffer&
SerialBuffer::operator=(const SerialBuffer& sb)
{
    _buffer = sb._buffer;
    _pos = 0;  // auto-rewind
}

SerialBuffer::~SerialBuffer()
{
}

const char*
SerialBuffer::bytes() const
{
    return &_buffer[0];
}

int
SerialBuffer::size() const
{
    return _buffer.size();
}

SerialBuffer&
SerialBuffer::clear()
{
    _buffer.clear();
    return *this;
}

SerialBuffer&
SerialBuffer::writeChar(char cval)
{
    _buffer.push_back(cval);
    return *this;
}

SerialBuffer&
SerialBuffer::writeInt(int ival)
{
    char *ptr = (char*)(&ival);

    ENDIAN_FOR_LOOP(i, sizeof(int)) {
        _buffer.push_back(ptr[i]);
    }
    return *this;
}

SerialBuffer&
SerialBuffer::writeDouble(double dval)
{
    char *ptr = (char*)(&dval);

    ENDIAN_FOR_LOOP(i, sizeof(double)) {
        _buffer.push_back(ptr[i]);
    }
    return *this;
}

SerialBuffer&
SerialBuffer::writeString(const char* sval)
{
    while (*sval != '\0') {
        _buffer.push_back(*sval++);
    }
    _buffer.push_back('\0');
    return *this;
}

SerialBuffer&
SerialBuffer::writeBytes(const char* bval, int nbytes)
{
    writeInt(nbytes);
    while (nbytes-- > 0) {
        _buffer.push_back(*bval++);
    }
    return *this;
}

void
SerialBuffer::rewind()
{
    _pos = 0;
}

int
SerialBuffer::atEnd() const
{
    return (_pos >= _buffer.size());
}

char
SerialBuffer::readChar()
{
    char c = '\0';
    if (_pos < _buffer.size()) {
        c = _buffer[_pos++];
    }
    return c;
}

int
SerialBuffer::readInt()
{
    int ival = 0;
    char *ptr = (char*)(&ival);

    ENDIAN_FOR_LOOP(i, sizeof(int)) {
        if (_pos < _buffer.size()) {
            ptr[i] = _buffer[_pos++];
        }
    }
    return ival;
}

double
SerialBuffer::readDouble()
{
    double dval = 0;
    char *ptr = (char*)(&dval);

    ENDIAN_FOR_LOOP(i, sizeof(double)) {
        if (_pos < _buffer.size()) {
            ptr[i] = _buffer[_pos++];
        }
    }
    return dval;
}

std::string
SerialBuffer::readString()
{
    std::string sval;
    char c;
    while (_pos < _buffer.size()) {
        c = _buffer[_pos++];
        if (c == '\0') {
            break;
        } else {
            sval.push_back(c);
        }
    }
    return sval;
}

std::vector<char>
SerialBuffer::readBytes()
{
    int nbytes;
    std::vector<char> bval;

    nbytes = readInt();
    while (_pos < _buffer.size() && nbytes-- > 0) {
        bval.push_back( _buffer[_pos++] );
    }
    return bval;
}
