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
extern "C" {
#include "Switch.h"
extern Tcl_AppInitProc RpEncoding_Init;
}

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

        isEncoded = (Rappture::encoding::headerFlags(buf, bufLen) != 0);
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

/*
 *---------------------------------------------------------------------------
 *
 * AsSwitch --
 *
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AsSwitch(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    int *flagsPtr = (int *)(record + offset);
    char c;
    char *string;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'b') && (strcmp(string, "b64") == 0)) {
	*flagsPtr = RPENC_B64;
    } else if ((c == 'z') && (strcmp(string, "zb64") == 0)) {
	*flagsPtr = RPENC_Z  | RPENC_B64;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
	*flagsPtr = RPENC_Z;
    } else {
	Tcl_AppendResult(interp, "bad order \"", string, 
		 "\": should be breadthfirst, inorder, preorder, or postorder",
		 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static SwitchParseProc AsSwitch;
static SwitchCustom asSwitch = {
    AsSwitch, NULL, 0,
};

typedef struct {
    unsigned int flags;
    unsigned int noheader;
} EncodeSwitches;

static SwitchSpec encodeSwitches[] = 
{
    {SWITCH_CUSTOM, "-as", "z|b64|zb64",
	offsetof(EncodeSwitches, flags), 0, 0, &asSwitch},
    {SWITCH_VALUE, "-no-header", "", 
	offsetof(EncodeSwitches, noheader), 0, true},
    {SWITCH_END}
};

static int
RpTclEncodingEncode (ClientData cdata, Tcl_Interp *interp, int objc,
		     Tcl_Obj *const *objv)
{
    if (objc < 1) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), 
		" ?-as z|b64|zb64? ?-no-header? ?--? string\"", (char*)NULL);
        return TCL_ERROR;
    }
    EncodeSwitches switches;
    switches.flags = 0;
    switches.noheader = 0;
    int n;
    n = Rp_ParseSwitches(interp, encodeSwitches, objc - 1, objv + 1, &switches,
			 SWITCH_OBJV_PARTIAL);
    if (n < 0) {
	return TCL_ERROR;
    }
    int last;
    last = n + 1;
    if ((objc - last) != 1) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), 
		" ?-as z|b64|zb64? ?-no-header? ?--? string\"", (char*)NULL);
        return TCL_ERROR;
    }
    int nBytes;
    const char* string;
    string = (const char*)Tcl_GetByteArrayFromObj(objv[last], &nBytes);
    if (nBytes <= 0) {
	return TCL_OK;		// Nothing to encode.
    }
    Rappture::Buffer buf(string, nBytes);
    if (!switches.noheader) {
        switches.flags |= RPENC_HDR;
    }
    Rappture::Outcome status;
    if (!Rappture::encoding::encode(status, buf, switches.flags)) {
        Tcl_AppendResult(interp, status.remark(), "\n", status.context(), NULL);
	return TCL_ERROR;
    }
    Tcl_SetByteArrayObj(Tcl_GetObjResult(interp), 
		(const unsigned char*)buf.bytes(), buf.size());
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

typedef struct {
    unsigned int flags;
} DecodeSwitches;

static SwitchSpec decodeSwitches[] = 
{
    {SWITCH_CUSTOM, "-as", "z|b64|zb64",
	offsetof(DecodeSwitches, flags), 0, 0, &asSwitch},
    {SWITCH_END}
};

static int
RpTclEncodingDecode(ClientData clientData, Tcl_Interp *interp, int objc,
		    Tcl_Obj *const *objv)
{
    if (objc < 1) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]),
                " ?-as z|b64|zb64? ?--? <string>\"", (char*)NULL);
        return TCL_ERROR;
    }

    DecodeSwitches switches;
    switches.flags = 0;
    int n;
    n = Rp_ParseSwitches(interp, decodeSwitches, objc - 1, objv + 1, &switches,
			 SWITCH_OBJV_PARTIAL);
    if (n < 0) {
	return TCL_ERROR;
    }
    int last;
    last = n + 1;
    if ((objc - last) != 1) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), 
		" ?-as z|b64|zb64? ?--? string\"", (char*)NULL);
        return TCL_ERROR;
    }
    int nBytes;
    const char* string;
    string = (const char*)Tcl_GetByteArrayFromObj(objv[last], &nBytes);
    if (nBytes <= 0) {
	return TCL_OK;		// Nothing to decode.
    }
    Rappture::Buffer buf(string, nBytes); 
    Rappture::Outcome status;
    if (!Rappture::encoding::decode(status, buf, switches.flags)) {
        Tcl_AppendResult(interp, status.remark(), "\n", status.context(), NULL);
	return TCL_ERROR;
    }
    Tcl_SetByteArrayObj(Tcl_GetObjResult(interp), 
		(const unsigned char*)buf.bytes(), buf.size());
    return TCL_OK;
}
