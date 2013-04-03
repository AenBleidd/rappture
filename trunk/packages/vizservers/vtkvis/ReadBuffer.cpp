/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <unistd.h>
#include <cstring>

#include "ReadBuffer.h"
#include "Trace.h"

using namespace VtkVis;

/**
 * \param[in] fd File descriptor to read
 * \param[in] bufferSize Block size to use in internal buffer
 */
ReadBuffer::ReadBuffer(int fd, size_t bufferSize) :
    _bufferSize(bufferSize),
    _fd(fd),
    _lastStatus(OK)
{
    _bytes = new unsigned char [_bufferSize];
    flush();
}

ReadBuffer::~ReadBuffer()
{
    TRACE("Deleting ReadBuffer");
    delete [] _bytes;
}

/**
 * \brief Checks if a new line is currently in the buffer.
 *
 * \return the index of the character past the new line.
 */
size_t
ReadBuffer::nextNewLine()
{
    /* Check for a newline in the current buffer. */
    unsigned char *ptr = 
        (unsigned char *)memchr(_bytes + _mark, '\n', _fill - _mark);
    if (ptr == NULL)
        return 0;
    else
        return (ptr - _bytes + 1);
}

/**  
 * \brief Fills the buffer with available data.
 * 
 * Any existing data in the buffer is moved to the front of the buffer, 
 * then the channel is read to fill the rest of the buffer.
 *
 * \return If an error occur when reading the channel, then ERROR is
 * returned. ENDFILE is returned on EOF.  If the buffer can't be filled, 
 * then CONTINUE is returned.
 */
ReadBuffer::BufferStatus
ReadBuffer::doFill()
{
    //TRACE("Enter, mark: %lu fill: %lu", _mark, _fill);
    if (_mark >= _fill) {
        flush();                        /* Fully consumed buffer */
    }
    if (_mark > 0) {
        /* Some data has been consumed. Move the unconsumed data to the front
         * of the buffer. */
        TRACE("memmove %lu bytes", _fill-_mark);
        memmove(_bytes, _bytes + _mark, _fill - _mark);
        _fill -= _mark;
        _mark = 0;
    }
    ssize_t numRead;
    size_t bytesLeft;

    bytesLeft = _bufferSize - _fill;
    //TRACE("going to read %lu bytes", bytesLeft);
    numRead = read(_fd, _bytes + _fill, bytesLeft);
    if (numRead == 0) {
        /* EOF */
        TRACE("EOF found reading buffer");
        return ReadBuffer::ENDFILE;
    }
    if (numRead < 0) {
        if (errno != EAGAIN) {
            ERROR("error reading buffer: %s", strerror(errno));
            return ReadBuffer::ERROR;
        }
        TRACE("Short read for buffer");
        return ReadBuffer::CONTINUE;
    }
    _fill += numRead;
    //TRACE("Read %lu bytes", numRead);
    return ((size_t)numRead == bytesLeft) 
        ? ReadBuffer::OK : ReadBuffer::CONTINUE;
}

/**
 * \brief Read the requested number of bytes from the buffer.

 * Fails if the requested number of bytes are not immediately 
 * available. Never should be short. 
 */
ReadBuffer::BufferStatus
ReadBuffer::followingData(unsigned char *out, size_t numBytes)
{
    TRACE("Enter");
    while (numBytes > 0) {
        size_t bytesLeft;

        bytesLeft = _fill - _mark;
        if (bytesLeft > 0) {
            int size;

            /* Pull bytes out of the buffer, updating the mark. */
            size = (bytesLeft >  numBytes) ? numBytes : bytesLeft;
            memcpy(out, _bytes + _mark, size);
            _mark += size;
            numBytes -= size;
            out += size;
        }
        if (numBytes == 0) {
            /* Received requested # bytes. */
            return ReadBuffer::OK;
        }
        /* Didn't get enough bytes, need to read some more. */
        _lastStatus = doFill();
        if (_lastStatus == ReadBuffer::ERROR ||
            _lastStatus == ReadBuffer::ENDFILE) {
            return _lastStatus;
        }
    }
    return ReadBuffer::OK;
}

/** 
 * \brief Returns the next available line (terminated by a newline)
 * 
 * If insufficient data is in the buffer, then the channel is
 * read for more data.  If reading the channel results in a
 * short read, CONTINUE is returned and *numBytesPtr is set to 0.
 */
ReadBuffer::BufferStatus
ReadBuffer::getLine(size_t *numBytesPtr, unsigned char **bytesPtr)
{
    TRACE("Enter");
    *numBytesPtr = 0;
    *bytesPtr = NULL;

    _lastStatus = ReadBuffer::OK;
    for (;;) {
        size_t _newline;

        _newline = nextNewLine();
        if (_newline > 0) {
            /* Start of the line. */
            *bytesPtr = _bytes + _mark;
            /* Number of bytes in the line. */
            *numBytesPtr = _newline - _mark;
            _mark = _newline;
            return ReadBuffer::OK;
        }
        /* Couldn't find a newline, so it may be that we need to read some
         * more. Check first that last read wasn't a short read. */
        if (_lastStatus == ReadBuffer::CONTINUE) {
            /* No complete line just yet. */
            return ReadBuffer::CONTINUE;
        }
        /* Try to add more data to the buffer. */
        _lastStatus = doFill();
        if (_lastStatus == ReadBuffer::ERROR ||
            _lastStatus == ReadBuffer::ENDFILE) {
            return _lastStatus;
        }
        /* OK or CONTINUE */
    }
    return ReadBuffer::CONTINUE;
}
