/*
 * ======================================================================
 *  Rappture::Buffer
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2008  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpBuffer.h"

#include "zlib.h"
#include "b64/encode.h"
#include "b64/decode.h"
#include <fstream>
#include <assert.h>

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

using namespace Rappture;

/**
 * Construct an empty Buffer.
 */
Buffer::Buffer()
  : SimpleBuffer(),
    _level(6),
    _compressionType(RPCOMPRESS_GZIP),
    _windowBits(15)
{}


/**
 * Construct an empty Buffer of specified size.
 */
Buffer::Buffer(int nbytes)
  : SimpleBuffer(nbytes),
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
  : SimpleBuffer(bytes,nbytes),
    _level(6),
    _compressionType(RPCOMPRESS_GZIP),
    _windowBits(15)
{}

/**
 * Copy constructor
 * @param Buffer object to copy
 */
Buffer::Buffer(const Buffer& b)
  : SimpleBuffer(b),
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
    SimpleBuffer::operator=(b);

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
    SimpleBuffer::operator+=(b);
    return *this;
}


Buffer::~Buffer()
{}


Outcome
Buffer::load (const char* filePath)
{
    Outcome status;
    std::ifstream::pos_type size = 0;
    std::ifstream inFile;
    char* memblock = NULL;


    inFile.open(filePath, std::ios::in | std::ios::ate | std::ios::binary);
    if (!inFile.is_open()) {
        status.error("error while opening file");
        status.addContext("Rappture::Buffer::load()");
        return status;
    }

    size = inFile.tellg();
    memblock = new char [size];
    if (memblock == NULL) {
        status.error("error while allocating memory");
        status.addContext("Rappture::Buffer::load()");
        inFile.close();
        return status;
    }

    inFile.seekg(0,std::ios::beg);
    inFile.read(memblock,size);

    // save data in buffer object.
    append(memblock,size);

    // close files, free memory
    inFile.close();
    delete [] memblock;
    memblock = NULL;

    // exit nicely
    return status;
}


Outcome
Buffer::dump (const char* filePath)
{
    Outcome status;
    std::ofstream outFile;

    outFile.open(filePath, std::ios::out|std::ios::trunc|std::ios::binary);
    if (!outFile.is_open()) {
        status.error("error while opening file");
        status.addContext("Rappture::Buffer::dump()");
        return status;
    }

    outFile.write(bytes(),size());
    outFile.close();

    // exit nicely
    return status;
}


Outcome
Buffer::encode (unsigned int compress, unsigned int base64)
{
    Outcome err;
    SimpleBuffer bin;
    SimpleBuffer bout;

    if ((base64 == 0) && (compress == 0)) {
        return err;
    }

    err.addContext("Rappture::Buffer::encode()");
    rewind();

    if (compress != 0) {
        do_compress(err,*this,bout);
        if (err) {
            return err;
        }
    }

    if (base64 != 0) {
        if (compress != 0) {
            bin.move(bout);
            do_base64_enc(err,bin,bout);
        }
        else {
            do_base64_enc(err,*this,bout);
        }
    }

    if (!err) {
        // write the encoded data to the internal buffer
        move(bout);
    }

    return err;
}


Outcome
Buffer::decode (unsigned int decompress, unsigned int base64)
{
    Outcome err;
    SimpleBuffer bin;
    SimpleBuffer bout;

    if ((base64 == 0) && (decompress == 0)) {
        return err;
    }

    err.addContext("Rappture::Buffer::decode()");
    rewind();

    if (base64 != 0) {
        do_base64_dec(err,*this,bout);
        if (err) {
            return err;
        }
    }

    if (decompress != 0) {
        if (base64) {
            bin.move(bout);
            do_decompress(err,bin,bout);
        }
        else {
            do_decompress(err,*this,bout);
        }
    }

    if (!err) {
        // write the decoded data to the internal buffer
        move(bout);
    }

    return err;
}


void
Buffer::do_compress(    Outcome& status,
                        SimpleBuffer& bin,
                        SimpleBuffer& bout  )
{
    int ret=0, flush=0;
    unsigned have=0;
    z_stream strm;

    char in[CHUNK];
    char out[CHUNK];

    int bytesWritten = 0;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit2( &strm, _level, Z_DEFLATED,
                        _windowBits+_compressionType,
                        8, Z_DEFAULT_STRATEGY);

    if (ret != Z_OK) {
        status.error("error while initializing zlib stream object");
        status.addContext("Rappture::Buffer::do_compress()");
        return;
    }

    /* compress until end of file */
    do {
        strm.avail_in = bin.read(in, CHUNK);
        if (bin.bad() == true) {
            (void)deflateEnd(&strm);
            // return Z_ERRNO;
            status.error("error while compressing");
            status.addContext("Rappture::Buffer::do_compress()");
            return;
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
            have = CHUNK - strm.avail_out;
            /* write to file and check for error */
            bytesWritten = bout.append(out, have);
            if ( ( (unsigned) bytesWritten != have ) ) {
                (void)deflateEnd(&strm);
                bout.clear();
                // return Z_ERRNO;
                status.error("error writing compressed data to temp buffer");
                status.addContext("Rappture::Buffer::do_compress()");
                return;
            }

        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);

    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    // return Z_OK;
    return;
}

void
Buffer::do_decompress(  Outcome& status,
                        SimpleBuffer& bin,
                        SimpleBuffer& bout  )
{
    int ret;
    unsigned have;
    z_stream strm;

    char in[CHUNK];
    char out[CHUNK];

    int bytesWritten = 0;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm,_windowBits+_compressionType);
    if (ret != Z_OK) {
        status.error("error while initializing zlib stream object");
        status.addContext("Rappture::Buffer::do_decompress()");
        // return status;
        return;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = bin.read(in, CHUNK);
        if (bin.bad() == true) {
            (void)inflateEnd(&strm);
            // return Z_ERRNO;
            status.error("error while compressing");
            status.addContext("Rappture::Buffer::do_decompress()");
            // return status;
            return;
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
                status.error("memory error while inflating data");
                status.addContext("Rappture::Buffer::do_decompress()");
                return;
            }
            have = CHUNK - strm.avail_out;
            bytesWritten = bout.append(out, have);
            if ( ( (unsigned) bytesWritten != have) ) {
                (void)inflateEnd(&strm);
                bout.clear();
                // return Z_ERRNO;
                status.error("error writing compressed data to temp buffer");
                status.addContext("Rappture::Buffer::do_decompress()");
                return;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    // return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
    return;
}


void
Buffer::do_base64_enc(  Outcome& status,
                        const SimpleBuffer& bin,
                        SimpleBuffer& bout )
{
    int tBufSize = 0;
    unsigned int tBufAvl = 2*bin.size();
    char* tBuf = new char[tBufAvl];

    base64::encoder E;

    tBufSize = E.encode(bin.bytes(),bin.size(),tBuf);
    tBufSize += E.encode_end(tBuf+tBufSize);

    bout = SimpleBuffer(tBuf,tBufSize);
    delete [] tBuf;

    return;
}


void
Buffer::do_base64_dec(  Outcome& status,
                        const SimpleBuffer& bin,
                        SimpleBuffer& bout )
{
    int tBufSize = 0;
    unsigned int tBufAvl = bin.size();
    char* tBuf = new char[tBufAvl];

    base64::decoder D;

    tBufSize = D.decode(bin.bytes(),bin.size(),tBuf);

    bout = SimpleBuffer(tBuf,tBufSize);
    delete [] tBuf;

    return;
}

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

