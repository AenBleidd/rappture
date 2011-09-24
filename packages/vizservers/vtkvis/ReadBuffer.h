/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#include <cstdlib>

#ifndef _READBUFFER_H
#define _READBUFFER_H

namespace Rappture {
namespace VtkVis {

/**
 * \brief Buffered input for reading socket
 */
class ReadBuffer
{
public:
    enum BufferStatus {
        ENDFILE=-1,
        OK,
        ERROR,
        CONTINUE,
    };

    ReadBuffer(int fd, size_t bufferSize);

    virtual ~ReadBuffer();

    BufferStatus getLine(size_t *numBytesPtr, unsigned char **bytesPtr);

    BufferStatus followingData(unsigned char *out, size_t numBytes);

    /**
     * \brief Check if buffer contains data ending in a newline
     * 
     * \return bool indicating if getLine can be called without blocking 
     */
    bool isLineAvailable()
    {
        return (nextNewLine() > 0);
    }

    /**
     * \brief Get the file descriptor this object reads from
     */
    int file()
    {
        return _fd;
    }

    /**
     * \brief Get the status of the last read
     */
    BufferStatus status()
    {
        return _lastStatus;
    }

private:
    unsigned char *_bytes;		/**< New-ed buffer to hold read
					 * characters. */
    size_t _bufferSize;			/**< # of bytes allocated for buffer */
    size_t _fill;			/**< # of bytes used in the buffer */
    size_t _mark;			/**< Starting point of the unconsumed
					 * data in the buffer. */
    int _fd;				/**< File descriptor to get data. */
    BufferStatus _lastStatus;           /**< Status of last read operation. */

    BufferStatus doFill();

    void flush()
    {
        _fill = _mark = 0;
    }

    size_t nextNewLine();
};

}
}

#endif /* _READBUFFER_H */
