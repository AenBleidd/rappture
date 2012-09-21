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
#ifndef RPFIELD1D_H
#define RPFIELD1D_H

#include <RpPtr.h>
#include <RpMesh1D.h>

namespace Rappture {

class Field1D {
public:
    Field1D();
    Field1D(const Ptr<Mesh1D>& meshPtr);
    Field1D(const Field1D& field);
    Field1D& operator=(const Field1D& field);
    virtual ~Field1D();

    virtual int add(double x);
    virtual Field1D& remove(int nodeId);
    virtual Field1D& clear();

    virtual int size() const;
    virtual Node1D& atNode(int pos);
    virtual double rangeMin() const;
    virtual double rangeMax() const;

    virtual int define(double x, double y);
    virtual int define(int nodeId, double y);
    virtual double value(double x) const;
    virtual double valueMin() const;
    virtual double valueMax() const;

private:
    std::deque<double> _valuelist;  // list of all values, in nodeId order
    double _vmin;                   // minimum value in _valuelist
    double _vmax;                   // maximum value in _valuelist
    Ptr<Mesh1D> _meshPtr;           // mesh for all x-points
    int _counter;                   // counter for generating node IDs
};

} // namespace Rappture

#endif /*RPFIELD1D_H*/
