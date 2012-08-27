/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_GROUP_H__
#define __RAPPTURE_VTKVIS_GROUP_H__

#include <tr1/unordered_map>

#include <vtkProp3D.h>

#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Collection of shapes with grouping
 */
class Group : public VtkGraphicsObject {
public:
    typedef std::string NodeId;
    typedef std::tr1::unordered_map<NodeId, VtkGraphicsObject *> NodeHashmap;

    Group(const NodeId& name);
    virtual ~Group();

    virtual const char *getClassName() const 
    {
        return "Group";
    }

    void addChild(const NodeId& name, VtkGraphicsObject *obj);

    VtkGraphicsObject *getChild(const NodeId& name);

    void removeChild(const NodeId& name);

private:
    Group();

    virtual void initProp();

    virtual void update();

    NodeHashmap _nodes;
};

}
}

#endif
