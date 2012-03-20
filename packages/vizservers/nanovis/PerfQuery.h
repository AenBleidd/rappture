/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * PerfQuery.h: performance query class
 * 		It counts then number of pixels rendered on screen using
 * 		OpenGL occlusion query extension
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
#ifndef _PERFQUERY_H_
#define _PERFQUERY_H_

#include <stdio.h>

#include <GL/glew.h>

#include "Trace.h"

//check if occlusion query is supported
inline bool check_query_support()
{
    int bitsSupported = -1;
    glGetQueryivARB(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, 
		    &bitsSupported);
    if (bitsSupported == 0) {
	TRACE("occlusion query not supported!\n");
	return false;
    } else {
	TRACE("Occlusion query with %d bits supported\n", bitsSupported);
	return true;
    }
}

class PerfQuery
{
public:
    GLuint id;
    int pixel;
    
    PerfQuery();
    ~PerfQuery();
    
    void enable();	//start counting how many pixels are rendered
    void disable();	//stop counting

    void reset()
    { 
	pixel = 0; 
    }

    int get_pixel_count()
    {
	return pixel;		//return current pixel count
    }
};

#endif

