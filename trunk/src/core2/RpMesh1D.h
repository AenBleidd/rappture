/*
 * ----------------------------------------------------------------------
 *  Rappture::Mesh1D
 *    This is a non-uniform mesh for 1-dimensional structures.
 *    It's like a string of x-coordinates that form the range for
 *    an X-Y plot.  Nodes can be added to and deleted from the mesh,
 *    and the interval containing a given coordinate can be found
 *    quickly, to support interpolation operations.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RPMESH1D_H
#define RPMESH1D_H

#include <deque>
#include <RpNode.h>
#include <RpSerializable.h>

namespace Rappture {

class Cell1D {
public:
    Cell1D();
    Cell1D(int n0, double x0, int n1, double x1);
    int& nodeId(int n);
    double& x(int n);

    int isOutside() const;

private:
    int _nodeIds[2];
    double _x[2];
};

class Mesh1D : public Serializable {
public:
    Mesh1D();
    Mesh1D(double x0, double x1, int npts);
    Mesh1D(const Mesh1D& mesh);
    Mesh1D& operator=(const Mesh1D& mesh);
    virtual ~Mesh1D();

    virtual Node1D& add(const Node1D& node);
    virtual Mesh1D& remove(int nodeId);
    virtual Mesh1D& remove(const Node1D& node);
    virtual Mesh1D& clear();

    virtual int size() const;
    virtual Node1D& at(int pos);
    virtual double rangeMin() const;
    virtual double rangeMax() const;

    virtual Cell1D locate(const Node1D& node) const;

    // required for base class Serializable:
    const char* serializerType() const { return "Mesh1D"; }
    char serializerVersion() const { return 'A'; }

    void serialize_A(SerialBuffer& buffer) const;
    static Ptr<Serializable> create();
    Outcome deserialize_A(SerialBuffer& buffer);

protected:
    virtual int _locateInterval(double x) const;
    virtual void _rebuildNodeIdMap();

private:
    std::deque<Node1D> _nodelist;   // list of all nodes, in sorted order
    int _counter;                   // auto counter for node IDs

    std::deque<int> _id2node;       // maps node Id => index in _nodelist
    int _id2nodeDirty;              // non-zero => _id2node needs to be rebuilt

    // methods for serializing/deserializing version 'A'
    static SerialConversion versionA;
};

} // namespace Rappture

#endif /*RPMESH1D_H*/
