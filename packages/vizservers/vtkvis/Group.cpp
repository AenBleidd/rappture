/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include "Group.h"
#include "Trace.h"

using namespace VtkVis;

Group::Group()
{
}

Group::~Group()
{
}

/**
 * \brief Create and initialize VTK Props to render the Group
 */
void Group::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkAssembly>::New();
    }
}

void Group::update()
{
    initProp();
}

void Group::addChild(const NodeId& name, GraphicsObject *obj)
{
    if (obj == NULL)
        return;
    if (obj->getProp3D() != NULL) {
        getAssembly()->AddPart(obj->getProp3D());
    }
    _nodes[name] = obj;
}

GraphicsObject *Group::getChild(const NodeId& name)
{
    NodeHashmap::iterator itr = _nodes.find(name);
    if (itr == _nodes.end()) {
        return NULL;
    } else {
        return itr->second;
    }
}

GraphicsObject *Group::removeChild(const NodeId& name)
{
    NodeHashmap::iterator itr = _nodes.find(name);
    if (itr == _nodes.end()) {
        ERROR("Node not found: '%s'", name.c_str());
        return NULL;
    }
    GraphicsObject *obj = itr->second;
    if (obj->getProp3D() != NULL) {
        getAssembly()->RemovePart(obj->getProp3D());
    }
    _nodes.erase(itr);
    return obj;
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Group::setClippingPlanes(vtkPlaneCollection *planes)
{
    for (NodeHashmap::iterator itr = _nodes.begin();
         itr != _nodes.end(); ++itr) {
        itr->second->setClippingPlanes(planes);
    }
}
