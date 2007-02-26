/*
 * ======================================================================
 *  Rappture::RpBuffer
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2007  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 *
 *  This code is based on the Tcl_DString facility included in the
 *  Tcl source release, which includes the following copyright:
 *
 *  Copyright (c) 1987-1993 The Regents of the University of California.
 *  Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *  Copyright (c) 2001 by Kevin B. Kenny.  All rights reserved.
 *
 *  This software is copyrighted by the Regents of the University of
 *  California, Sun Microsystems, Inc., Scriptics Corporation,
 *  and other parties.  The following terms apply to all files associated
 *  with the software unless explicitly disclaimed in individual files.
 *
 *  The authors hereby grant permission to use, copy, modify, distribute,
 *  and license this software and its documentation for any purpose, provided
 *  that existing copyright notices are retained in all copies and that this
 *  notice is included verbatim in any distributions. No written agreement,
 *  license, or royalty fee is required for any of the authorized uses.
 *  Modifications to this software may be copyrighted by their authors
 *  and need not follow the licensing terms described here, provided that
 *  the new terms are clearly indicated on the first page of each file where
 *  they apply.
 *
 *  IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *  FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *  ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *  DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
 *  IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
 *  NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 *  MODIFICATIONS.
 *
 *  GOVERNMENT USE: If you are acquiring this software on behalf of the
 *  U.S. government, the Government shall have only "Restricted Rights"
 *  in the software and related documentation as defined in the Federal·
 *  Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *  are acquiring the software on behalf of the Department of Defense, the
 *  software shall be classified as "Commercial Computer Software" and the
 *  Government shall have only "Restricted Rights" as defined in Clause
 *  252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 *  authors grant the U.S. Government and others acting in its behalf
 *  permission to use and distribute the software in accordance with the
 *  terms specified in this license.·
 *
 * ======================================================================
 */

#include "RpBuffer.h"

using namespace Rappture;

/**
 * Construct an empty SimpleBuffer.
 */
SimpleBuffer::SimpleBuffer()
{
    bufferInit();
}


/**
 * Construct a SimpleBuffer loaded with initial data.
 *
 * @param bytes pointer to bytes being stored.
 * @param nbytes number of bytes being stored.
 */
SimpleBuffer::SimpleBuffer(const char* bytes, int nbytes)
{
    bufferInit();
    append(bytes,nbytes);
}


/**
 * Copy constructor
 * @param SimpleBuffer object to copy
 */
SimpleBuffer::SimpleBuffer(const SimpleBuffer& b)
{
    bufferInit();
    append(b.bytes(),b.size());
}


/**
 * Assignment operator
 * @param SimpleBuffer object to copy
 */
SimpleBuffer&
SimpleBuffer::operator=(const SimpleBuffer& b)
{
    bufferFree();
    bufferInit();
    append(b.bytes(),b.size());
    return *this;
}


/**
 * Operator +
 * @param SimpleBuffer object to add
 * @param SimpleBuffer object to add
 */
SimpleBuffer
SimpleBuffer::operator+(const SimpleBuffer& b) const
{
    SimpleBuffer newBuffer(*this);
    newBuffer.operator+=(b);
    return newBuffer;
}


/**
 * Operator +=
 * @param SimpleBuffer object to add
 */
SimpleBuffer&
SimpleBuffer::operator+=(const SimpleBuffer& b)
{
    append(b.bytes(),b.size());
    return *this;
}


/**
 * Destructor
 */
SimpleBuffer::~SimpleBuffer()
{
    bufferFree();
}


/**
 * Get the bytes currently stored in the buffer.  These bytes can
 * be stored, and used later to construct another Buffer to
 * decode the information.
 *
 * @return Pointer to the bytes in the buffer.
 */
const char*
SimpleBuffer::bytes() const
{
    return _buf;
}


/**
 * Get the number of bytes currently stored in the buffer.
 * @return Number of the bytes used in the buffer.
 */
unsigned int
SimpleBuffer::size() const
{
    return _size;
}


/**
 * Clear the buffer, making it empty.
 * @return Reference to this buffer.
 */
SimpleBuffer&
SimpleBuffer::clear()
{
    bufferFree();
    bufferInit();

    return *this;
}


/**
 * Append bytes to the end of this buffer
 * @param Buffer object to copy
 * @return Reference to this buffer.
 */
int
SimpleBuffer::append(const char* bytes, int nbytes)
{
    unsigned int newSize = 0;
    char *newBuffer = NULL;

    // User specified NULL buffer to append
    if (bytes == NULL) {
        return 0;
    }

    if (nbytes == -1) {
        // user signaled null terminated string
        nbytes = strlen(bytes);
    }

    if (nbytes <= 0) {
        // no data written, invalid option
        return nbytes;
    }

    // Empty internal buffer, make sure its properly initialized.
    if (_buf == NULL) {
        bufferInit();
    }

    newSize = (unsigned int)(_size + nbytes);

    // ensure that our smallest buffer is 200 bytes
    if (newSize < (RP_BUFFER_MIN_SIZE/2)) {
        newSize = (RP_BUFFER_MIN_SIZE/2);
    }

    /*
     * Allocate a larger buffer for the string if the current one isn't
     * large enough. Allocate extra space in the new buffer so that there
     * will be room to grow before we have to allocate again.
     */

    if (newSize >= _spaceAvl) {
        _spaceAvl = newSize * 2;
        newBuffer = new char[_spaceAvl];
        if (newBuffer == NULL) {
            // return memory error
            return -1;
        }
        if (_buf != NULL) {
            memcpy((void*) newBuffer, (void*) _buf, (size_t) _size);
            delete [] _buf;
            _buf = NULL;
        }
        _buf = newBuffer;
    }

    memcpy((void*) (_buf + _size), (void*) bytes, (size_t) nbytes);

    _size = _size + (unsigned int) nbytes;

    return nbytes;
}


/**
 * Read data from the buffer into a memory location provided by caller
 * @param Pointer locating where to place read bytes.
 * @param Size of the memory location.
 * @return Number of bytes written to memory location
 */
int
SimpleBuffer::read(const char* bytes, int nbytes)
{
    unsigned int bytesRead = 0;

    // SimpleBuffer is empty.
    if (_buf == NULL) {
        return 0;
    }

    // User specified NULL buffer.
    if (bytes == NULL) {
        return 0;
    }

    // User specified negative buffer size
    if (nbytes <= 0) {
        return 0;
    }

    // make sure we don't read off the end of our buffer
    if ( (_pos + nbytes) >= _size ) {
        bytesRead = (_size - _pos);
    }
    else {
        bytesRead = (unsigned int) nbytes;
    }

    if (bytesRead <= 0) {
        return 0;
    }

    if (bytesRead > 0) {
        memcpy((void*) bytes, (void*) (_buf+_pos), (size_t) bytesRead);
    }

    _pos = (_pos + bytesRead);

    return (int)bytesRead;
}


/**
 * Set buffer position indicator to spot within the buffer
 * @param Offset from whence location in buffer.
 * @param Place from where offset is added or subtracted.
 * @return 0 on success, anything else is failure
 */
int
SimpleBuffer::seek(int offset, int whence)
{
    int retVal = 0;

    if (_buf == NULL) {
        return -1 ;
    }

    if (whence == SEEK_SET) {
        if (offset < 0) {
            /* dont go off the beginning of data */
            _pos = 0;
        }
        else if (offset >= (int)_size) {
            /* dont go off the end of data */
            _pos = _size - 1;
        }
        else {
            _pos = (unsigned int)(_pos + offset);
        }
    }
    else if (whence == SEEK_CUR) {
        if ( (_pos + offset) < 0) {
            /* dont go off the beginning of data */
            _pos = 0;
        }
        else if ((_pos + offset) >= _size) {
            /* dont go off the end of data */
            _pos = _size - 1;
        }
        else {
            _pos = (unsigned int)(_pos + offset);
        }
    }
    else if (whence == SEEK_END) {
        if (offset <= (-1*((int)_size))) {
            /* dont go off the beginning of data */
            _pos = 0;
        }
        else if (offset >= 0) {
            /* dont go off the end of data */
            _pos = _size - 1;
        }
        else {
            _pos = (unsigned int)((_size - 1) + offset);
        }
    }
    else {
        retVal = -1;
    }

    return retVal;
}


/**
 * Tell caller the offset of the position indicator from the start of buffer
 * @return Number of bytes the position indicator is from start of buffer
 */
int
SimpleBuffer::tell()
{
   return (int)_pos;
}


/**
 * Read data from the buffer into a memory location provided by caller
 */
SimpleBuffer&
SimpleBuffer::rewind()
{
    _pos = 0;
    return *this;
}


/**
 * Tell if the last file like operation (ie. read()) was successful
 * or if there was a failure like eof, or bad memory
 * @return True or false boolean value
 */
bool
SimpleBuffer::good() const
{
    return (_fileState);
}


/**
 * Tell if the last file like operation (ie. read()) failed
 * Opposite of good()
 * @return True or false boolean value
 */
bool
SimpleBuffer::bad() const
{
    return (!_fileState);
}


/**
 * Tell if the position flag is at the end of the buffer
 * @return True or false boolean value
 */
bool
SimpleBuffer::eof() const
{
    return (_pos >= _size);
}


/**
 * Move the data from this SimpleBuffer to the SimpleBuffer provided by
 * the caller. All data except the _pos is moved and this SimpleBuffer is
 * re-initialized with bufferInit().
 * @param SimpleBuffer to move the information to
 * @return reference to this SimpleBuffer object.
 */
SimpleBuffer&
SimpleBuffer::move(SimpleBuffer& b)
{
    bufferFree();

    _buf = b._buf;
    _pos = b._pos;
    _size = b._size;
    _spaceAvl = b._spaceAvl;
    _fileState = b._fileState;

    b.bufferInit();

    return *this;
}


 /**
  *  Initializes a dynamic buffer, discarding any previous contents
  *  of the buffer. bufferFree() should have been called already
  *  if the dynamic buffer was previously in use.
  */
void
SimpleBuffer::bufferInit()
{
    _buf = NULL;
    _pos = 0;
    _size = 0;
    _spaceAvl = 0;
    _fileState = true;
}


/**
 *  Frees up any memory allocated for the dynamic buffer and
 *  reinitializes the buffer to an empty state.
 */
void
SimpleBuffer::bufferFree()
{
    if (_buf != NULL) {
        delete [] _buf;
        _buf = NULL;
    }
    bufferInit();
}


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
Buffer::encode (bool compress, bool base64)
{
    Outcome err;
    SimpleBuffer bin;
    SimpleBuffer bout;

    err.addContext("Rappture::Buffer::encode()");

    rewind();

    if (compress) {
        do_compress(err,*this,bout);
    }

    if (base64) {
        if (compress) {
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
Buffer::decode (bool decompress, bool base64)
{
    Outcome err;
    SimpleBuffer bin;
    SimpleBuffer bout;

    rewind();

    if (base64) {
        do_base64_dec(err,*this,bout);
    }

    if (decompress) {
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
