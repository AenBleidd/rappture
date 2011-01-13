/*
 * ----------------------------------------------------------------------
 *  TkFFMPEG:  video
 *
 *  These routines support the methods in the "video" class, which is
 *  a video stream that can be read from or written to.  The class
 *  itself is defined in itcl, but when methods are called, execution
 *  jumps down to this level.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2008  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifdef __cplusplus
extern "C" {
#endif


typedef struct VideoObjRec VideoObj;

VideoObj *VideoInitCmd ();
int VideoCleanupCmd (VideoObj *vidPtr);
int VideoOpenFile (VideoObj *vidPtr,
    const char *fileName, const char *mode);
int VideoGetImage (VideoObj *vidPtr,
    int width, int height, void **img, int *bufSize);
int VideoGetPositionCur (VideoObj *vidPtr, int *pos);
int VideoGetPositionEnd (VideoObj *vidPtr, int *pos);
int VideoGetFrameRate (VideoObj *vidPtr, double *fr);
int VideoGetFileName (VideoObj *vidPtr, const char **fname);
// static int VideoPutCmd (ClientData clientData,
//     Tcl_Interp  *interp, int argc, CONST84 char *argv[]);
int VideoGoNext (VideoObj *vidPtr);
int VideoGoPlusMinusN (VideoObj *vidPtr, int n);
int VideoGoToN (VideoObj *vidPtr, int n);
int VideoSizeCmd (VideoObj *vidPtr, int *width, int *height);
int VideoClose (VideoObj *vidPtr);

#ifdef __cplusplus
}
#endif
