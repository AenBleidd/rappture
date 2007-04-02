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
#include "RpBuffer.h"
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

int isbinary(const char* buf, int size);

int isbinary(const char* buf, int size)
{
    int index = 0;

    for (index = 0; index < size; index++) {
        if (((buf[index] >= '\000') && (buf[index] <= '\010')) ||
            ((buf[index] >= '\013') && (buf[index] <= '\014')) ||
            ((buf[index] >= '\016') && (buf[index] <= '\037')) ||
            ((buf[index] >= '\177') && (buf[index] <= '\377')) ) {
            // data is binary
            return index+1;
        }

    }
    return 0;
}





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
RpTclEncodingIs     (   ClientData cdata,
                        Tcl_Interp *interp,
                        int objc,
                        Tcl_Obj *const objv[]  )
{
    const char* type     = NULL; // type of data being checked
    const char* buf      = NULL; // buffer to be checked
    const char* cmdName  = NULL;

    int typeLen          = 0;
    int bufLen           = 0; // length of user provided buffer
    int nextarg          = 0;  // next argument

    Tcl_ResetResult(interp);

    cmdName = Tcl_GetString(objv[nextarg++]);

    // parse through command line options
    if (objc != 3) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"",
            cmdName," binary <string>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    type = Tcl_GetStringFromObj(objv[nextarg++],&typeLen);
    buf = Tcl_GetStringFromObj(objv[nextarg++],&bufLen);

    if (strncmp(type,"binary",typeLen) == 0) {
        if (isbinary(buf,bufLen) != 0) {
            // non-ascii character found, return yes
            Tcl_AppendResult(interp, "yes",(char*)NULL);
            return TCL_OK;
        }
    }
    else {
        Tcl_AppendResult(interp, "bad option \"", type,
                "\": should be binary",
                (char*)NULL);
        return TCL_ERROR;

    }

    // default return
    // no binary characters found, return no
    Tcl_AppendResult(interp, "no",(char*)NULL);
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
 * ::Rappture::encoding::encode ?-as z|b64|zb64? <string>
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

    int optionLen             = 0;
    int typeLen               = 0;
    int nextarg               = 0; // start parsing using the '2'th argument

    int compress              = 1;
    int base64                = 1;

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
    }

    if ((objc - nextarg) != 1) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", cmdName,
                " ?-as z|b64|zb64? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    option = (const char*) Tcl_GetByteArrayFromObj(objv[nextarg++], &optionLen);

    if (strncmp(option,"@@RP-ENC:z\n",11) == 0) {
        buf = Rappture::Buffer(option+11,optionLen-11);
        buf.decode(1,0);
    }
    else if (strncmp(option,"@@RP-ENC:b64\n",13) == 0) {
        buf = Rappture::Buffer(option+13,optionLen-13);
        buf.decode(0,1);
    }
    else if (strncmp(option,"@@RP-ENC:zb64\n",14) == 0) {
        buf = Rappture::Buffer(option+14,optionLen-14);
        buf.decode(1,1);
    }
    else {
        // no special recognized tags
        buf = Rappture::Buffer(option,optionLen);
    }

    buf.encode(compress,base64);
    result = Tcl_GetObjResult(interp);

    if ((compress == 1) && (base64 == 0)) {
        Tcl_AppendToObj(result,"@@RP-ENC:z\n",11);
    }
    else if ((compress == 0) && (base64 == 1)) {
        Tcl_AppendToObj(result,"@@RP-ENC:b64\n",13);
    }
    else if ((compress == 1) && (base64 == 1)) {
        Tcl_AppendToObj(result,"@@RP-ENC:zb64\n",14);
    }
    else {
        // do nothing
    }

    Tcl_AppendToObj(result,buf.bytes(),buf.size());

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

    int optionLen             = 0;
    int typeLen               = 0;
    int nextarg               = 0; // start parsing using the '2'th argument

    int decompress            = 0;
    int base64                = 0;

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

    if (strncmp(option,"@@RP-ENC:z\n",11) == 0) {
        buf = Rappture::Buffer(option+11,optionLen-11);
        if ( (decompress == 0) && (base64 == 0) ) {
            decompress = 1;
            base64 = 0;
        }
    }
    else if (strncmp(option,"@@RP-ENC:b64\n",13) == 0) {
        buf = Rappture::Buffer(option+13,optionLen-13);
        if ( (decompress == 0) && (base64 == 0) ) {
            decompress = 0;
            base64 = 1;
        }
    }
    else if (strncmp(option,"@@RP-ENC:zb64\n",14) == 0) {
        buf = Rappture::Buffer(option+14,optionLen-14);
        if ( (decompress == 0) && (base64 == 0) ) {
            decompress = 1;
            base64 = 1;
        }
    }
    else {
        // no special recognized tags
        buf = Rappture::Buffer(option,optionLen);
    }

    buf.decode(decompress,base64);
    result = Tcl_GetObjResult(interp);

    Tcl_AppendToObj(result,buf.bytes(),buf.size());

    return TCL_OK;
}
