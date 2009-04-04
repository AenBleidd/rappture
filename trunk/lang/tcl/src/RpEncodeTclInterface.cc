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

extern "C" Tcl_AppInitProc RpEncoding_Init;

static Tcl_ObjCmdProc RpTclEncodingIs;
static Tcl_ObjCmdProc RpTclEncodingEncode;
static Tcl_ObjCmdProc RpTclEncodingDecode;

/**********************************************************************/
// FUNCTION: RpEncoding_Init()
/// Initializes the Rappture Encoding module and commands defined below
/**
 * Called in RapptureGUI_Init() to initialize the Rappture Units module.
 * Initialized commands include:
 * ::Rappture::encoding::is
 * ::Rappture::encoding::encode
 * ::Rappture::encoding::decode
 */

int
RpEncoding_Init(Tcl_Interp *interp)
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

static int
RpTclEncodingIs (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const objv[])
{
    Tcl_ResetResult(interp);

    // parse through command line options
    if (objc != 3) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"",
            Tcl_GetString(objv[0])," binary|encoded <string>\"",
            (char*)NULL);
        return TCL_ERROR;
    }


    int bufLen;
    const char *buf;
    const char *string;
    buf = (const char*) Tcl_GetByteArrayFromObj(objv[2], &bufLen);
    const char *type = Tcl_GetString(objv[1]);
    if (('b' == *type) && (strcmp(type,"binary") == 0)) {
	bool isBinary;

        isBinary = (Rappture::encoding::isbinary(buf,bufLen) != 0);
	string = (isBinary) ? "yes" : "no" ;
    } else if (('e' == *type) && (strcmp(type,"encoded") == 0)) {
	bool isEncoded;

        isEncoded = (Rappture::encoding::isencoded(buf, bufLen) != 0);
	string = (isEncoded) ? "yes" : "no" ;
    } else {
	Tcl_AppendResult(interp, "bad option \"", type, 
		"\": should be binary or encoded", (char*)NULL);
	return TCL_ERROR;
    }
    Tcl_SetResult(interp, (char *)string, TCL_STATIC);
    return TCL_OK;
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

static int
RpTclEncodingEncode (ClientData cdata, Tcl_Interp *interp, int objc,
		     Tcl_Obj *const *objv)
{
    const char* encodeType    = NULL; // name of the units provided by user
    const char* cmdName       = NULL;
    Rappture::Outcome err;

    int typeLen               = 0;
    int nextarg               = 0; // start parsing using the '2'th argument

    bool compress, base64, addHeader;

    compress = base64 = addHeader = true;

    Tcl_Obj *result           = NULL;

    Tcl_ResetResult(interp);

    cmdName = Tcl_GetString(objv[nextarg++]);

    // parse through command line options
    if (objc < 1) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? ?-no-header? ?--? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    while ((objc - nextarg) > 0) {
	const char *option;
	int optionLen;

        option = Tcl_GetStringFromObj(objv[nextarg], &optionLen);
        if (*option == '-') {
            if (strncmp(option,"-as", optionLen) == 0 ) {
                nextarg++;
                typeLen = 0;
                if (nextarg < objc) {
                    encodeType = Tcl_GetStringFromObj(objv[nextarg],&typeLen);
                    nextarg++;
                }
                if ((typeLen == 1) && (strncmp(encodeType,"z",typeLen) == 0) ) {
                    compress = true;
                    base64 = false;
                } else if ((typeLen == 3) && 
			 (strncmp(encodeType,"b64",typeLen) == 0) ) {
                    compress = false;
                    base64 = true;
                } else if ((typeLen == 4) &&
			 (strncmp(encodeType,"zb64",typeLen) == 0) ) {
                    compress = true;
                    base64 = true;
                } else {
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
            } else if ( strncmp(option,"-no-header", optionLen) == 0 ) {
                nextarg++;
                addHeader = false;
            } else if ( strcmp(option,"--") == 0 ) {
                nextarg++;
                break;
            } else {
                Tcl_AppendResult(interp,
                    "bad option \"", option,
                    "\": should be -as, -no-header, --", (char*)NULL);
                return TCL_ERROR;
            }
        } else {
            break;
        }
    }

    if ((objc - nextarg) != 1) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? ?-no-header? ?--? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    int nBytes;
    const char* string;
    string = (const char*)Tcl_GetByteArrayFromObj(objv[nextarg++], &nBytes);
    if (nBytes <= 0) {
	return TCL_OK;		// Nothing to encode.
    }
    Rappture::Buffer buf(string, nBytes);
    unsigned int flags = 0;
    if (compress) {
        flags |= RPENC_Z;
    }
    if (base64) {
        flags |= RPENC_B64;
    }
    if (addHeader) {
        flags |= RPENC_HDR;
    }
    if (!Rappture::encoding::encode(err, buf, flags)) {
        Tcl_AppendResult(interp, err.remark(), "\n", err.context(), NULL);
	return TCL_ERROR;
    }
    result = Tcl_NewByteArrayObj((const unsigned char*)buf.bytes(), buf.size());
    Tcl_SetObjResult(interp, result);
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

static int
RpTclEncodingDecode (   ClientData cdata,
                        Tcl_Interp *interp,
                        int objc,
                        Tcl_Obj *const objv[]  )
{

    const char* encodeType    = NULL; // name of the units provided by user
    const char* cmdName       = NULL;
    Rappture::Outcome err;

    int typeLen               = 0;
    int nextarg               = 0; // start parsing using the '2'th argument

    bool decompress, base64;
    decompress = base64 = false;

    cmdName = Tcl_GetString(objv[nextarg++]);

    // parse through command line options
    if (objc < 1) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? ?--? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    while ((objc - nextarg) > 0) {
	const char *option;
	int optionLen;

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
                    decompress = true;
                    base64 = false;
                }
                else if (   (typeLen == 3) &&
                            (strncmp(encodeType,"b64",typeLen) == 0) ) {
                    decompress = false;
                    base64 = true;
                }
                else if (   (typeLen == 4) &&
                            (strncmp(encodeType,"zb64",typeLen) == 0) ) {
                    decompress = true;
                    base64 = true;
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
            } else if ( strcmp(option,"--") == 0 ) {
                nextarg++;
                break;
            } else {
                Tcl_AppendResult(interp,
                    "bad option \"", option,
                    "\": should be -as, --", (char*)NULL);
                return TCL_ERROR;
            }
        } else {
            break;
        }
    }

    if ((objc - nextarg) != 1) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? ?--? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    int optionLen;
    const char* option;
    option = (const char*) Tcl_GetByteArrayFromObj(objv[nextarg++], &optionLen);

    unsigned int flags = 0;
    if (decompress) {
        flags |= RPENC_Z;
    }
    if (base64) {
        flags |= RPENC_B64;
    }
    Rappture::Buffer buf(option, optionLen); 
    if (!Rappture::encoding::decode(err, buf,flags)) {
        Tcl_AppendResult(interp, err.remark(), "\n", err.context(), NULL);
	return TCL_ERROR;
    }
    Tcl_Obj *objPtr;
    objPtr = Tcl_NewByteArrayObj((const unsigned char*)buf.bytes(), buf.size());
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}
