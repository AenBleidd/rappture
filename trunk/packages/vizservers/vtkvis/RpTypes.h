/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_TYPES_H__
#define __RAPPTURE_VTKVIS_TYPES_H__

namespace Rappture {
namespace VtkVis {

    enum Axis {
        X_AXIS,
        Y_AXIS,
        Z_AXIS
    };

    enum PrincipalPlane {
        PLANE_XY,
        PLANE_ZY,
        PLANE_XZ
    };

}
}

#endif
