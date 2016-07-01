/*
 * ----------------------------------------------------------------------
 *  RpSqueezer - image distortion for "cover flow" effect.
 *
 *  Modifies a Tk image to squeeze one side or the other and perhaps
 *  foreshorten the image.
 *
 *    squeezer ?options...? <original> <newImage>
 *        -side left|right     << which side to squeeze
 *        -amount <fraction>   << squeeze down to this size (0-1)
 *        -shorten <fraction>  << squeeze horizontally to this size (0-1)
 *        -shadow <fraction>   << apply shadow horizontally (strength 0-1)
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <string.h>
#include <math.h>
#include "tk.h"

/*
 * Forward declarations.
 */
static int RpSqueezerCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int argc, CONST char **argv));

/*
 * ------------------------------------------------------------------------
 *  RpSqueezer_Init --
 *
 *  Installs the "squeezer" command in a Tcl interpreter.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpSqueezer_Init(interp)
    Tcl_Interp *interp;         /* interpreter being initialized */
{
    /* install the widget command */
    Tcl_CreateCommand(interp, "squeezer", RpSqueezerCmd,
        NULL, NULL);

    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * RpSqueezerCmd()
 *
 * Called to handle the "squeezer" command.  Returns TCL_OK on success
 * and TCL_ERROR otherwise.
 * ----------------------------------------------------------------------
 */
static int
RpSqueezerCmd(clientData, interp, argc, argv)
    ClientData clientData;        /* not used */
    Tcl_Interp *interp;           /* current interpreter */
    int argc;                     /* number of command arguments */
    CONST char **argv;            /* command argument strings */
{
    int sideRight = 1;
    double amount = 0.8;
    double shorten = 0.7;
    double shadow = 0.0;

    int i, j, j0, j1, p, srcw, srch, destw, desth, nbytes;
    int srci, srcj, rval, gval, bval, aval;
    double shfac;
    Tk_PhotoHandle srcPhoto, destPhoto;
    Tk_PhotoImageBlock srcBlock, destBlock;
    unsigned char* pixelPtr;

   /* parse command line options */
    i = 1;
    while (i < argc) {
        if (*argv[i] != '-') {
            break;
        }
        if (i+1 >= argc) {
            Tcl_AppendResult(interp, "missing value for option \"",
                argv[i], "\"", (char*)NULL);
            return TCL_ERROR;
        }

        if (strcmp(argv[i],"-side") == 0) {
            if (strcmp(argv[i+1],"left") == 0) {
                sideRight = 0;
            } else if (strcmp(argv[i+1],"right") == 0) {
                sideRight = 1;
            } else {
                Tcl_AppendResult(interp, "bad value \"",
                    argv[i+1], "\": should be left or right", (char*)NULL);
                return TCL_ERROR;
            }
        }
        else if (strcmp(argv[i],"-amount") == 0) {
            if (Tcl_GetDouble(interp, argv[i+1], &amount) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        else if (strcmp(argv[i],"-shorten") == 0) {
            if (Tcl_GetDouble(interp, argv[i+1], &shorten) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        else if (strcmp(argv[i],"-shadow") == 0) {
            if (Tcl_GetDouble(interp, argv[i+1], &shadow) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        else {
            Tcl_AppendResult(interp, "bad option \"", argv[i],
                "\": should be -side, -amount, -shorten, -shadow",
                (char*)NULL);
            return TCL_ERROR;
        }

        i += 2;
    }

    if (i+2 != argc) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            argv[0], " ?options? origImg newImg\"", (char*)NULL);
        return TCL_ERROR;
    }

    /* get the last two args... source and destination images */
    srcPhoto = Tk_FindPhoto(interp, argv[i]);
    if (srcPhoto == NULL) {
        Tcl_AppendResult(interp, "image \"", argv[i], "doesn't exist",
            " or isn't a photo image", (char*)NULL);
        return TCL_ERROR;
    }

    destPhoto = Tk_FindPhoto(interp, argv[i+1]);
    if (destPhoto == NULL) {
        Tcl_AppendResult(interp, "image \"", argv[i+1], "doesn't exist",
            " or isn't a photo image", (char*)NULL);
        return TCL_ERROR;
    }

    /*
     * Update the destination image to the proper size.
     */
    Tk_PhotoGetSize(srcPhoto, &srcw, &srch);
    destw = round(srcw * shorten);

    /*
     * Scan through each column along the width.  Compute the height
     * of the shortened column and then compute the pixels along the
     * height.
     */
    Tk_PhotoGetImage(srcPhoto, &srcBlock);

    destBlock.width = destw;
    destBlock.height = srch;
    destBlock.pixelSize = 4;
    destBlock.offset[0] = 0;
    destBlock.offset[1] = 1;
    destBlock.offset[2] = 2;
    destBlock.offset[3] = 3;
    destBlock.pitch = destBlock.pixelSize * destw;
    nbytes = destBlock.pitch * srch;
    destBlock.pixelPtr = (unsigned char*)ckalloc((unsigned)nbytes);
    memset(destBlock.pixelPtr, 0, (size_t)nbytes);

    for (p=0; p < destw; p++) {
        i = (sideRight) ? p : destw-p-1;
        desth = round(srch - srch*(1.0-amount)*p/(double)destw);
        j0 = (srch-desth)/2;
        j1 = j0 + desth;

        /* strength of shadow */
        shfac = 1.0 - shadow * p/(double)destw;

        for (j=j0; j < j1; j++) {
            srci = srcw * i/(double)destw;
            srcj = srch * (j-j0)/(double)desth;
            pixelPtr = srcBlock.pixelPtr + srcj*srcBlock.pitch 
                + srci*srcBlock.pixelSize;
            rval = *(pixelPtr+srcBlock.offset[0]);
            gval = *(pixelPtr+srcBlock.offset[1]);
            bval = *(pixelPtr+srcBlock.offset[2]);
            aval = *(pixelPtr+srcBlock.offset[3]);

            pixelPtr = destBlock.pixelPtr + j*destBlock.pitch 
                + i*destBlock.pixelSize;
            *(pixelPtr+0) = round(rval*shfac);
            *(pixelPtr+1) = round(gval*shfac);
            *(pixelPtr+2) = round(bval*shfac);
            *(pixelPtr+3) = aval;
        }
    }
#if TK_MAJOR_VERSION > 8 || (TK_MAJOR_VERSION == 8 && TK_MINOR_VERSION > 4)
    if (Tk_PhotoSetSize(interp, destPhoto, destw, srch) != TCL_OK) {
        ckfree((char*)destBlock.pixelPtr);
        return TCL_ERROR;
    }
    if (Tk_PhotoPutBlock(interp, destPhoto, &destBlock, 0, 0,
                         destBlock.width, destBlock.height, TK_PHOTO_COMPOSITE_SET) != TCL_OK) {
        ckfree((char*)destBlock.pixelPtr);
        return TCL_ERROR;
    }
#else
    Tk_PhotoSetSize(destPhoto, destw, srch);
    Tk_PhotoPutBlock(destPhoto, &destBlock, 0, 0,
        destBlock.width, destBlock.height, TK_PHOTO_COMPOSITE_SET);
#endif
    ckfree((char*)destBlock.pixelPtr);
    return TCL_OK;
}
