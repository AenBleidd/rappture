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

#ifndef RAPPTURE_BUFFER_H
#define RAPPTURE_BUFFER_H

#include <RpOutcome.h>
#include <RpSimpleBuffer.h>


#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

enum RP_COMPRESS_CONSTS {
    RPCOMPRESS_ZLIB      = 0,
    RPCOMPRESS_GZIP      = 16
};

namespace Rappture {

/**
 * Block of memory that dynamically resizes itself as needed.
 * The buffer also allows caller to massage the data it holds
 * with functionallity including gzip compression based on zlib
 * and base64 encoding based on libb64.
 */

class Buffer : public SimpleCharBuffer{
public:
    Buffer();
    Buffer(int nbytes);
    Buffer(const char* bytes, int nbytes=-1);
    Buffer(const Buffer& buffer);
    Buffer& operator=(const Buffer& b);
    Buffer  operator+(const Buffer& b) const;
    Buffer& operator+=(const Buffer& b);
    virtual ~Buffer();

    Outcome load(const char* filePath);
    Outcome dump(const char* filePath);
    Outcome encode(unsigned int compress=1, unsigned int base64=1);
    Outcome decode(unsigned int decompress=1, unsigned int base64=1);

protected:

    /// Compression level for the zlib functions
    int _level;

    /// compression type for the zlib functions.
    int _compressionType;

    /// number of window bits, adjusts speed of compression
    int _windowBits;

    enum { CHUNK = 4096 };

    void do_compress(   Outcome& status,
                        SimpleCharBuffer& bin,
                        SimpleCharBuffer& bout  );
    void do_decompress( Outcome& status,
                        SimpleCharBuffer& bin,
                        SimpleCharBuffer& bout  );
    void do_base64_enc( Outcome& status,
                        const SimpleCharBuffer& bin,
                        SimpleCharBuffer& bout  );
    void do_base64_dec( Outcome& status,
                        const SimpleCharBuffer& bin,
                        SimpleCharBuffer& bout  );
};

} // namespace Rappture
 
#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // RAPPTURE_BUFFER_H
