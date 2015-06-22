/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Buffer Interface Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpBuffer.h"
#include "RpBufferCInterface.h"
#include "RpOutcomeCHelper.h"

#ifdef __cplusplus
extern "C" {
#endif

int
RapptureBufferInit(RapptureBuffer* buf)
{
    if (buf == NULL) {
        return -1;
    }

    buf->_buf   = NULL;

    return 0;
}

int
RapptureBufferNew(RapptureBuffer* buf)
{
    if (buf == NULL) {
        return -1;
    }

    RapptureBufferFree(buf);
    buf->_buf   = (void*) new Rappture::Buffer();

    return 0;
}

int
RapptureBufferFree(RapptureBuffer* buf)
{
    if (buf->_buf != NULL) {
        delete ((Rappture::Buffer*)buf->_buf);
        buf->_buf = NULL;
    }

    return 0;
}

int
RpBufferToCBuffer(Rappture::Buffer* rpbuf, RapptureBuffer* buf)
{
    if (rpbuf == NULL) {
        return -1;
    }

    if (buf == NULL) {
        return -1;
    }

    RapptureBufferFree(buf);
    buf->_buf   = (void*) new Rappture::Buffer(*rpbuf);

    return 0;
}

const char*
RapptureBufferBytes(RapptureBuffer* buf)
{
    const char* retVal = NULL;

    if (buf == NULL) {
        return retVal;
    }

    if (buf->_buf == NULL) {
        return retVal;
    }

    return ((Rappture::Buffer*)buf->_buf)->bytes();
}

unsigned int
RapptureBufferSize(RapptureBuffer* buf)
{
    unsigned int retVal = 1;

    if (buf == NULL) {
        return retVal;
    }

    if (buf->_buf == NULL) {
        return retVal;
    }

    return ((Rappture::Buffer*)buf->_buf)->size();
}

int
RapptureBufferAppend(RapptureBuffer* buf, const char* bytes, int size)
{
    int retVal = 1;

    if (buf == NULL) {
        return retVal;
    }

    if (buf->_buf == NULL) {
        return retVal;
    }

    return ((Rappture::Buffer*)buf->_buf)->append(bytes,size);
}

int
RapptureBufferRead(RapptureBuffer* buf, const char* bytes, int size)
{
    int retVal = 1;

    if (buf == NULL) {
        return retVal;
    }

    if (buf->_buf == NULL) {
        return retVal;
    }

    return ((Rappture::Buffer*)buf->_buf)->read(bytes,size);
}

int
RapptureBufferSeek(RapptureBuffer* buf, int offset, int whence)
{
    int retVal = 1;

    if (buf == NULL) {
        return retVal;
    }

    if (buf->_buf == NULL) {
        return retVal;
    }

    return ((Rappture::Buffer*)buf->_buf)->seek(offset,whence);
}

int
RapptureBufferTell(RapptureBuffer* buf)
{
    int retVal = 1;

    if (buf == NULL) {
        return retVal;
    }

    if (buf->_buf == NULL) {
        return retVal;
    }

    return ((Rappture::Buffer*)buf->_buf)->tell();
}

RapptureOutcome
RapptureBufferLoad(RapptureBuffer* buf, const char* filename)
{
    Rappture::Outcome s;
    RapptureOutcome status;

    RapptureOutcomeInit(&status);

    if (buf == NULL) {
        s.error("invalid parameter: buf == NULL");
        s.addContext("while in RapptureBufferLoad()");
        RpOutcomeToCOutcome(&s,&status);
        return status;
    }

    if (buf->_buf == NULL) {
        s.error("uninitialized parameter: buf, did you call RapptureBufferInit()?");
        s.addContext("while in RapptureBufferLoad()");
        RpOutcomeToCOutcome(&s,&status);
        return status;
    }

    ((Rappture::Buffer*)buf->_buf)->load(s, filename);
    RpOutcomeToCOutcome(&s,&status);
    return status;
}

RapptureOutcome
RapptureBufferDump(RapptureBuffer* buf, const char* filename)
{
    Rappture::Outcome s;
    RapptureOutcome status;

    s.addContext("while in RapptureBufferLoad()");
    RapptureOutcomeInit(&status);

    if (buf == NULL) {
        s.error("invalid parameter: buf == NULL");
        RpOutcomeToCOutcome(&s,&status);
        return status;
    }

    if (buf->_buf == NULL) {
        s.error("uninitialized parameter: buf, did you call RapptureBufferInit()?");
        RpOutcomeToCOutcome(&s,&status);
        return status;
    }

    ((Rappture::Buffer*)buf->_buf)->dump(s, filename);
    RpOutcomeToCOutcome(&s,&status);
    return status;
}

RapptureOutcome
RapptureBufferEncode(RapptureBuffer* buf, int compress, int base64)
{
    Rappture::Outcome s;
    RapptureOutcome status;

    RapptureOutcomeInit(&status);
    s.addContext("while in RapptureBufferLoad()");
    if (buf == NULL) {
        s.error("invalid parameter: buf == NULL");
        RpOutcomeToCOutcome(&s,&status);
        return status;
    }

    if (buf->_buf == NULL) {
        s.error("uninitialized parameter: buf, did you call RapptureBufferInit()?");
        RpOutcomeToCOutcome(&s, &status);
        return status;
    }
    unsigned int flags;
    flags = RPENC_HDR;
    if (compress) {
	flags |= RPENC_Z;
    }
    if (base64) {
	flags |= RPENC_B64;
    }
    ((Rappture::Buffer*)buf->_buf)->encode(s, flags);
    RpOutcomeToCOutcome(&s,&status);
    return status;
}

RapptureOutcome
RapptureBufferDecode(RapptureBuffer* buf, int decompress, int base64)
{
    Rappture::Outcome s;
    RapptureOutcome status;

    RapptureOutcomeInit(&status);

    if (buf == NULL) {
        s.error("invalid parameter: buf == NULL");
        s.addContext("while in RapptureBufferLoad()");
        RpOutcomeToCOutcome(&s,&status);
        return status;
    }

    if (buf->_buf == NULL) {
        s.error("uninitialized parameter: buf, did you call RapptureBufferInit()?");
        s.addContext("while in RapptureBufferLoad()");
        RpOutcomeToCOutcome(&s,&status);
        return status;
    }
    unsigned int flags;
    flags = 0;
    if (decompress) {
	flags |= RPENC_Z;
    }
    if (base64) {
	flags |= RPENC_B64;
    }
    ((Rappture::Buffer*)buf->_buf)->decode(s, flags);
    RpOutcomeToCOutcome(&s,&status);
    return status;
}

#ifdef __cplusplus
}
#endif // ifdef __cplusplus
