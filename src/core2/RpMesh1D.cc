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
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <assert.h>
#include "RpMesh1D.h"

using namespace Rappture;

// serialization version 'A' for Mesh1D...
SerialConversion Mesh1D::versionA("Mesh1D", 'A',
    (Serializable::serializeObjectMethod)&Mesh1D::serialize_A,
    &Mesh1D::create,
    (Serializable::deserializeObjectMethod)&Mesh1D::deserialize_A);


Cell1D::Cell1D()
{
    _nodeIds[0] = _nodeIds[1] = -1;
    _x[0] = _x[1] = 0.0;
}

Cell1D::Cell1D(int n0, double x0, int n1, double x1)
{
    _nodeIds[0] = n0;
    _nodeIds[1] = n1;
    _x[0] = x0;
    _x[1] = x1;
}

int&
Cell1D::nodeId(int n)
{
    assert(n >= 0 && n < 2);
    return _nodeIds[n];
}

double&
Cell1D::x(int n)
{
    assert(n >= 0 && n < 2);
    return _x[n];
}

int
Cell1D::isOutside() const
{
    return (_nodeIds[0] < 0 || _nodeIds[1] < 0);
}


Mesh1D::Mesh1D()
  : _nodelist(),
    _counter(0),
    _id2node(),
    _id2nodeDirty(1)
{
}

Mesh1D::Mesh1D(double x0, double x1, int npts)
  : _nodelist(),
    _counter(0),
    _id2node(),
    _id2nodeDirty(1)
{
    Node1D newnode(0.0);

    assert(npts >= 2);
    double dx = (x1-x0)/(npts-1);

    for (int i=0; i < npts; i++) {
        newnode.x(x0 + i*dx);
        newnode.id(_counter++);
        _nodelist.push_back(newnode);
    }
}

Mesh1D::Mesh1D(const Mesh1D& mesh)
  : _nodelist(mesh._nodelist),
    _counter(mesh._counter),
    _id2node(mesh._id2node),
    _id2nodeDirty(mesh._id2nodeDirty)
{
}

Mesh1D&
Mesh1D::operator=(const Mesh1D& mesh)
{
    _nodelist = mesh._nodelist;
    _counter = mesh._counter;
    _id2node = mesh._id2node;
    _id2nodeDirty = mesh._id2nodeDirty;
    return *this;
}

Mesh1D::~Mesh1D()
{
}

Node1D&
Mesh1D::add(const Node1D& node)
{
    int added = 0;  // nothing added yet

    // make sure this node has an ID, or assign one automatically
    Node1D newnode(node);
    if (newnode.id() < 0) {
        newnode.id(_counter++);
    }

    // find the spot where this node should be inserted
    int n = _locateInterval(newnode.x());
    if (n >= 0) {
        if (newnode.x() > _nodelist[n].x()) {
            _nodelist.insert(_nodelist.begin()+n+1, newnode);
            added = 1;
        }
    }
    else if (_nodelist.size() > 0 && newnode.x() < _nodelist[0].x()) {
        _nodelist.push_front(newnode);
        added = 1;
        n = 0;
    }
    else {
        _nodelist.push_back(newnode);
        added = 1;
        n = _nodelist.size()-1;
    }

    // did we really add a node?  then rebuild the id2node map
    if (added) {
        _id2nodeDirty = 1;
    }
    return _nodelist[n];
}

Mesh1D&
Mesh1D::remove(int nodeId)
{
    if (!_id2nodeDirty) {
        if ((unsigned int) nodeId < _id2node.size()) {
            int n = _id2node[nodeId];
            _nodelist.erase( _nodelist.begin()+n );
        }
    } else {
        std::deque<Node1D>::iterator i;
        for (i=_nodelist.begin(); i != _nodelist.end(); ++i) {
            if (i->id() == nodeId) {
                _nodelist.erase(i);
                break;
            }
        }
    }
    _id2nodeDirty = 1;
    return *this;
}

Mesh1D&
Mesh1D::remove(const Node1D& node)
{
    if (node.id() >= 0) {
        // does this node have an ID?  then use it directly
        remove(node.id());
    } else {
        // try to find the node for this coordinate
        int n = _locateInterval(node.x());
        if (n >= 0) {
            // first node in interval?
            if (node.x() == _nodelist[n].x()) {
                _nodelist.erase( _nodelist.begin()+n );
                _id2nodeDirty = 1;
            }
            // last node in interval?
            else if (  ( (unsigned int)(n+1) < _nodelist.size())
                    && ( node.x() == _nodelist[n+1].x()) ) {
                _nodelist.erase( _nodelist.begin()+n+1 );
                _id2nodeDirty = 1;
            }
        }
    }
    return *this;
}

Mesh1D&
Mesh1D::clear()
{
    _nodelist.clear();
    _id2node.clear();
    _counter = 0;
    _id2nodeDirty = 0;
    return *this;
}

int
Mesh1D::size() const
{
    return _nodelist.size();
}

Node1D&
Mesh1D::at(int pos)
{
    return _nodelist.at(pos);
}

double
Mesh1D::rangeMin() const
{
    if (_nodelist.size() > 0) {
        return _nodelist.at(0).x();
    }
    return 0.0;
}

double
Mesh1D::rangeMax() const
{
    int last = _nodelist.size()-1;
    if (last >= 0) {
        return _nodelist.at(last).x();
    }
    return 0.0;
}

Cell1D
Mesh1D::locate(const Node1D& node) const
{
    Cell1D rval;

    int n = _locateInterval(node.x());
    if (n < 0) {
        if (node.x() <= rangeMin()) {
            if (_nodelist.size() > 0) {
                rval.nodeId(1) = _nodelist[0].id();
                rval.x(1) = _nodelist[0].x();
            }
        }
        else if (node.x() >= rangeMax()) {
            int last = _nodelist.size()-1;
            if (last >= 0) {
                rval.nodeId(0) = _nodelist.at(last).id();
                rval.x(0) = _nodelist.at(last).x();
            }
        }
    } else {
        rval.nodeId(0) = _nodelist[n].id();
        rval.x(0)      = _nodelist[n].x();
        rval.nodeId(1) = _nodelist[n+1].id();
        rval.x(1)      = _nodelist[n+1].x();
    }
    return rval;
}

int
Mesh1D::_locateInterval(double x) const
{
    int max   = _nodelist.size()-1;
    int first = 0;
    int last  = max;
    int pos   = (first+last)/2;

    while (first <= last) {
        int prev = pos-1;
        int next = pos+1;

        double x0 = _nodelist[pos].x();
        double x1 = (next <= max) ? _nodelist[next].x() : x0;

        if (x >= x0 && x <= x1) {
            if (next > max) {
                // On boundary node
                if (prev >= first)
                    return prev;
                else
                    return -1; // Shouldn't happen - single node!
            }
            return pos;
        }
        else if (x < x0) {
            if (prev < first) {
                return -1;   // can't go there -- bail out!
            }
            last = prev;
        }
        else {
            if (next > max) {
                return -1;   // can't go there -- bail out!
            }
            first = next;
        }
        pos = (first+last)/2;
    }
    return -1;
}

void
Mesh1D::_rebuildNodeIdMap()
{
    int maxid = 0;
    int n;
    std::deque<Node1D>::iterator i;

    // compute the maximum value for all node IDs
    for (i=_nodelist.begin(); i != _nodelist.end(); ++i) {
        if (i->id() > maxid) {
            maxid = i->id();
        }
    }

    // reserve enough space to map all of these IDs
    _id2node.clear();
    for (n=0; n < maxid; n++) {
        _id2node.push_back(-1);  // mapping not known
    }

    // run through all nodes and build the node map
    n = 0;
    for (i=_nodelist.begin(); i != _nodelist.end(); ++i,++n) {
        _id2node[ i->id() ] = n;
    }

    _id2nodeDirty = 0;
}

Ptr<Serializable>
Mesh1D::create()
{
    return Ptr<Serializable>( (Serializable*) new Mesh1D() );
}

void
Mesh1D::serialize_A(SerialBuffer& buffer) const
{
    Mesh1D* nonconst = (Mesh1D*)this;
    buffer.writeInt(_nodelist.size());

    std::deque<Node1D>::iterator iter = nonconst->_nodelist.begin();
    while (iter != _nodelist.end()) {
        buffer.writeInt( (*iter).id() );
        buffer.writeDouble( (*iter).x() );
        ++iter;
    }
    buffer.writeInt(_counter);
}

Outcome
Mesh1D::deserialize_A(SerialBuffer& buffer)
{
    Outcome status;
    Node1D newnode(0.0);

    clear();
    int npts = buffer.readInt();

    for (int n=0; n < npts; n++) {
        newnode.id( buffer.readInt() );
        newnode.x( buffer.readDouble() );
        _nodelist.push_back(newnode);
    }
    _counter = buffer.readInt();

    return status;
}
