/*
 * ----------------------------------------------------------------------
 *  Rappture::MeshTri2D
 *    This is a non-uniform, triangular mesh for 2-dimensional
 *    structures.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <math.h>
#include <assert.h>
#include <float.h>
#include <iostream>

#include "RpMeshTri2D.h"

using namespace Rappture;

CellTri2D::CellTri2D()
{
    _cellId = -1;
    _nodes[0] = NULL;
    _nodes[1] = NULL;
    _nodes[2] = NULL;
}

CellTri2D::CellTri2D(int cellId, Node2D* n1Ptr, Node2D* n2Ptr, Node2D* n3Ptr)
{
    _cellId = cellId;
    _nodes[0] = n1Ptr;
    _nodes[1] = n2Ptr;
    _nodes[2] = n3Ptr;
}

CellTri2D::CellTri2D(const CellTri2D& cell)
{
    _cellId = cell._cellId;
    _nodes[0] = cell._nodes[0];
    _nodes[1] = cell._nodes[1];
    _nodes[2] = cell._nodes[2];
}

CellTri2D&
CellTri2D::operator=(const CellTri2D& cell)
{
    _cellId = cell._cellId;
    _nodes[0] = cell._nodes[0];
    _nodes[1] = cell._nodes[1];
    _nodes[2] = cell._nodes[2];
    return *this;
}

int
CellTri2D::isNull() const
{
    return (_nodes[0] == NULL || _nodes[1] == NULL || _nodes[2] == NULL);
}

int
CellTri2D::isOutside() const
{
    return (_cellId < 0);
}

void
CellTri2D::clear()
{
    _cellId = -1;
    _nodes[0] = _nodes[1] = _nodes[2] = NULL;
}

int
CellTri2D::cellId() const
{
    return _cellId;
}

int
CellTri2D::nodeId(int n) const
{
    assert(n >= 0 && n <= 2);
    return _nodes[n]->id();
}

double
CellTri2D::x(int n) const
{
    assert(n >= 0 && n <= 2);
    return _nodes[n]->x();
}

double
CellTri2D::y(int n) const
{
    assert(n >= 0 && n <= 2);
    return _nodes[n]->y();
}

void
CellTri2D::barycentrics(const Node2D& node, double* phi) const
{
    assert( _nodes[0] != NULL && _nodes[1] != NULL && _nodes[2] != NULL);

    double x2 = _nodes[1]->x() - _nodes[0]->x();
    double y2 = _nodes[1]->y() - _nodes[0]->y();
    double x3 = _nodes[2]->x() - _nodes[0]->x();
    double y3 = _nodes[2]->y() - _nodes[0]->y();
    double xr = node.x() - _nodes[0]->x();
    double yr = node.y() - _nodes[0]->y();
    double det = x2*y3-x3*y2;

    phi[1] = (xr*y3 - x3*yr)/det;
    phi[2] = (x2*yr - xr*y2)/det;
    phi[0] = 1.0-phi[1]-phi[2];
}


Tri2D::Tri2D()
{
    nodes[0] = nodes[1] = nodes[2] = -1;
    neighbors[0] = neighbors[1] = neighbors[2] = -1;
}

Tri2D::Tri2D(int n1, int n2, int n3)
{
    nodes[0] = n1;
    nodes[1] = n2;
    nodes[2] = n3;
    neighbors[0] = neighbors[1] = neighbors[2] = -1;
}


MeshTri2D::MeshTri2D()
  : _counter(0),
    _id2nodeDirty(0),
    _id2node(100,-1)
{
    _nodelist.reserve(1024);
    _min[0] = _min[1] = NAN;
    _max[0] = _max[1] = NAN;
}

MeshTri2D::MeshTri2D(const MeshTri2D& mesh)
  : _nodelist(mesh._nodelist),
    _counter(mesh._counter),
    _celllist(mesh._celllist),
    _id2nodeDirty(mesh._id2nodeDirty),
    _id2node(mesh._id2node)
{
    for (int i=0; i < 2; i++) {
        _min[i] = mesh._min[i];
        _max[i] = mesh._max[i];
    }
}

MeshTri2D&
MeshTri2D::operator=(const MeshTri2D& mesh)
{
    _nodelist = mesh._nodelist;
    _counter = mesh._counter;
    for (int i=0; i < 2; i++) {
        _min[i] = mesh._min[i];
        _max[i] = mesh._max[i];
    }
    _celllist = mesh._celllist;
    _id2nodeDirty = mesh._id2nodeDirty;
    _id2node = mesh._id2node;
    _lastLocate.clear();
    return *this;
}

MeshTri2D::~MeshTri2D()
{
}

Node2D&
MeshTri2D::addNode(const Node2D& nd)
{
    Node2D node(nd);

    if (node.id() < 0) {
        node.id(_counter++);
    } else {
        // see if this node already exists
        Node2D* nptr = _getNodeById(node.id());
        if (nptr) {
            return *nptr;
        }
    }

    // add this new node
    _nodelist.push_back(node);

    if (isnan(_min[0])) {
        _min[0] = _max[0] = node.x();
        _min[1] = _max[1] = node.y();
    } else {
        if (node.x() < _min[0]) { _min[0] = node.x(); }
        if (node.x() > _max[0]) { _max[0] = node.x(); }
        if (node.y() < _min[1]) { _min[1] = node.y(); }
        if (node.y() > _max[1]) { _max[1] = node.y(); }
    }

    // index of this new node
    int n = _nodelist.size()-1;

    if (!_id2nodeDirty) {
        // id2node map up to date?  then keep it up to date
        if ((unsigned int)node.id() >= _id2node.size()) {
            int newsize = 2*_id2node.size();
            for (int i=_id2node.size(); i < newsize; i++) {
                _id2node.push_back(-1);
            }
        }
        _id2node[node.id()] = n;
    }
    return _nodelist[n];
}

void
MeshTri2D::addCell(const Node2D& n1, const Node2D& n2, const Node2D& n3)
{
    Node2D node1 = addNode(n1);
    Node2D node2 = addNode(n2);
    Node2D node3 = addNode(n3);
    addCell(node1.id(), node2.id(), node3.id());
}

void
MeshTri2D::addCell(int nId1, int nId2, int nId3)
{
    _celllist.push_back( Tri2D(nId1,nId2,nId3) );
    int triId = _celllist.size()-1;

    Edge2D edge;
    int nodes[4];
    nodes[0] = nId1;
    nodes[1] = nId2;
    nodes[2] = nId3;
    nodes[3] = nId1;

    // update the neighbors for this triangle
    for (int i=0; i < 3; i++) {
        int n = (i+2) % 3;    // this is node opposite i/i+1 edge

        // build an edge with nodes in proper order
        if (nodes[i] < nodes[i+1]) {
            edge.fromNode = nodes[i];
            edge.toNode = nodes[i+1];
        } else {
            edge.fromNode = nodes[i+1];
            edge.toNode = nodes[i];
        }

        Neighbor2D& nbr = _edge2neighbor[edge];
        if (nbr.triId < 0) {
            // not found? then register this for later
            nbr.triId = triId;
            nbr.index = n;
        } else {
            // this triangle points to other one for the edge
            _celllist[triId].neighbors[n] = nbr.triId;
            // other triangle points to this one for the same edge
            _celllist[nbr.triId].neighbors[nbr.index] = triId;
            _edge2neighbor.erase(edge);
        }
    }
}

MeshTri2D&
MeshTri2D::clear()
{
    _nodelist.clear();
    _counter = 0;
    _id2nodeDirty = 0;
    _id2node.assign(100, -1);
    _lastLocate.clear();
    return *this;
}

int
MeshTri2D::sizeNodes() const
{
    return _nodelist.size();
}

Node2D&
MeshTri2D::atNode(int pos)
{
    assert(pos >= 0 && (unsigned int)(pos) < _nodelist.size());
    return _nodelist.at(pos);
}

int
MeshTri2D::sizeCells() const
{
    return _celllist.size();
}

CellTri2D
MeshTri2D::atCell(int pos)
{
    assert(pos >= 0 && (unsigned int)(pos) < _celllist.size());

    Tri2D& cell = _celllist[pos];
    Node2D* n1Ptr = _getNodeById(cell.nodes[0]);
    Node2D* n2Ptr = _getNodeById(cell.nodes[1]);
    Node2D* n3Ptr = _getNodeById(cell.nodes[2]);
    assert(n1Ptr && n2Ptr && n3Ptr);

    CellTri2D rval(pos, n1Ptr, n2Ptr, n3Ptr);
    return rval;
}

double
MeshTri2D::rangeMin(Axis which) const
{
    assert(which != Rappture::zaxis);
    return _min[which];
}

double
MeshTri2D::rangeMax(Axis which) const
{
    assert(which != Rappture::zaxis);
    return _max[which];
}

CellTri2D
MeshTri2D::locate(const Node2D& node) const
{
    MeshTri2D* nonconst = (MeshTri2D*)this;
    CellTri2D cell = _lastLocate;

    if (cell.isNull() && _celllist.size() > 0) {
        Tri2D& tri = nonconst->_celllist[0];
        cell = CellTri2D(0, &nonconst->_nodelist[tri.nodes[0]],
                            &nonconst->_nodelist[tri.nodes[1]],
                            &nonconst->_nodelist[tri.nodes[2]]);
    }

    while (!cell.isNull()) {
        double phi[3];

        // compute barycentric coords
        // if all are >= 0, then this tri contains node
        // Check against -DBL_EPSILON to handle precision issues at boundaries
        cell.barycentrics(node, phi);
        if (phi[0] > -DBL_EPSILON &&
            phi[1] > -DBL_EPSILON &&
            phi[2] > -DBL_EPSILON) {
            break;
        }

        // find the smallest (most negative) coord phi, and search that dir
        int dir = 0;
        for (int i=1; i <= 2; i++) {
            if (phi[i] < phi[dir]) {
                dir = i;
            }
        }

        Tri2D& tri = nonconst->_celllist[ cell.cellId() ];
        int neighborId = tri.neighbors[dir];
        if (neighborId < 0) {
            cell.clear();
            return cell;
        }

        Tri2D& tri2 = nonconst->_celllist[neighborId];
        Node2D *n1Ptr = &nonconst->_nodelist[tri2.nodes[0]];
        Node2D *n2Ptr = &nonconst->_nodelist[tri2.nodes[1]];
        Node2D *n3Ptr = &nonconst->_nodelist[tri2.nodes[2]];
        cell = CellTri2D(neighborId, n1Ptr, n2Ptr, n3Ptr);
    }

    nonconst->_lastLocate = cell;
    return cell;
}

Ptr<Serializable>
MeshTri2D::create()
{
    return Ptr<Serializable>( (Serializable*) new MeshTri2D() );
}

void
MeshTri2D::serialize_A(SerialBuffer& buffer) const
{
}

Outcome
deserialize_A(SerialBuffer& buffer)
{
    Outcome status;
    return status;
}

Node2D*
MeshTri2D::_getNodeById(int nodeId)
{
    MeshTri2D* nonconst = (MeshTri2D*)this;
    Node2D *rptr = NULL;
    nonconst->_rebuildNodeIdMap();

    if ((unsigned int)nodeId < _id2node.size()) {
        int n = _id2node[nodeId];
        if (n >= 0) {
            return &_nodelist[n];
        }
    }
    return rptr;
}

void
MeshTri2D::_rebuildNodeIdMap()
{
    if (_id2nodeDirty) {
        _min[0] = _min[1] = NAN;
        _max[0] = _max[1] = NAN;

        // figure out how big the _id2node array should be
        int maxId = -1;
        std::vector<Node2D>::iterator n = _nodelist.begin();
        while (n != _nodelist.end()) {
            if (n->id() > maxId) {
                maxId = n->id();
            }
            ++n;
        }
        if (maxId > 0) {
            _id2node.assign(maxId+1, -1);
        }

        // scan through and map id -> node index
        int i = 0;
        n = _nodelist.begin();
        while (n != _nodelist.end()) {
            _id2node[n->id()] = i++;

            if (isnan(_min[0])) {
                _min[0] = _max[0] = n->x();
                _min[1] = _max[1] = n->y();
            } else {
                if (n->x() < _min[0]) { _min[0] = n->x(); }
                if (n->x() > _max[0]) { _max[0] = n->x(); }
                if (n->y() < _min[1]) { _min[1] = n->y(); }
                if (n->y() > _max[1]) { _max[1] = n->y(); }
            }
            ++n;
        }
        _id2nodeDirty = 0;
    }
}
