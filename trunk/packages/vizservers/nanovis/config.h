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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define NV40				/* Uncomment if using 6 series
					 * card. By default we assume older
					 * card the 5xxx series */
#define XINETD				/* Enable render server. */
//#define EVENTLOG			/* Enable event logging. */
//#define DO_RLE			/* Do run length compression. */

/* 
 * The following define controls whether new prototype features are to be
 * compiled.  Right now by default it's off (0). That's because nanovis
 * releases are built directly from the subversion repository.  So for now,
 * we'll rely on developers to set this in their respective sandboxes.
 */
#define PROTOTYPE		0

//#define USE_POINTSET_RENDERER

/* 
 * The following define controls whether new load_volume_stream or
 * load_volume_stream2 are used to load DX data.  The difference is that
 * load_volume_stream2 doesn't do any interpolation of the points to a coarser
 * mesh.  Right now, we're using load_volume_stream2 to make isosurfaces
 * work correctly.  

 * [In the future, we'll use the OpenDX library reader and determine at
 * runtime if mesh decimation is required]
 */
#define ISO_TEST                0

/* 
 * The following define controls whether the plane* commands are 
 * registered in the interpreter.  Right now it's off.  [Are these
 * commands still required?]
 */
#define PLANE_CMD               0

#endif 
