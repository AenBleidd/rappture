/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_TYPES_H
#define VTKVIS_TYPES_H

namespace VtkVis {

    enum Axis {
        X_AXIS = 0,
        Y_AXIS,
        Z_AXIS
    };

    enum PrincipalPlane {
        PLANE_XY = 0,
        PLANE_ZY,
        PLANE_XZ
    };

}

#endif
