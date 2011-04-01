/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <string>
#include <list>
#include <cassert>
#include <vtkLookupTable.h>

#include "ColorMap.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

ColorMap::ColorMap(const std::string& name) :
    _name(name),
    _needsBuild(true),
    _numTableEntries(256)
{
}

ColorMap::~ColorMap()
{
}

/**
 * \brief Return the name/ID of the ColorMap
 */
const std::string& ColorMap::getName()
{
    return _name;
}

/**
 * \brief Build and return the vtkLookupTable from the transfer function
 */
vtkLookupTable *ColorMap::getLookupTable()
{
    build();
    assert(_lookupTable != NULL);
    return _lookupTable;
}

/**
 * \brief Insert a new RGB control point into the transfer function
 */
void ColorMap::addControlPoint(ControlPoint& cp)
{
    _needsBuild = true;
    // Clamp value to [0,1]
    if (cp.value < 0.0)
	cp.value = 0.0;
    if (cp.value > 1.0)
	cp.value = 1.0;

    TRACE("New control point: %g  = %g %g %g",
	  cp.value, cp.color[0], cp.color[1], cp.color[2]);

    for (std::list<ControlPoint>::iterator itr = _controlPoints.begin();
	 itr != _controlPoints.end(); ++itr) {
	if (itr->value == cp.value) {
	    *itr = cp;
	    return;
	} else if (itr->value > cp.value) {
	    _controlPoints.insert(itr, cp);
	    return;
	}
    }
    // If we reach here, our control point goes at the end
    _controlPoints.insert(_controlPoints.end(), cp);
}

/**
 * \brief Insert a new opacity/alpha control point into the transfer function
 */
void ColorMap::addOpacityControlPoint(OpacityControlPoint& cp)
{
    _needsBuild = true;
    // Clamp value to [0,1]
    if (cp.value < 0.0)
	cp.value = 0.0;
    if (cp.value > 1.0)
	cp.value = 1.0;

    TRACE("New opacity control point: %g  = %g",
	  cp.value, cp.alpha);

    for (std::list<OpacityControlPoint>::iterator itr = _opacityControlPoints.begin();
	 itr != _opacityControlPoints.end(); ++itr) {
	if (itr->value == cp.value) {
	    *itr = cp;
	    return;
	} else if (itr->value > cp.value) {
	    _opacityControlPoints.insert(itr, cp);
	    return;
	}
    }
    // If we reach here, our control point goes at the end
    _opacityControlPoints.insert(_opacityControlPoints.end(), cp);
}

/**
 * \brief Set the number of discrete color table entries
 *
 * The number of table entries refers to the underlying 
 * vtkLookupTable and is independent of the number of 
 * control points in the transfer function.
 */
void ColorMap::setNumberOfTableEntries(int numEntries)
{
    if (numEntries != _numTableEntries) {
	_needsBuild = true;
	_numTableEntries = numEntries;
	if (_lookupTable != NULL) {
	    build();
	}
    }
}

/**
 * \brief Build the lookup table from the control points in the transfer 
 * function
 */
void ColorMap::build()
{
    if (!_needsBuild)
	return;

    if (_lookupTable == NULL) {
	_lookupTable = vtkSmartPointer<vtkLookupTable>::New();
    }

    _lookupTable->SetNumberOfTableValues(_numTableEntries);

    std::list<ControlPoint>::iterator itr = _controlPoints.begin();
    std::list<OpacityControlPoint>::iterator oitr = _opacityControlPoints.begin();

    // If first value is > 0, insert a copy at 0 to create
    // constant range up to first specified cp
    if (itr->value > 0.0) {
	ControlPoint cp = *itr;
	cp.value = 0.0;
	itr = _controlPoints.insert(itr, cp);
    }
    if (oitr->value > 0.0) {
	OpacityControlPoint ocp = *oitr;
	ocp.value = 0.0;
	oitr = _opacityControlPoints.insert(oitr, ocp);
    }

    std::list<ControlPoint>::iterator itr2 = itr;
    itr2++;
    std::list<OpacityControlPoint>::iterator oitr2 = oitr;
    oitr2++;

    for (int i = 0; i < _numTableEntries; i++) {
	double value = ((double)i)/(_numTableEntries-1);
        double color[4];
	if (itr2 != _controlPoints.end() && value > itr2->value) {
	    itr = itr2;
	    itr2++;
	}
	if (oitr2 != _opacityControlPoints.end() && value > oitr2->value) {
	    oitr = oitr2;
	    oitr2++;
	}
	if (itr2 == _controlPoints.end()) {
	    TRACE("val: %g Range: %g - 1 Color: %g %g %g", value, itr->value,
		  itr->color[0], itr->color[1], itr->color[2]);
            memcpy(color, itr->color, sizeof(double)*3);
	} else {
	    assert(itr->value < itr2->value);
	    assert(value >= itr->value && value <= itr2->value);
	    lerp(color, *itr, *itr2, value);
	    TRACE("val: %g Range: %g - %g Color: %g %g %g", value, itr->value, itr2->value,
		  color[0], color[1], color[2]);
	}
	if (oitr2 == _opacityControlPoints.end()) {
	    TRACE("val: %g Range: %g - 1 Alpha %g", value, oitr->value,
		  oitr->alpha);
            color[3] = oitr->alpha;
	} else {
	    assert(oitr->value < oitr2->value);
	    assert(value >= oitr->value && value <= oitr2->value);
	    lerp(&color[3], *oitr, *oitr2, value);
	    TRACE("val: %g Range: %g - %g Alpha: %g", value, oitr->value, oitr2->value,
		  color[3]);
	}
        _lookupTable->SetTableValue(i, color);
    }
    _needsBuild = false;
}

void ColorMap::lerp(double *result, const ControlPoint& cp1, const ControlPoint& cp2, double value)
{
    double factor = (value - cp1.value) / (cp2.value - cp1.value);
    for (int i = 0; i < 3; i++) {
	result[i] = cp1.color[i] * (1.0 - factor) + cp2.color[i] * factor;
    }
}

void ColorMap::lerp(double *result, const OpacityControlPoint& cp1, const OpacityControlPoint& cp2, double value)
{
    double factor = (value - cp1.value) / (cp2.value - cp1.value);
    *result = cp1.alpha * (1.0 - factor) + cp2.alpha * factor;
}

/**
 * \brief Remove all control points and lookup table
 */
void ColorMap::clear()
{
    _controlPoints.clear();
    _opacityControlPoints.clear();
    _lookupTable = NULL;
}

/**
 * \brief Create a default ColorMap with a blue-cyan-green-yellow-red ramp
 */
ColorMap * ColorMap::createDefault()
{
    ColorMap *colorMap = new ColorMap("default");
    ControlPoint cp[5];
    cp[0].value = 0.0;
    cp[0].color[0] = 0.0;
    cp[0].color[1] = 0.0;
    cp[0].color[2] = 1.0;
    cp[1].value = 0.25;
    cp[1].color[0] = 0.0;
    cp[1].color[1] = 1.0;
    cp[1].color[2] = 1.0;
    cp[2].value = 0.5;
    cp[2].color[0] = 0.0;
    cp[2].color[1] = 1.0;
    cp[2].color[2] = 0.0;
    cp[3].value = 0.75;
    cp[3].color[0] = 1.0;
    cp[3].color[1] = 1.0;
    cp[3].color[2] = 0.0;
    cp[4].value = 1.0;
    cp[4].color[0] = 1.0;
    cp[4].color[1] = 0.0;
    cp[4].color[2] = 0.0;
    for (int i = 0; i < 5; i++) {
	colorMap->addControlPoint(cp[i]);
    }
    OpacityControlPoint ocp[2];
    ocp[0].value = 0.0;
    ocp[0].alpha = 1.0;
    ocp[1].value = 1.0;
    ocp[1].alpha = 1.0;
    colorMap->addOpacityControlPoint(ocp[0]);
    colorMap->addOpacityControlPoint(ocp[1]);
    colorMap->build();
    return colorMap;
}
