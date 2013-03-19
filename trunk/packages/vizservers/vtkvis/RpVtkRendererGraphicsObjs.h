/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_RENDERER_GRAPHICS_OBJS_H__
#define __RAPPTURE_VTKVIS_RENDERER_GRAPHICS_OBJS_H__

#include <tr1/unordered_map>
#include <typeinfo>

// Use GCC/G++ specific name demangling
#define DEMANGLE

#ifdef DEMANGLE
#include <cstdlib>
#include <cxxabi.h>
#define GO_TRACE(go, format, ...)                                            \
do {                                                                         \
    int status;                                                              \
    char * typeName = abi::__cxa_demangle(typeid(go).name(), 0, 0, &status); \
    TRACE("%s " format, typeName, __VA_ARGS__);                              \
    free(typeName);                                                          \
} while (0)

#define GO_ERROR(go, format, ...)                                            \
do {                                                                         \
    int status;                                                              \
    char * typeName = abi::__cxa_demangle(typeid(go).name(), 0, 0, &status); \
    ERROR("%s " format, typeName, __VA_ARGS__);                              \
    free(typeName);                                                          \
} while (0)
#else
#define GO_TRACE(go, format, ...) TRACE("%s " format, typeid(go).name(), __VA_ARGS__);
#define GO_ERROR(go, format, ...) ERROR("%s " format, typeid(go).name(), __VA_ARGS__);
#endif

#include "RpVtkRenderer.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Look up graphics object by name
 */
template<class GraphicsObject>
GraphicsObject *Renderer::getGraphicsObject(const DataSetId& id)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap =
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator
        itr = hashmap.find(id);

    if (itr == hashmap.end()) {
#ifdef DEBUG
        GO_TRACE(GraphicsObject, "not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Delete graphics object by name
 */
template<class GraphicsObject>
void Renderer::deleteGraphicsObject(const DataSetId& id)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap =
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting %s for %s", itr->second->getClassName(), id.c_str());

    do {
        GraphicsObject *gobj = itr->second;
        if (gobj->getProp())
            _renderer->RemoveViewProp(gobj->getProp());
        if (gobj->getOverlayProp())
            _renderer->RemoveViewProp(gobj->getOverlayProp());
        delete gobj;

        itr = hashmap.erase(itr);
    } while (doAll && itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Delete all graphics objects from renderer
 */
template<class GraphicsObject>
void Renderer::deleteAllGraphicsObjects()
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap =
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    itr = hashmap.begin();
    if (itr == hashmap.end())
        return;

    TRACE("Deleting all %s objects", itr->second->getClassName());

    for (; itr != hashmap.end(); ++itr) {
        delete itr->second;
    }
    hashmap.clear();
}

/**
 * \brief Add a graphics objects to the renderer's scene
 */
template<class GraphicsObject>
bool Renderer::addGraphicsObject(const DataSetId& id)
{
    DataSetHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _dataSets.begin();
    } else {
        itr = _dataSets.find(id);
    }
    if (itr == _dataSets.end()) {
        ERROR("Unknown dataset %s", id.c_str());
        return false;
    }

    do {
        DataSet *ds = itr->second;
        const DataSetId& dsID = ds->getName();

        GraphicsObject *gobj;
        if ((gobj = getGraphicsObject<GraphicsObject>(dsID)) != NULL) {
            WARN("Replacing existing %s %s", gobj->getClassName(), dsID.c_str());
            deleteGraphicsObject<GraphicsObject>(dsID);
        }

        gobj = new GraphicsObject();
 
        gobj->setDataSet(ds, this);

        if (gobj->getProp() == NULL &&
            gobj->getOverlayProp() == NULL) {
            delete gobj;
            return false;
        } else {
            if (gobj->getProp())
                _renderer->AddViewProp(gobj->getProp());
            if (gobj->getOverlayProp())
                _renderer->AddViewProp(gobj->getOverlayProp());
        }

        getGraphicsObjectHashmap<GraphicsObject>()[dsID] = gobj;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTransform(trans);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectOrientation(const DataSetId& id, double quat[4])
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(quat);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the prop orientation with a rotation about an axis
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectOrientation(const DataSetId& id, double angle, double axis[3])
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectPosition(const DataSetId& id, double pos[3])
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPosition(pos);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling based on a 2D aspect ratio
 *
 * \param id The name of the DataSet
 * \param aspect The aspect ratio (width/height), zero means native aspect
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectAspect(const DataSetId& id, double aspect)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAspect(aspect);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the prop scaling
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectScale(const DataSetId& id, double scale[3])
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set visibility of VtkGraphicsObject for the given DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectVisibility(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap =
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setVisibility(state);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the volume slice
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectVolumeSlice(const DataSetId& id, Axis axis, double ratio)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }

    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->selectVolumeSlice(axis, ratio);
     } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}


/**
 * \brief Set the RGB actor color for the specified DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectColor(const DataSetId& id, float color[3])
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColor(color);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off edges for the given DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectEdgeVisibility(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeVisibility(state);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the RGB isosurface edge color for the specified DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectEdgeColor(const DataSetId& id, float color[3])
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeColor(color);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set the isosurface edge width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectEdgeWidth(const DataSetId& id, float edgeWidth)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set ambient lighting/shading coefficient for the specified DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectAmbient(const DataSetId& id, double coeff)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAmbient(coeff);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set diffuse lighting/shading coefficient for the specified DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectDiffuse(const DataSetId& id, double coeff)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setDiffuse(coeff);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set specular lighting/shading coefficient and power for the specified DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectSpecular(const DataSetId& id, double coeff, double power)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSpecular(coeff, power);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Turn actor lighting on/off for the specified DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectLighting(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set opacity of VtkGraphicsObject for the given DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectOpacity(const DataSetId& id, double opacity)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOpacity(opacity);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set the point size for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide points, 
 * this function may not have an effect.
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectPointSize(const DataSetId& id, float size)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setPointSize(size);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set wireframe actor rendering for the specified DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectWireframe(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWireframe(state);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a graphics object for the given DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectInterpolateBeforeMapping(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }

    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setInterpolateBeforeMapping(state);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Associate an existing named color map with a graphics object for the given DataSet
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        if (itr == hashmap.end())
            return;
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }

    if (itr == hashmap.end()) {
        GO_ERROR(GraphicsObject, "not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        GO_TRACE(GraphicsObject, "Set color map: %s for dataset %s",
                 colorMapId.c_str(),
                 itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Notify graphics object that color map has changed
 */
template<class GraphicsObject>
void Renderer::updateGraphicsObjectColorMap(ColorMap *cmap)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        if (itr->second->getColorMap() == cmap) {
            itr->second->updateColorMap();
            _needsRedraw = true;
        }
    }
}

/**
 * \brief Check if a color map is in use by a graphics object
 */
template<class GraphicsObject>
bool Renderer::graphicsObjectColorMapUsed(ColorMap *cmap)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    return false;
}

/**
 * \brief Compute union of bounds and GO's bounds
 */
template<class GraphicsObject>
void Renderer::mergeGraphicsObjectBounds(double *bounds, bool onlyVisible)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getBounds());
#ifdef DEBUG
        double *bPtr = itr->second->getBounds();
        assert(bPtr != NULL);
        GO_TRACE(GraphicsObject,
                 "%s bounds: %g %g %g %g %g %g",
                 itr->first.c_str(),
                 bPtr[0], bPtr[1], bPtr[2], bPtr[3], bPtr[4], bPtr[5]);
#endif
    }
}

/**
 * \brief Compute union of bounds and GO's unscaled bounds
 *
 * Unscaled bounds are the bounds of the object before any actor scaling is
 * applied
 */
template<class GraphicsObject>
void Renderer::mergeGraphicsObjectUnscaledBounds(double *bounds, bool onlyVisible)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getUnscaledBounds());
#ifdef DEBUG
        double *bPtr = itr->second->getUnscaledBounds();
        assert(bPtr != NULL);
        GO_TRACE(GraphicsObject,
                 "%s bounds: %g %g %g %g %g %g",
                 itr->first.c_str(),
                 bPtr[0], bPtr[1], bPtr[2], bPtr[3], bPtr[4], bPtr[5]);
#endif
    }
}

/**
 * \brief Notify object that field value ranges have changed
 */
template<class GraphicsObject>
void Renderer::updateGraphicsObjectFieldRanges()
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        itr->second->updateRanges(this);
    }
}

/**
 * \brief Pass global clip planes to graphics object
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectClippingPlanes(vtkPlaneCollection *planes)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        itr->second->setClippingPlanes(planes);
    }
}

/**
 * \brief Set the prop scaling based on a 2D aspect ratio
 *
 * This method sets the aspect on all graphics objects of a given type
 *
 * \param aspectRatio The aspect ratio (width/height), zero means native aspect
 */
template<class GraphicsObject>
void Renderer::setGraphicsObjectAspect(double aspectRatio)
{
    std::tr1::unordered_map<DataSetId, GraphicsObject *>& hashmap = 
        getGraphicsObjectHashmap<GraphicsObject>();
    typename std::tr1::unordered_map<DataSetId, GraphicsObject *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        itr->second->setAspect(aspectRatio);
    }

    sceneBoundsChanged();
    _needsRedraw = true;
}

}
}

#endif
