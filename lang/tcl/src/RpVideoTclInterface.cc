/*
 * ----------------------------------------------------------------------
 *  Rappture::MediaPlayer
 *
 *  This is an interface to the rappture movieplayer module.
 *  It allows you to grab image frames from mpeg movies using ffmpeg.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2010  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include <string.h>
#include "RpVideo.h"

extern "C" Tcl_AppInitProc RpVideo_Init;

#include "RpOp.h"

static Tcl_ObjCmdProc VideoCmd;
static Tcl_ObjCmdProc VideoCallCmd;
static Tcl_ObjCmdProc GetOp;
static Tcl_ObjCmdProc NextOp;
static Tcl_ObjCmdProc SeekOp;
static Tcl_ObjCmdProc ReleaseOp;

static Rp_OpSpec rpVideoOps[] = {
    {"get",  1, (void *)GetOp, 3, 5, "[image width height]|[position cur|end]|[framerate]",},
    {"next",  1, (void *)NextOp, 2, 2, "",},
    {"seek",  1, (void *)SeekOp, 3, 3, "+n|-n|n",},
    {"release", 1, (void *)ReleaseOp, 2, 2, "",},
};

static int nRpVideoOps = sizeof(rpVideoOps) / sizeof(Rp_OpSpec);

/*
 * ------------------------------------------------------------------------
 *  RpVideo_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpVideo_Init(Tcl_Interp *interp)
{

    Tcl_CreateObjCommand(interp, "::Rappture::Video",
        VideoCmd, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

/*
 * USAGE: Video <file>
 */
static int
VideoCmd(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj* const *objv)
{
    char cmdName[64];
    static int movieCount = 0;
    const char *filename = Tcl_GetString(objv[1]);

    // create a new command
    VideoObj *movie = NULL;
    movie = VideoInitCmd();
    if (movie == NULL) {
        Tcl_AppendResult(interp, "error while creating movie object", "\n",
                         "VideoInitCmd(movie);", (char*)NULL);
        return TCL_ERROR;
    }
    VideoOpenFile(movie,filename,"r");

    sprintf(cmdName,"::movieObj%d",movieCount);

    Tcl_CreateObjCommand(interp, cmdName, VideoCallCmd,
        (ClientData)movie, (Tcl_CmdDeleteProc*)NULL);

    Tcl_AppendResult(interp, cmdName, (char*)NULL);
    return TCL_OK;
}


static int
VideoCallCmd(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = (Tcl_ObjCmdProc *)Rp_GetOpFromObj(interp, nRpVideoOps, rpVideoOps,
        RP_OP_ARG1, objc, objv, 0);

    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/**********************************************************************/
// FUNCTION: GetOp()
/// Get info about the video
/**
 * Full function call:
 *
 * get position cur
 * get position end
 * get image width height
 * get framerate
 *
 */
static int
GetOp (ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    const char *cmd = Tcl_GetString(objv[1]);

    /*
     * Decode the first arg and figure out how we're supposed to advance.
     */
    if (objc > 5) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", cmd,
            " [image width height]|[position cur|end]\"", (char*)NULL);
        return TCL_ERROR;
    }

    const char *info = Tcl_GetString(objv[2]);
    if ((*info == 'p') && (strcmp(info,"position") == 0)) {
        if (objc != 4) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", cmd,
                " position cur|end\"", (char*)NULL);
            return TCL_ERROR;
        }
        const char *which = Tcl_GetString(objv[3]);
        if ((*which == 'c') && (strcmp(which,"cur") == 0)) {
            int pos = 0;
            VideoGetPositionCur((VideoObj *)clientData,&pos);
            Tcl_SetObjResult(interp, Tcl_NewIntObj(pos));
        }
        else if ((*which == 'e') && (strcmp(which,"end") == 0)) {
            int pos = 0;
            VideoGetPositionEnd((VideoObj *)clientData,&pos);
            Tcl_SetObjResult(interp, Tcl_NewIntObj(pos));
        }
        else {
            Tcl_AppendResult(interp, "unrecognized command: \"", which,
                "\" should be one of cur,end ", (char*)NULL);
            return TCL_ERROR;
        }
    }
    else if ((*info == 'i') && (strcmp(info,"image") == 0)) {
        if (objc != 5) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", cmd,
                " image width height\"", (char*)NULL);
            return TCL_ERROR;
        }

        void *img = NULL;
        int width = 0;
        int height = 0;
        int bufSize = 0;

        Tcl_GetIntFromObj(interp, objv[3], &width);
        Tcl_GetIntFromObj(interp, objv[4], &height);

//        VideoGoNext((VideoObj *)clientData);
        VideoGetImage((VideoObj *)clientData, width, height, &img, &bufSize);

        Tcl_SetByteArrayObj(Tcl_GetObjResult(interp),
                            (const unsigned char*)img, bufSize);
    }
    else if ((*info == 'f') && (strcmp(info,"framerate") == 0)) {
        if (objc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", cmd,
                " framerate\"", (char*)NULL);
            return TCL_ERROR;
        }

        double fr = 0;

        VideoGetFrameRate((VideoObj *)clientData, &fr);
        Tcl_SetObjResult(interp, Tcl_NewDoubleObj(fr));
    }
    else {
        Tcl_AppendResult(interp, "unrecognized command \"", info, "\": should be \"", cmd,
            " [image width height]|[position cur|end]|[framerate]\"", (char*)NULL);
        return TCL_ERROR;
    }


    return TCL_OK;
}


/**********************************************************************/
// FUNCTION: NextOp()
/// Get the next frame from a video
/**
 * Return the next frame from a video as an image
 * Full function call:
 *
 * next
 *
 */
static int
NextOp (ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{

    void *img = NULL;
    int width = 960;
    int height = 540;
    int bufSize = 0;

    VideoGoNext((VideoObj *)clientData);
    VideoGetImage((VideoObj *)clientData, width, height, &img, &bufSize);

    Tcl_SetByteArrayObj(Tcl_GetObjResult(interp),
                        (const unsigned char*)img, bufSize);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: SeekOp()
/// Get the next frame from a video
/**
 * Return the frame specified, or at the specified offset, as an image
 * Full function call:
 *
 * seek +5
 * seek -5
 * seek 22
 *
 */
static int
SeekOp (ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{

    void *img = NULL;
    int width = 960;
    int height = 540;
    int bufSize = 0;
    const char *val_s = NULL;
    int val = 0;

    val_s = Tcl_GetString(objv[2]);
    if (*val_s == '+') {
        if (Tcl_GetInt(interp, val_s+1, &val) != TCL_OK) {
            Tcl_AppendResult(interp, "bad value \"", val_s,
                "\": should be next, +n, -n, or n", (char*)NULL);
            return TCL_ERROR;
        }
        VideoGoPlusMinusN((VideoObj *)clientData, val);
    }
    else if (*val_s == '-') {
        if (Tcl_GetInt(interp, val_s, &val) != TCL_OK) {
            Tcl_AppendResult(interp, "bad value \"", val_s,
                "\": should be next, +n, -n, or n", (char*)NULL);
            return TCL_ERROR;
        }
        VideoGoPlusMinusN((VideoObj *)clientData, val);
    }
    else if (Tcl_GetInt(interp, val_s, &val) != TCL_OK) {
        Tcl_AppendResult(interp, "bad value \"", val_s,
            "\": should be next, +n, -n, or n", (char*)NULL);
        return TCL_ERROR;
    }
    else {
        int c = 0;
        c = VideoGoToN((VideoObj *)clientData, val);
        // printf("c = %d\tval = %d\n",c,val);
    }

    VideoGetImage((VideoObj *)clientData, width, height, &img, &bufSize);

    if (img == NULL) {
        printf("img is null\n");
    }

    Tcl_SetByteArrayObj(Tcl_GetObjResult(interp),
                        (const unsigned char*)img, bufSize);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: ReleaseOp()
/// Clean up memory from an open video in a movie player object
/**
 * Close all file handles and free allocated memory associated with an
 * open video in a movie player object.
 * Full function call:
 *
 * close
 *
 */
static int
ReleaseOp (ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    VideoClose((VideoObj *)clientData);

    Tcl_ResetResult(interp);
    return TCL_OK;
}
