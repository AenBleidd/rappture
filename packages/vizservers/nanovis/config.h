/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef CONFIG_H__
#define CONFIG_H__

/*
 * GeForce 6 series and above cards support non-power-of-two texture
 * dimensions.  If using a 5 series card, disable this define
 */
#define HAVE_NPOT_TEXTURES

/*
 * GeForce 6 series and above cards support 16- or 32-bit float filtering 
 * and blending.  If using a 5 series card, disable this define
 */
#define HAVE_FLOAT_TEXTURES

/*
 * GeForce 8 series cards support 32-bit float filtering and blending.
 * If using a 6 or 7 series card, enable this define to use 16-bit float
 * textures and blending
 */
#define USE_HALF_FLOAT

#define XINETD				/* Enable render server. */
//#define EVENTLOG			/* Enable event logging. */
//#define DO_RLE			/* Do run length compression. */

#define KEEPSTATS		1

/*
 * Controls if debug trace logging is enabled
 */
#define WANT_TRACE

//#define OLD_CAMERA

/* 
 * The following define controls whether new prototype features are to be
 * compiled.  Right now by default it's off (0). That's because nanovis
 * releases are built directly from the subversion repository.  So for now,
 * we'll rely on developers to set this in their respective sandboxes.
 */
#define PROTOTYPE		0

/* 
 * The following define controls whether the new or old load_volume_stream 
 * implementation is used to load DX data. The difference is that the old
 * implementation doesn't do any interpolation of the points to a coarser
 * mesh.  Setting ISO_TEST to 1 will cause the old implementation to be
 * used, which makes isosurfaces work correctly (FIXME: is this still 
 * true?)
 *
 * [In the future, we'll use the OpenDX library reader and determine at
 * runtime if mesh decimation is required]
 */
#define ISO_TEST                0

/*
 * Determines if Sobel filter is applied to gradients when loading a 
 * volume
 */
#define FILTER_GRADIENTS        0

/* 
 * The following define controls whether the plane* commands are 
 * registered in the interpreter.  Right now it's off.  [Are these
 * commands still required?]
 */
#define PLANE_CMD               0

#endif 
