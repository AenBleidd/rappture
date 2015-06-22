/*
 * ======================================================================
 *  Rappture::Buffer
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <zlib.h>
#include "b64/encode.h"
#include "b64/decode.h"
#include "RpBuffer.h"
#include "RpEncode.h"

namespace Rappture {

/**
 * Construct an empty Buffer.
 */
Buffer::Buffer()
  : SimpleCharBuffer(),
    _level(6),
    _compressionType(RPCOMPRESS_GZIP),
    _windowBits(15)
{}

/**
 * Construct an empty Buffer of specified size.
 */
Buffer::Buffer(int nbytes)
  : SimpleCharBuffer(nbytes),
    _level(6),
    _compressionType(RPCOMPRESS_GZIP),
    _windowBits(15)
{}

/**
 * Construct a Buffer loaded with initial data.
 *
 * @param bytes pointer to bytes being stored.
 * @param nbytes number of bytes being stored.
 */
Buffer::Buffer(const char* bytes, int nbytes)
  : SimpleCharBuffer(bytes,nbytes),
    _level(6),
    _compressionType(RPCOMPRESS_GZIP),
    _windowBits(15)
{}

/**
 * Copy constructor
 * @param Buffer object to copy
 */
Buffer::Buffer(const Buffer& b)
  : SimpleCharBuffer(b),
    _level(b._level),
    _compressionType(b._compressionType),
    _windowBits(b._windowBits)
{}

/**
 * Assignment operator
 * @param Buffer object to copy
 */
Buffer&
Buffer::operator=(const Buffer& b)
{
    SimpleCharBuffer::operator=(b);

    _level = b._level;
    _compressionType = b._compressionType;
    _windowBits = b._windowBits;

    return *this;
}

Buffer
Buffer::operator+(const Buffer& b) const
{
    Buffer newBuffer(*this);
    newBuffer.operator+=(b);
    return newBuffer;
}

Buffer&
Buffer::operator+=(const Buffer& b)
{
    SimpleCharBuffer::operator+=(b);
    return *this;
}

Buffer::~Buffer()
{}

bool
Buffer::load (Outcome &status, const char *path)
{
    status.addContext("Rappture::Buffer::load()");

    FILE *f;
    f = fopen(path, "rb");
    if (f == NULL) {
        status.addError("can't open \"%s\": %s", path, strerror(errno));
        return false;
    }

    struct stat stat;
    if (fstat(fileno(f), &stat) < 0) {
        status.addError("can't stat \"%s\": %s", path, strerror(errno));
        return false;
    }

    size_t oldSize, numBytesRead;

    // Save the # of elements in the current buffer.
    oldSize = count();

    // Extend the buffer to accomodate the file contents.
    if (extend(stat.st_size) == 0) {
        status.addError("can't allocate %d bytes for file \"%s\": %s",
		stat.st_size, path, strerror(errno));
        fclose(f);
        return false;
    }	
    // Read the file contents directly onto the end of the old buffer.
    numBytesRead = fread((char *)bytes() + oldSize, sizeof(char),
	stat.st_size, f);
    fclose(f);
    if (numBytesRead != (size_t)stat.st_size) {
	status.addError("can't read %ld bytes from \"%s\": %s", stat.st_size,
			path, strerror(errno));
	return false;
    }
    // Reset the # of elements in the buffer to the new count.
    count(stat.st_size + oldSize);
    return true;
}

bool
Buffer::dump(Outcome &status, const char* path)
{
    status.addContext("Rappture::Buffer::dump()");

    FILE *f;
    f = fopen(path, "wb");
    if (f == NULL) {
        status.addError("can't open \"%s\": %s\n", path, strerror(errno));
        return false;
    }
    ssize_t nWritten;
    nWritten = fwrite(bytes(), sizeof(char), size(), f);
    fclose(f);                        // Close the file.

    if (nWritten != (ssize_t)size()) {
        status.addError("can't write %d bytes to \"%s\": %s\n", size(),
                        path, strerror(errno));
        return false;
    }
    return true;
}

bool
Buffer::encode(Outcome &status, unsigned int flags)
{
    SimpleCharBuffer bout;

    rewind();

    switch (flags & (RPENC_Z | RPENC_B64)) {
    case 0:
        break;

    case RPENC_Z:                // Compress only
        if (!do_compress(status, *this, bout)) {
            return false;
        }
        move(bout);
        break;

    case RPENC_B64:                // Encode only
        if (!do_base64_enc(status, *this, bout)) {
            return false;
        }
        move(bout);
        break;

    case (RPENC_B64 | RPENC_Z):

        // It's always compress then encode
        if (!do_compress(status, *this, bout)) {
            return false;
        }
        if (!do_base64_enc(status, bout, *this)) {
            return false;
        }
        break;
    }
    return true;
}

bool
Buffer::decode(Outcome &status, unsigned int flags)
{
    SimpleCharBuffer bout;

    rewind();

    switch (flags & (RPENC_Z | RPENC_B64)) {
    case 0:
        if (encoding::isBase64(bytes(), size())) {
            if (!do_base64_dec(status, *this, bout)) {
                return false;
            }
            move(bout);
        }
        bout.clear();
        if (encoding::isBinary(bytes(), size())) {
            if (!do_decompress(status, *this, bout)) {
                return false;
            }
            move(bout);
        }
        break;

    case RPENC_Z:                // Decompress only
        if (!do_decompress(status, *this, bout)) {
            return false;
        }
        move(bout);
        break;

    case RPENC_B64:                // Decode only
        if (!do_base64_dec(status, *this, bout)) {
            return false;
        }
        move(bout);
        break;

    case (RPENC_B64 | RPENC_Z):

        // It's always decode then decompress
        if (!do_base64_dec(status, *this, bout)) {
            return false;
        }
        clear();
        if (!do_decompress(status, bout, *this)) {
            return false;
        }
        break;
    }
    return true;
}

bool
Buffer::do_compress(Outcome& status, SimpleCharBuffer& bin,
                    SimpleCharBuffer& bout)
{
    int ret=0, flush=0;
    z_stream strm;

    char in[CHUNK];
    char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    status.addContext("Rappture::Buffer::do_compress()");

    ret = deflateInit2( &strm, _level, Z_DEFLATED,
                        _windowBits+_compressionType,
                        8, Z_DEFAULT_STRATEGY);

    if (ret != Z_OK) {
        status.addError("error while initializing zlib stream object");
        return false;
    }

    /* compress until end of file */
    do {
        strm.avail_in = bin.read(in, CHUNK);
        if (bin.bad() == true) {
            (void)deflateEnd(&strm);
            // return Z_ERRNO;
            status.addError("error while compressing");
            return false;
        }
        flush = bin.eof() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = (Bytef*) in;
        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = (Bytef*) out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

	    int have;
            have = CHUNK - strm.avail_out;
            /* write to file and check for error */
            if ((have > 0) && (bout.append(out, have) == 0)) {
                (void)deflateEnd(&strm);
                bout.clear();
                // return Z_ERRNO;
                status.addError("error writing compressed data to temp buffer numBytes=%d", have);
                return false;
            }

        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);

    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    // return Z_OK;
    return true;
}

bool
Buffer::do_decompress(Outcome& status, SimpleCharBuffer& bin,
                      SimpleCharBuffer& bout)
{
    int ret;
    unsigned have;
    z_stream strm;

    char in[CHUNK];
    char out[CHUNK];

    int bytesWritten = 0;

    status.addContext("Rappture::Buffer::do_decompress()");

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm,_windowBits+_compressionType);
    if (ret != Z_OK) {
        status.addError("error while initializing zlib stream object");
        return false;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = bin.read(in, CHUNK);
        if (bin.bad() == true) {
            (void)inflateEnd(&strm);
            // return Z_ERRNO;
            status.addError("error while compressing");
            return false;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = (unsigned char*) in;
        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = (unsigned char*) out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                bout.clear();
                status.addError("memory error while inflating data");
                return false;
            }
            have = CHUNK - strm.avail_out;
            bytesWritten = bout.append(out, have);
            if ( ( (unsigned) bytesWritten != have) ) {
                (void)inflateEnd(&strm);
                bout.clear();
                // return Z_ERRNO;
                status.addError("error writing compressed data to temp buffer");
                return false;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    // return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
    return true;
}

bool
Buffer::do_base64_enc(Outcome& status, const SimpleCharBuffer& bin,
                      SimpleCharBuffer& bout )
{
    int tBufSize = 0;
    unsigned int tBufAvl = 2*bin.size();
    char* tBuf = new char[tBufAvl];

    base64::encoder E;

    tBufSize = E.encode(bin.bytes(),bin.size(),tBuf);
    tBufSize += E.encode_end(tBuf+tBufSize);

    bout = SimpleCharBuffer(tBuf,tBufSize);
    delete [] tBuf;

    return true;
}

bool
Buffer::do_base64_dec(Outcome& status, const SimpleCharBuffer& bin,
                      SimpleCharBuffer& bout )
{
    int tBufSize = 0;
    unsigned int tBufAvl = bin.size();
    char* tBuf = new char[tBufAvl];

    base64::decoder D;

    tBufSize = D.decode(bin.bytes(),bin.size(),tBuf);

    bout = SimpleCharBuffer(tBuf,tBufSize);
    delete [] tBuf;

    return true;
}

}
