/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_TYPES_H
#define GEOVIS_TYPES_H

namespace GeoVis {

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
