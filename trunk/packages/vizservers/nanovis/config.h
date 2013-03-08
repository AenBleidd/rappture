/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

/*
 * Do run length compression
 */
//#define DO_RLE

/*
 * Keep statistics
 */
#define KEEPSTATS

/*
 * Controls whether DX data is downsampled.
 */
//#define DOWNSAMPLE_DATA

/*
 * Determines if Sobel filter is applied to gradients when loading a 
 * volume
 */
#define FILTER_GRADIENTS

/* 
 * Controls whether the plane* commands are registered in the 
 * interpreter. [Are these commands still required?]
 */
//#define PLANE_CMD

#endif 
