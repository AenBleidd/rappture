/*
 * ----------------------------------------------------------------------
 *  Rappture::encoding
 *
 *  The encoding module for rappture used to zip and b64 encode data.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2007  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpEncode.h"
#include <cstring>

/**********************************************************************/
// FUNCTION: Rappture::encoding::isbinary()
/// isbinary checks to see if given string is binary.
/**
 * Checks to see if any of size characters in *buf are binary
 * Full function call:
 * Rappture::encoding::isbinary(buf,size);
 *
 */

int
Rappture::encoding::isbinary(const char* buf, int size)
{
    if (buf == NULL) {
        return 0;
    }
    if (size < 0) {
        size = strlen(buf);
    }
    const char *cp, *endPtr;
    for (cp = buf, endPtr = buf + size; cp < endPtr; cp++) {
	if ((!isascii(*cp)) || (!isprint(*cp))) {
	    return (cp - buf) + 1;
	}
    }
    return 0;
}

/**********************************************************************/
// FUNCTION: Rappture::encoding::isencoded()
/// checks header of given string to determine if it was encoded by rappture.
/**
 * Checks to see if the string buf was encoded by rappture
 * and contains the proper "@@RP-ENC:" header.
 * rappture encoded strings start with the string "@@RP-ENC:X\n"
 * where X is one of z, b64, zb64
 * This function will not work for strings that do not have the header.
 * Full function call:
 * Rappture::encoding::isencoded(buf,size);
 *
 */

size_t
Rappture::encoding::isencoded(const char* buf, int size)
{
    size_t flags = 0;
    size_t len = 0;

    if (buf == NULL) {
        return flags;
    }

    if (size < 0) {
        len = strlen(buf);
    } else {
        len = size;
    }

    // check the following for valid rappture encoded string:
    // all strings encoded by rappture are at least 11 characters
    // rappture encoded strings start with the '@' character
    // rappture encoded strings start with the string "@@RP-ENC:X\n"
    // where X is one of z, b64, zb64
    if (    (len >= 11)
        &&  ('@' == *buf)
        &&  (strncmp("@@RP-ENC:",buf,9) == 0) ) {

        size_t idx = 9;

        // check the string length and if the z flag was specified
        // add 1 for \n
        if (    (len >= (idx + 1))
            &&  (buf[idx] == 'z') ) {
            flags |= RPENC_Z;
            ++idx;
        }
        // check the string length and if the b64 flag was specified
        // add 1 for \n
        if (    (len >= (idx + 2 + 1))
            &&  (buf[idx]   == 'b')
            &&  (buf[idx+1] == '6')
            &&  (buf[idx+2] == '4') ) {
            flags |= RPENC_B64;
            idx += 3;
        }
        // check for the '\n' at the end of the header
        if (buf[idx] != '\n') {
            flags = 0;
        }
    }
    return flags;
}

/**********************************************************************/
// FUNCTION: Rappture::encoding::encode()
/// Rappture::encoding::encode function encodes provided string
/**
 * Encode a string by compressing it with zlib and then base64 encoding it.
 *
 * Full function call:
 * Rappture::encoding::encode(buf,flags)
 */

bool
Rappture::encoding::encode(Rappture::Outcome &err, Rappture::Buffer& buf, 
	size_t flags)
{
    Rappture::Buffer outData;

    bool compress, base64, addHeader;
    compress  = (flags & RPENC_Z);
    base64    = (flags & RPENC_B64);
    addHeader = (flags & RPENC_HDR);

    if ((!compress) && (!base64)) {
	return true;		// Nothing to do.
    }
    if (outData.append(buf.bytes(), buf.size()) != (int)buf.size()) {
	err.addError("can't append %d bytes", buf.size());
	return false;
    }
    if (outData.encode(err, compress, base64)) {
	buf.clear();
	if (addHeader) {
	    if ((compress) && (!base64)) {
		buf.append("@@RP-ENC:z\n", 11);
	    } else if ((!compress) && (base64)) {
		buf.append("@@RP-ENC:b64\n", 13);
	    } else if ((compress) && (base64)) {
		buf.append("@@RP-ENC:zb64\n", 14);
	    } else {
		// do nothing
	    }
	} else {
	    // do nothing
	}
    }
    if (buf.append(outData.bytes(),outData.size()) != (int)outData.size()) {
	err.addError("can't append %d bytes", outData.size());
	return false;
    }
    return true;
}

/**********************************************************************/
// FUNCTION: Rappture::encoding::decode()
/// Rappture::encoding::decode function decodes provided string
/**
 * Decode a string by uncompressing it with zlib and base64 decoding it.
 * If binary data is provided, the data is base64 decoded and uncompressed.
 * Rappture::encoding::isbinary is used to qualify binary data.
 *
 * Full function call:
 * Rappture::encoding::decode (buf,flags)
 */

bool
Rappture::encoding::decode(Rappture::Outcome &err, Rappture::Buffer& buf, 
			   size_t flags)
{
    Rappture::Buffer outData;

    bool decompress, base64, checkHeader;
    decompress  = (flags & RPENC_Z);
    base64      = (flags & RPENC_B64);
    checkHeader = (flags & RPENC_HDR);

    off_t offset;
    const char *bytes;
    int size;

    size = buf.size();
    bytes = buf.bytes();
    if (strncmp(bytes, "@@RP-ENC:z\n", 11) == 0) {
	bytes += 11;
	size -= 11;
        if ((checkHeader) || ((!decompress) && (!base64))) {
            decompress = true;
            base64     = false;
        }
    } else if (strncmp(bytes, "@@RP-ENC:b64\n",13) == 0) {
	bytes += 13;
	size -= 13;
        if ((checkHeader) || ((!decompress) && (!base64) )) {
            decompress = false;
            base64     = true;
        }
    } else if (strncmp(bytes, "@@RP-ENC:zb64\n",14) == 0) {
	bytes += 14;
	size -= 14;
        if ((checkHeader) || ((!decompress) && (!base64))) {
            decompress = true;
            base64     = true;
        }
    } else {
	offset = 0;
    }
    if (outData.append(bytes, size) != size) {
	err.addError("can't append %d bytes to buffer", size);
	return false;
    }
    if ((decompress) || (base64)) {
	if (!outData.decode(err, decompress, base64)) {
	    return false;
	}
    }
    buf.move(outData);
    return true;
}

