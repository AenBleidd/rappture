/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_GROUP_H
#define VTKVIS_GROUP_H

#include <tr1/unordered_map>

#include <vtkProp3D.h>

#include "VtkGraphicsObject.h"

namespace VtkVis {

/**
 * \brief Collection of shapes with grouping
 */
class Group : public VtkGraphicsObject {
public:
    typedef std::string NodeId;
    typedef std::tr1::unordered_map<NodeId, VtkGraphicsObject *> NodeHashmap;

    Group();
    virtual ~Group();

    virtual const char *getClassName() const 
    {
        return "Group";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer)
    {
        assert(dataSet == NULL);
        update();
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void addChild(const NodeId& name, VtkGraphicsObject *obj);

    VtkGraphicsObject *getChild(const NodeId& name);

    void getChildren(std::vector<VtkGraphicsObject *>& children)
    {
        for (NodeHashmap::iterator itr = _nodes.begin();
             itr != _nodes.end(); ++itr) {
            children.push_back(itr->second);
        }
    }

    void removeChild(const NodeId& name);

private:
    virtual void initProp();

    virtual void update();

    NodeHashmap _nodes;
};

}

#endif
