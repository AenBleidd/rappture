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

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

using namespace Rappture::encoding;

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
isbinary(const char* buf, int size)
{
    if (buf == NULL) {
        return 0;
    }

    if (size < 0) {
        size = strlen(buf);
    }

    for (int index = 0; index < size; index++) {
        int c = *buf++;
        if (((c >= '\000') && (c <= '\010')) ||
            ((c >= '\013') && (c <= '\014')) ||
            ((c >= '\016') && (c <= '\037')) ||
            ((c >= '\177') && (c <= '\377')) ) {
            // data is binary
            return index+1;
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
isencoded(const char* buf, int size)
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

Rappture::Outcome
encode (Rappture::Buffer& buf, size_t flags)
{

    int compress              = 0;
    int base64                = 0;
    int addHeader             = 0;
    Rappture::Outcome err;
    Rappture::Buffer outData;

    if ((flags & RPENC_Z) == RPENC_Z ) {
        compress = 1;
    }
    if ((flags & RPENC_B64) == RPENC_B64 ) {
        base64 = 1;
    }
    if ((flags & RPENC_HDR) == RPENC_HDR ) {
        addHeader = 1;
    }

    outData.append(buf.bytes(),buf.size());
    err = outData.encode(compress,base64);
    if (!err) {
        buf.clear();
        if (addHeader == 1) {
            if ((compress == 1) && (base64 == 0)) {
                buf.append("@@RP-ENC:z\n",11);
            }
            else if ((compress == 0) && (base64 == 1)) {
                buf.append("@@RP-ENC:b64\n",13);
            }
            else if ((compress == 1) && (base64 == 1)) {
                buf.append("@@RP-ENC:zb64\n",14);
            }
            else {
                // do nothing
            }
        }
        else {
            // do nothing
        }
        buf.append(outData.bytes(),outData.size());
    }

    return err;
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

Rappture::Outcome
decode (Rappture::Buffer& buf, size_t flags)
{

    int decompress            = 0;
    int base64                = 0;
    int checkHDR              = 0;
    Rappture::Outcome err;
    Rappture::Buffer outData;

    if ((flags & RPENC_Z) == RPENC_Z ) {
        decompress = 1;
    }
    if ((flags & RPENC_B64) == RPENC_B64 ) {
        base64 = 1;
    }
    if ((flags & RPENC_HDR) == RPENC_HDR ) {
        checkHDR = 1;
    }

    if ((buf.size() > 11) && (strncmp(buf.bytes(),"@@RP-ENC:z\n",11) == 0)) {
        outData.append(buf.bytes()+11,buf.size()-11);
        if ( (checkHDR == 1) || ( (decompress == 0) && (base64 == 0) ) ) {
            decompress = 1;
            base64 = 0;
        }
    }
    else if ((buf.size() > 13) && (strncmp(buf.bytes(),"@@RP-ENC:b64\n",13) == 0)) {
        outData.append(buf.bytes()+13,buf.size()-13);
        if ( (checkHDR == 1) || ( (decompress == 0) && (base64 == 0) ) ) {
            decompress = 0;
            base64 = 1;
        }
    }
    else if ((buf.size() > 14) && (strncmp(buf.bytes(),"@@RP-ENC:zb64\n",14) == 0)) {
        outData.append(buf.bytes()+14,buf.size()-14);
        if ( (checkHDR == 1) || ( (decompress == 0) && (base64 == 0) ) ) {
            decompress = 1;
            base64 = 1;
        }
    }
    else {
        // no special recognized tags
        outData.append(buf.bytes(),buf.size());
    }

    err = outData.decode(decompress,base64);
    if (!err) {
        buf.move(outData);
    }

    return err;
}

#ifdef __cplusplus
}
#endif // ifdef __cplusplus
