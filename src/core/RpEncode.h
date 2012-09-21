
/*
 * ======================================================================
 *  Rappture::encoding
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RP_ENCODE_H
#define RP_ENCODE_H

#include <RpOutcome.h>
#include <RpBuffer.h>

namespace Rappture {
namespace encoding {

enum RapptureEncodingFlags {
    RPENC_Z  =(1<<0),		/* Compress/Decompress the string. */
    RPENC_B64=(1<<1),		/* Base64 encode/decode the string. */ 
    RPENC_HDR=(1<<2),		/* Placebo. Header is by default added. */
    RPENC_RAW=(1<<3)		/* Treat the string as raw input. 
				 * Decode: ignore the header.
				 * Encode: don't add a header.
				 */
};

bool isBinary(const char* buf, int size);
bool isBase64(const char* buf, int size);
bool isGzipped(const char* buf, int size);
unsigned int headerFlags(const char* buf, int size);
bool encode(Rappture::Outcome &err, Rappture::Buffer& buf, unsigned int flags);
bool decode(Rappture::Outcome &err, Rappture::Buffer& buf, unsigned int flags);

}
}
#endif /*RP_ENCODE_H*/
