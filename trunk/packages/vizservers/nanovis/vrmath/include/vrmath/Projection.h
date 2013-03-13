/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */
#ifndef VRPROJECTION_H
#define VRPROJECTION_H

namespace vrmath {

extern int
unproject(float winx, float winy, float winz,
          const float modelMatrix[16],
          const float projMatrix[16],
          const int viewport[4],
          float *objx, float *objy, float *objz);

extern void
perspective(float fovy, float aspect, float zNear, float zFar, float *matrix);

}

#endif

