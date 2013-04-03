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
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef NV_PERFQUERY_H
#define NV_PERFQUERY_H

#include <stdio.h>

#include <GL/glew.h>

#include "Trace.h"

namespace nv {

class PerfQuery
{
public:
    PerfQuery();
    ~PerfQuery();

    void enable();	//start counting how many pixels are rendered
    void disable();	//stop counting

    void reset()
    {
        pixel = 0; 
    }

    int getPixelCount()
    {
        return pixel;		//return current pixel count
    }

    static bool checkQuerySupport();

    GLuint id;
    int pixel;
};

//check if occlusion query is supported
inline bool PerfQuery::checkQuerySupport()
{
    if (!GLEW_ARB_occlusion_query) {
        TRACE("ARB_occlusion_query extension not supported");
        return false;
    }
    int bitsSupported = -1;
    glGetQueryivARB(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, 
                    &bitsSupported);
    if (bitsSupported == 0) {
        TRACE("occlusion query not supported!");
        return false;
    } else {
        TRACE("Occlusion query with %d bits supported", bitsSupported);
        return true;
    }
}

}

#endif

