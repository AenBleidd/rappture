
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

/* 
 * Valid XML (ASCII encoded) characters 0-127:
 * 
 *	0x9 (\t) 0xA (\n) 0xD (\r) and
 *	0x20 (space) through 0x7F (del)
 *
 * This isn't for UTF-8, only ASCII.  We don't allow high-order bit
 * characters for ASCII editors.
 */
static unsigned char _xmlchars[256] = {
    /*     -  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    /*00*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,
    /*10*/    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*20*/    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*30*/    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*40*/    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*50*/    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*60*/    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*70*/    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /*80*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*90*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*A0*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*B0*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*C0*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*E0*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*F0*/    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* 
 * This routine is misnamed "isBinary". By definition, all strings are binary,
 * even the ones with just letters or digits.  It's really a test if the
 * string can be used by XML verbatim and if it can be read by the usual ASCII
 * editors (the reason high-order bit characters are disallowed).  [Note we
 * aren't checking if entity replacements are necessary.]
 *
 * The "is*" routines should be moved somewhere else since they really don't
 * have anything to do with encoding/decoding.
 */
bool
Rappture::encoding::isBinary(const char* buf, int size)
{
    if (buf == NULL) {
        return false;			/* Really should let this segfault. */
    }
    if (size < 0) {
        size = strlen(buf);
    }
    const unsigned char *cp, *endPtr;
    for (cp = (const unsigned char *)buf, endPtr = cp + size; cp < endPtr; 
	 cp++) {
	if (!_xmlchars[*cp]) {
	    return true;		
	}
    }
    return false;
}

bool
Rappture::encoding::isBase64(const char* buf, int size)
{
    if (buf == NULL) {
        return false;			/* Really should let this segfault. */
    }
    if (size < 0) {
        size = strlen(buf);
    }
    const char *cp, *endPtr;
    for (cp = buf, endPtr = buf + size; cp < endPtr; cp++) {
        if (((*cp < 'A') || (*cp > '/')) && (!isspace(*cp)) && (*cp != '=')) {
            return false;
        }
    }
    return true;
}

/**********************************************************************/
// FUNCTION: Rappture::encoding::headerFlags()
/// checks header of given string to determine if it was encoded by rappture.
/**
 * Checks to see if the string buf was encoded by rappture
 * and contains the proper "@@RP-ENC:" header.
 * rappture encoded strings start with the string "@@RP-ENC:X\n"
 * where X is one of z, b64, zb64
 * This function will not work for strings that do not have the header.
 * Full function call:
 * Rappture::encoding::headerFlags(buf,size);
 *
 */

unsigned int
Rappture::encoding::headerFlags(const char* buf, int size)
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
    if ((len >= 11) &&  ('@' == *buf) &&  (strncmp("@@RP-ENC:",buf,9) == 0) ) {

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
Rappture::encoding::encode(Rappture::Outcome &status, Rappture::Buffer& buf,
                           unsigned int flags)
{
    Rappture::Buffer outData;

    if (buf.size() <= 0) {
        return true;                // Nothing to encode.
    }
    if ((flags & (RPENC_Z | RPENC_B64)) == 0) {
        // By default compress and encode the string.
        flags |= RPENC_Z | RPENC_B64;
    }
    if (outData.append(buf.bytes(), buf.size()) != (int)buf.size()) {
        status.addError("can't append %lu bytes", buf.size());
        return false;
    }
    if (!outData.encode(status, flags)) {
        return false;
    }
    buf.clear();
    if ((flags & RPENC_RAW) == 0) {
        switch (flags & (RPENC_Z | RPENC_B64)) {
        case RPENC_Z:
            buf.append("@@RP-ENC:z\n", 11);
            break;
        case RPENC_B64:
            buf.append("@@RP-ENC:b64\n", 13);
            break;
        case (RPENC_B64 | RPENC_Z):
            buf.append("@@RP-ENC:zb64\n", 14);
            break;
        default:
            break;
        }
    }
    if (buf.append(outData.bytes(),outData.size()) != (int)outData.size()) {
        status.addError("can't append %d bytes", outData.size());
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
 * Rappture::encoding::decode(context, buf,flags)
 *
 * The check header flag is confusing here.
 */

bool
Rappture::encoding::decode(Rappture::Outcome &status, Rappture::Buffer& buf,
                           unsigned int flags)
{
    Rappture::Buffer outData;

    const char *bytes;

    size_t size;
    size = buf.size();
    if (size == 0) {
        return true;                // Nothing to decode.
    }
    bytes = buf.bytes();
    if ((flags & RPENC_RAW) == 0) {
        unsigned int headerFlags = 0;
        if ((size > 11) && (strncmp(bytes, "@@RP-ENC:z\n", 11) == 0)) {
            bytes += 11;
            size -= 11;
            headerFlags |= RPENC_Z;
        } else if ((size > 13) && (strncmp(bytes, "@@RP-ENC:b64\n", 13) == 0)){
            bytes += 13;
            size -= 13;
            headerFlags |= RPENC_B64;
        } else if ((size > 14) && (strncmp(bytes, "@@RP-ENC:zb64\n", 14) == 0)){
            bytes += 14;
            size -= 14;
            headerFlags |= (RPENC_B64 | RPENC_Z);
        } 
         if (headerFlags != 0) {
            unsigned int reqFlags;

            reqFlags = flags & (RPENC_B64 | RPENC_Z);
            /* 
             * If there's a header and the programmer also requested decoding
             * flags, verify that the two are the same.  We don't want to
             * penalize the programmer for over-specifying.  But we need to
             * catch cases when they don't match.  If you really want to
             * override the header, you should also specify the RPENC_RAW flag
             * (-noheader).
             */
            if ((reqFlags != 0) && (reqFlags != headerFlags)) {
                status.addError("decode flags don't match the header");
                return false;
            }
            flags |= headerFlags;
        }
    }
    if ((flags & (RPENC_B64 | RPENC_Z)) == 0) {
        return true;                /* No decode or decompress flags present. */
    }
    if (outData.append(bytes, size) != (int)size) {
        status.addError("can't append %d bytes to buffer", size);
        return false;
    }
    if (!outData.decode(status, flags)) {
        return false;
    }
    buf.move(outData);
    return true;
}

