/*
 * ----------------------------------------------------------------------
 *  Rappture::Field1D
 *    This is a continuous, linear function defined by a series of
 *    points for 1-dimensional structures.  It's essentially a string
 *    of (x,y) points where the y values can be interpolated at any
 *    point within the range of defined values.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <assert.h>
#include <math.h>
#include "RpField1D.h"

using namespace Rappture;

Field1D::Field1D()
  : _vmin(NAN),
    _vmax(NAN),
    _meshPtr(NULL),
    _counter(0)
{
    _meshPtr = Ptr<Mesh1D>(new Mesh1D());
}

Field1D::Field1D(const Ptr<Mesh1D>& meshPtr)
  : _valuelist(),
    _vmin(NAN),
    _vmax(NAN),
    _meshPtr(meshPtr),
    _counter(0)
{
}

Field1D::Field1D(const Field1D& field)
  : _valuelist(field._valuelist),
    _vmin(field._vmin),
    _vmax(field._vmax),
    _meshPtr(field._meshPtr),
    _counter(0)
{
}

Field1D&
Field1D::operator=(const Field1D& field)
{
    _valuelist = field._valuelist;
    _vmin = field._vmin;
    _vmax = field._vmax;
    _meshPtr = field._meshPtr;
    _counter = field._counter;
    return *this;
}

Field1D::~Field1D()
{
}

int
Field1D::add(double x)
{
    Node1D node = Node1D(x);
    node.id(_counter++);

    _valuelist.push_back(0.0);
    _meshPtr->add(node);

    return node.id();
}

Field1D&
Field1D::remove(int nodeId)
{
    _meshPtr->remove(nodeId);
    return *this;
}

Field1D&
Field1D::clear()
{
    _valuelist.clear();
    _meshPtr->clear();
    _counter = 0;
    return *this;
}

int
Field1D::size() const
{
    return _meshPtr->size();
}

Node1D&
Field1D::atNode(int pos)
{
    return _meshPtr->at(pos);
}

double
Field1D::rangeMin() const
{
    return _meshPtr->rangeMin();
}

double
Field1D::rangeMax() const
{
    return _meshPtr->rangeMax();
}

int
Field1D::define(double x, double y)
{
    Node1D node = Node1D(x);
    Cell1D cell = _meshPtr->locate(node);

    if (x == cell.x(0) && !cell.isOutside()) {
        define(cell.nodeId(0), y);
    }
    else if (x == cell.x(1) && !cell.isOutside()) {
        define(cell.nodeId(1), y);
    }
    else {
        int nodeId = _counter++;
        node.id(nodeId);
        _valuelist.push_back(0.0);
        _meshPtr->add(node);
        define(nodeId, y);
    }
    return 0;
}

int
Field1D::define(int nodeId, double y)
{
    _valuelist[nodeId] = y;

    if (isnan(_vmin) || isnan(_vmax)) {
        _vmin = _vmax = y;
    } else {
        if (y < _vmin) { _vmin = y; }
        if (y > _vmax) { _vmax = y; }
    }
    return 0;
}

double
Field1D::value(double x) const
{
    Node1D node = Node1D(x);
    Cell1D cell = _meshPtr->locate(node);

    // if this is a normal cell, then interpolate values
    if (cell.nodeId(0) >= 0 && cell.nodeId(1) >= 0) {
        double y0 = _valuelist[cell.nodeId(0)];
        double y1 = _valuelist[cell.nodeId(1)];
        double xrange = cell.x(1) - cell.x(0);

        if (xrange == 0.0) {
            // interval undefined? then return avg y value
            return 0.5*(y1+y0);
        }
        // interpolate
        double delx = x - cell.x(0);
        return y0 + (delx/xrange)*(y1-y0);
    }

    // extrapolate with constant value for x < xmin
    else if (cell.nodeId(0) >= 0) {
        return _valuelist[cell.nodeId(0)];
    }

    // extrapolate with constant value for x > xmax
    else if (cell.nodeId(1) >= 0) {
        return _valuelist[cell.nodeId(1)];
    }

    // all else fails, return an empty value
    return 0.0;
}

double
Field1D::valueMin() const
{
    return _vmin;
}

double
Field1D::valueMax() const
{
    return _vmax;
}
