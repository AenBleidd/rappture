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
#include <tcl.h>
#include "RpEncode.h"
#include "RpEncodeTclInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

static int RpTclEncodingIs      _ANSI_ARGS_((   ClientData cdata,
                                                Tcl_Interp *interp,
                                                int objc,
                                                Tcl_Obj *const objv[]    ));

static int RpTclEncodingEncode  _ANSI_ARGS_((   ClientData cdata,
                                                Tcl_Interp *interp,
                                                int objc,
                                                Tcl_Obj *const objv[]    ));

static int RpTclEncodingDecode  _ANSI_ARGS_((   ClientData cdata,
                                                Tcl_Interp *interp,
                                                int objc,
                                                Tcl_Obj *const objv[]    ));

#ifdef __cplusplus
}
#endif


/**********************************************************************/
// FUNCTION: RapptureEncoding_Init()
/// Initializes the Rappture Encoding module and commands defined below
/**
 * Called in RapptureGUI_Init() to initialize the Rappture Units module.
 * Initialized commands include:
 * ::Rappture::encoding::is
 * ::Rappture::encoding::encode
 * ::Rappture::encoding::decode
 */

int
RapptureEncoding_Init(Tcl_Interp *interp)
{

    Tcl_CreateObjCommand(interp, "::Rappture::encoding::is",
        RpTclEncodingIs, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateObjCommand(interp, "::Rappture::encoding::encode",
        RpTclEncodingEncode, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateObjCommand(interp, "::Rappture::encoding::decode",
        RpTclEncodingDecode, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclEncodingIs()
/// Rappture::encoding::is checks to see if given string is binary.
/**
 * Checks to see <string> is binary
 * Full function call:
 * ::Rappture::encoding::is binary <string>
 *
 */

int
RpTclEncodingIs (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const objv[])
{
    Tcl_ResetResult(interp);

    // parse through command line options
    if (objc != 3) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"",
            Tcl_GetString(objv[0])," binary <string>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    const char *type = Tcl_GetString(objv[1]);

    int bufLen;
    const char *buf = (const char*) Tcl_GetByteArrayFromObj(objv[2],&bufLen);

    if (strcmp(type,"binary") == 0) {
        if (Rappture::encoding::isbinary(buf,bufLen) != 0) {
            // non-ascii character found, return yes
            Tcl_AppendResult(interp, "yes", (char*)NULL);
        } else {
            Tcl_AppendResult(interp, "no",(char*)NULL);
        }
        return TCL_OK;
    }
    Tcl_AppendResult(interp, "bad option \"", type,
            "\": should be binary",
            (char*)NULL);
    return TCL_ERROR;
}

/**********************************************************************/
// FUNCTION: RpTclEncodingEncode()
/// Rappture::encoding::encode function in Tcl, encodes provided string
/**
 * Encode a string by compressing it with zlib and then base64 encoding it.
 * If binary data is provided, the data is compressed and base64 encoded.
 * RpTclEncodingIs is used to qualify binary data.
 *
 * If the -as option is not set, zlib compression and base64 encoding 
 * is performed by default
 *
 * Full function call:
 * ::Rappture::encoding::encode ?-as z|b64|zb64? ?-no-header? <string>
 */

int
RpTclEncodingEncode (   ClientData cdata,
                        Tcl_Interp *interp,
                        int objc,
                        Tcl_Obj *const objv[]  )
{

    const char* encodeType    = NULL; // name of the units provided by user
    const char* option        = NULL;
    const char* cmdName       = NULL;
    Rappture::Buffer buf; // name of the units provided by user
    Rappture::Outcome err;

    int optionLen             = 0;
    int typeLen               = 0;
    int nextarg               = 0; // start parsing using the '2'th argument

    int compress              = 1;
    int base64                = 1;
    int addHeader             = 1;
    int flags                 = 0;

    Tcl_Obj *result           = NULL;

    Tcl_ResetResult(interp);

    cmdName = Tcl_GetString(objv[nextarg++]);

    // parse through command line options
    if ((objc <= 2) && (objc >= 5)) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? ?-no-header? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    while ((objc - nextarg) > 0) {
        option = Tcl_GetStringFromObj(objv[nextarg], &optionLen);
        if (*option == '-') {
            if ( strncmp(option,"-as",optionLen) == 0 ) {
                nextarg++;
                typeLen = 0;
                if (nextarg < objc) {
                    encodeType = Tcl_GetStringFromObj(objv[nextarg],&typeLen);
                    nextarg++;
                }
                if (        (typeLen == 1) &&
                            (strncmp(encodeType,"z",typeLen) == 0) ) {
                    compress = 1;
                    base64 = 0;
                }
                else if (   (typeLen == 3) &&
                            (strncmp(encodeType,"b64",typeLen) == 0) ) {
                    compress = 0;
                    base64 = 1;
                }
                else if (   (typeLen == 4) &&
                            (strncmp(encodeType,"zb64",typeLen) == 0) ) {
                    compress = 1;
                    base64 = 1;
                }
                else {
                    // user did not specify recognized wishes for this option,
                    Tcl_AppendResult(interp, "bad value \"",(char*)NULL);
                    if (encodeType != NULL) {
                        Tcl_AppendResult(interp, encodeType,(char*)NULL);
                    }
                    Tcl_AppendResult(interp,
                            "\": should be one of z, b64, zb64",
                            (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else if ( strncmp(option,"-no-header",optionLen) == 0 ) {
                nextarg++;
                addHeader = 0;
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }

    if ((objc - nextarg) != 1) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? ?-no-header? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    option = (const char*) Tcl_GetByteArrayFromObj(objv[nextarg++], &optionLen);
    buf = Rappture::Buffer(option,optionLen);

    if (compress == 1) {
        flags = flags | RPENC_Z;
    }
    if (base64 == 1) {
        flags = flags | RPENC_B64;
    }
    if (addHeader == 1) {
        flags = flags | RPENC_HDR;
    }

    err &= Rappture::encoding::encode(buf,flags);

    if (!err) {
        result = Tcl_NewByteArrayObj(
            (const unsigned char*)buf.bytes(), buf.size());
        Tcl_SetObjResult(interp, result);
    }
    else {
        result = Tcl_GetObjResult(interp);
        Tcl_AppendStringsToObj( result, err.remark().c_str(),
                                "\n", err.context().c_str(), NULL   );
    }
    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclEncodingDecode()
/// Rappture::encoding::decode function in Tcl, decodes provided string
/**
 * Decode a string by uncompressing it with zlib and base64 decoding it.
 * If binary data is provided, the data is base64 decoded and uncompressed.
 * RpTclEncodingIs is used to qualify binary data.
 *
 * Full function call:
 * ::Rappture::encoding::decode ?-as z|b64|zb64? <string>
 */

int
RpTclEncodingDecode (   ClientData cdata,
                        Tcl_Interp *interp,
                        int objc,
                        Tcl_Obj *const objv[]  )
{

    const char* encodeType    = NULL; // name of the units provided by user
    const char* option        = NULL;
    const char* cmdName       = NULL;
    Rappture::Buffer buf      = ""; // name of the units provided by user
    Rappture::Outcome err;

    int optionLen             = 0;
    int typeLen               = 0;
    int nextarg               = 0; // start parsing using the '2'th argument

    int decompress            = 0;
    int base64                = 0;
    int flags                 = 0;

    Tcl_Obj *result           = NULL;

    Tcl_ResetResult(interp);

    cmdName = Tcl_GetString(objv[nextarg++]);

    // parse through command line options
    if ((objc != 2) && (objc != 4)) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    option = Tcl_GetStringFromObj(objv[nextarg], &optionLen);
    if (*option == '-') {
        if ( strncmp(option,"-as",optionLen) == 0 ) {
            nextarg++;
            typeLen = 0;
            if (nextarg < objc) {
                encodeType = Tcl_GetStringFromObj(objv[nextarg],&typeLen);
                nextarg++;
            }
            if (        (typeLen == 1) &&
                        (strncmp(encodeType,"z",typeLen) == 0) ) {
                decompress = 1;
                base64 = 0;
            }
            else if (   (typeLen == 3) &&
                        (strncmp(encodeType,"b64",typeLen) == 0) ) {
                decompress = 0;
                base64 = 1;
            }
            else if (   (typeLen == 4) &&
                        (strncmp(encodeType,"zb64",typeLen) == 0) ) {
                decompress = 1;
                base64 = 1;
            }
            else {
                // user did not specify recognized wishes for this option,
                Tcl_AppendResult(interp, "bad value \"",(char*)NULL);
                if (encodeType != NULL) {
                    Tcl_AppendResult(interp, encodeType,(char*)NULL);
                }
                Tcl_AppendResult(interp,
                        "\": should be one of z, b64, zb64",
                        (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    if ((objc - nextarg) != 1) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    option = (const char*) Tcl_GetByteArrayFromObj(objv[nextarg++], &optionLen);

    buf = Rappture::Buffer(option,optionLen);

    if (decompress == 1) {
        flags = flags | RPENC_Z;
    }
    if (base64 == 1) {
        flags = flags | RPENC_B64;
    }

    err &= Rappture::encoding::decode(buf,flags);

    if (!err) {
        result = Tcl_NewByteArrayObj(
            (const unsigned char*)buf.bytes(), buf.size());
        Tcl_SetObjResult(interp, result);
    }
    else {
        result = Tcl_GetObjResult(interp);
        Tcl_AppendStringsToObj( result, err.remark().c_str(),
                                "\n", err.context().c_str(), NULL   );
    }
    return TCL_OK;
}
