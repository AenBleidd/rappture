/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2014  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_SCALEBAR_H
#define GEOVIS_SCALEBAR_H

namespace GeoVis {

enum ScaleBarUnits {
    UNITS_METERS,
    UNITS_INTL_FEET,
    UNITS_US_SURVEY_FEET,
    UNITS_NAUTICAL_MILES
};

extern double normalizeScaleMeters(double meters);

extern double normalizeScaleFeet(double feet);

extern double normalizeScaleNauticalMiles(double nmi);

}

#endif
