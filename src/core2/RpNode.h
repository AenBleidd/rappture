/*
 * ----------------------------------------------------------------------
 *  Rappture::Node1D
 *  Rappture::Node2D
 *  Rappture::Node3D
 *    This is a node in cartesian coordinate space.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RPNODE_H
#define RPNODE_H

namespace Rappture {

enum Axis {
   xaxis, yaxis, zaxis
};

class Node {
public:
    Node() : _id(-1) {}

    virtual Node& id(int newid) { _id=newid; return *this; }
    virtual int id() const { return _id; }

    virtual int dimensionality() const = 0;

    virtual ~Node() {}

private:
    int _id;  // node identifier (used for fields)
};


class Node1D : public Node {
public:
    Node1D(double x) : _x(x) {}

    virtual int dimensionality() const { return 1; }
    virtual double x() const { return _x; }
    virtual double x(double newval) { _x = newval; return _x; }

    virtual ~Node1D() {}

private:
    double _x;  // x-coordinate
};


class Node2D : public Node {
public:
    Node2D(double x, double y) : _x(x), _y(y) {}

    virtual int dimensionality() const { return 2; }
    virtual double x() const { return _x; }
    virtual double y() const { return _y; }

    virtual double x(double newval) { _x = newval; return _x; }
    virtual double y(double newval) { _y = newval; return _y; }

    virtual ~Node2D() {}

private:
    double _x;  // x-coordinate
    double _y;  // y-coordinate
};


class Node3D : public Node {
public:
    Node3D(double x, double y, double z) : _x(x), _y(y), _z(z) {}

    virtual int dimensionality() const { return 3; }
    virtual double x() const { return _x; }
    virtual double y() const { return _y; }
    virtual double z() const { return _z; }

    virtual double x(double newval) { _x = newval; return _x; }
    virtual double y(double newval) { _y = newval; return _y; }
    virtual double z(double newval) { _z = newval; return _z; }

    virtual ~Node3D() {}

private:
    double _x;  // x-coordinate
    double _y;  // y-coordinate
    double _z;  // y-coordinate
};

} // namespace Rappture

#endif /*RPNODE_H*/
