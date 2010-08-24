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
#include "RpMediaPlayer.h"

extern "C" Tcl_AppInitProc RpMediaPlayer_Init;

#include "RpOp.h"

static Tcl_ObjCmdProc MediaPlayerCmd;
static Tcl_ObjCmdProc MediaPlayerCallCmd;
static Tcl_ObjCmdProc InitOp;
static Tcl_ObjCmdProc LoadOp;
static Tcl_ObjCmdProc ReadOp;
static Tcl_ObjCmdProc ReleaseOp;

static Rp_OpSpec rpMediaPlayerOps[] = {
    {"init",  1, (void *)InitOp, 3, 3, "<path>",},
    {"load",  1, (void *)LoadOp, 3, 3, "<path>",},
    {"read",  1, (void *)ReadOp, 2, 2, "",},
    {"release", 1, (void *)ReleaseOp, 2, 2, "",},
};

static int nRpMediaPlayerOps = sizeof(rpMediaPlayerOps) / sizeof(Rp_OpSpec);

/*
 * ------------------------------------------------------------------------
 *  RpMediaPlayer_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpMediaPlayer_Init(Tcl_Interp *interp)
{

    Tcl_CreateObjCommand(interp, "::Rappture::MediaPlayer",
        MediaPlayerCmd, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

/*
 * USAGE: MediaPlayer <file>
 */
static int
MediaPlayerCmd(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj* const *objv)
{
    char cmdName[64];
    static int movieCount = 0;
    const char *filename = Tcl_GetString(objv[1]);
    Rappture::Outcome context;

    // create a new command
    Rappture::MediaPlayer *movie = new Rappture::MediaPlayer();
    if (!movie->init(context, filename)) {
        Tcl_AppendResult(interp, context.remark(), "\n",
                         context.context(), (char*)NULL);
        return TCL_ERROR;
    }

    sprintf(cmdName,"::movieObj%d",movieCount);

    Tcl_CreateObjCommand(interp, cmdName, MediaPlayerCallCmd,
        (ClientData)movie, (Tcl_CmdDeleteProc*)NULL);

    Tcl_AppendResult(interp, cmdName, (char*)NULL);
    return TCL_OK;
}


static int
MediaPlayerCallCmd(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = (Tcl_ObjCmdProc *)Rp_GetOpFromObj(interp, nRpMediaPlayerOps, rpMediaPlayerOps,
        RP_OP_ARG1, objc, objv, 0);

    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/**********************************************************************/
// FUNCTION: InitOp()
/// initialize a movie player object
/**
 * Initialize a movie player object with the video located at <path>
 * Full function call:
 *
 * init <path>
 *
 * <path> is the path of a video file.
 */
static int
InitOp (ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Rappture::Outcome context;
    const char *path = NULL;    // path of video

    path = Tcl_GetString(objv[0]);

    if (! ((Rappture::MediaPlayer *) clientData)->init(context, path) ) {
        Tcl_AppendResult(interp, context.remark(), "\n",
                         context.context(), (char*)NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: LoadOp()
/// Load a new video into the movie player object
/**
 * Load a new video into the movie player object
 * Full function call:
 *
 * load <path>
 *
 * <path> is the path of a video file.
 */
static int
LoadOp (ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Rappture::Outcome context;
    const char *path = NULL;    // path of video

    path = Tcl_GetString(objv[2]);

    if (! ((Rappture::MediaPlayer *) clientData)->load(context, path) ) {
        Tcl_AppendResult(interp, context.remark(), "\n",
                         context.context(), (char*)NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: ReadOp()
/// Read frames from a video
/**
 * Read frames from a video and return them as images
 * Full function call:
 *
 * read
 *
 */
static int
ReadOp (ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Rappture::Outcome context;
    Rappture::SimpleCharBuffer sb;

    if (((Rappture::MediaPlayer *)clientData)->read(context, &sb) != 1 ) {
        Tcl_AppendResult(interp, context.remark(), "\n",
                         context.context(), (char*)NULL);
        return TCL_ERROR;
    }

    Tcl_SetByteArrayObj(Tcl_GetObjResult(interp),
                        (const unsigned char*)sb.bytes(), sb.size());

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
    Rappture::Outcome context;

    if (! ((Rappture::MediaPlayer *) clientData)->release() ) {
        Tcl_AppendResult(interp, context.remark(), "\n",
                         context.context(), (char*)NULL);
        return TCL_ERROR;
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}

