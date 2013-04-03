/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 *   Insoo Woo <iwoo@purdue.edu>
 *   George A. Howlett <gah@purdue.edu>
 *   Leif Delgass <ldelgass@purdue.edu>
 */
#ifndef NV_FLOWTYPES_H
#define NV_FLOWTYPES_H

namespace nv {

struct FlowColor {
    float r, g, b, a;
};

struct FlowPoint {
    float x, y, z;
};

struct FlowPosition {
    float value;
    unsigned int flags;
    int axis;
};

// FlowPosition flag values
#define RELPOS 0
#define ABSPOS 1

}

#endif
