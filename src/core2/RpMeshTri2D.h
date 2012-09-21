/*
 * ----------------------------------------------------------------------
 *  Rappture::MeshTri2D
 *    This is a non-uniform, triangular mesh for 2-dimensional
 *    structures.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RAPPTURE_MESHTRI2D_H
#define RAPPTURE_MESHTRI2D_H

#include "RpNode.h"
#include "RpSerializable.h"

namespace Rappture {

class CellTri2D {
public:
    CellTri2D();
    CellTri2D(int cellId, Node2D* n1Ptr, Node2D* n2Ptr, Node2D* n3Ptr);
    CellTri2D(const CellTri2D& cell);
    CellTri2D& operator=(const CellTri2D& cell);

    int cellId() const;
    int nodeId(int n) const;
    double x(int n) const;
    double y(int n) const;

    int isNull() const;
    int isOutside() const;
    void clear();

    void barycentrics(const Node2D& node, double* phi) const;

private:
    int _cellId;
    const Node2D* _nodes[3];
};

class Tri2D {
public:
    Tri2D();
    Tri2D(int n1, int n2, int n3);

    int nodes[3];
    int neighbors[3];
};

class MeshTri2D : public Serializable {
public:
    MeshTri2D();
    MeshTri2D(const MeshTri2D& mesh);
    MeshTri2D& operator=(const MeshTri2D& mesh);
    virtual ~MeshTri2D();

    virtual Node2D& addNode(const Node2D& node);
    virtual void addCell(int nId1, int nId2, int nId3);
    virtual void addCell(const Node2D& n1, const Node2D& n2, const Node2D& n3);
    virtual MeshTri2D& clear();

    virtual int sizeNodes() const;
    virtual Node2D& atNode(int pos);

    virtual int sizeCells() const;
    virtual CellTri2D atCell(int pos);

    virtual double rangeMin(Axis which) const;
    virtual double rangeMax(Axis which) const;

    virtual CellTri2D locate(const Node2D& node) const;

    // required for base class Serializable:
    const char* serializerType() const { return "MeshTri2D"; }
    char serializerVersion() const { return 'A'; }

    static Ptr<Serializable> create();
    void serialize_A(SerialBuffer& buffer) const;
    Outcome deserialize_A(SerialBuffer& buffer);

protected:
    virtual Node2D* _getNodeById(int nodeId);
    virtual void _rebuildNodeIdMap();

private:
    std::vector<Node2D> _nodelist;  // list of all nodes
    int _counter;                   // auto counter for node IDs
    double _min[2];                 // min values for (x,y)
    double _max[2];                 // max values for (x,y)

    std::vector<Tri2D> _celllist;   // list of all triangular cells

    // use this structure and map to find neighbors for all triangles
    typedef struct Edge2D {
        int fromNode;  // minimum node ID
        int toNode;    // maximum node ID
        Edge2D() : fromNode(-1), toNode(-1) {}
        int operator<(const Edge2D& edge) const {
            if (fromNode < edge.fromNode) {
                return 1;
            } else if (fromNode == edge.fromNode && toNode < edge.toNode) {
                return 1;
            }
            return 0;
        }
    } Edge2D;
    typedef struct Neighbor2D {
        int triId;     // ID of triangle containing edge
        int index;     // index (0/1/2) for point opposite edge
        Neighbor2D() : triId(-1), index(-1) {}
    } Neighbor2D;
    typedef std::map<Edge2D, Neighbor2D> Edge2Neighbor;
    Edge2Neighbor _edge2neighbor;   // maps tri edge to tri containing it

    int _id2nodeDirty;              // non-zero => _id2node needs to be rebuilt
    std::vector<int> _id2node;      // maps node Id => index in _nodelist

    CellTri2D _lastLocate;          // last result from locate() operation

    // methods for serializing/deserializing version 'A'
    static SerialConversion versionA;
};

} // namespace Rappture

#endif
