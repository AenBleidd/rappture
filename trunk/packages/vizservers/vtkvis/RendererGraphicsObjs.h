/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_RENDERER_GRAPHICS_OBJS_H
#define VTKVIS_RENDERER_GRAPHICS_OBJS_H

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

#include "Renderer.h"

namespace VtkVis {

/**
 * \brief Look up graphics object by name
 */
template<class T>
T *Renderer::getGraphicsObject(const DataSetId& id)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap =
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator
        itr = hashmap.find(id);

    if (itr == hashmap.end()) {
#ifdef DEBUG
        GO_TRACE(T, "not found: %s", id.c_str());
#endif
        return NULL;
    } else
        return itr->second;
}

/**
 * \brief Delete graphics object by name
 */
template<class T>
void Renderer::deleteGraphicsObject(const DataSetId& id)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap =
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting %s for %s", itr->second->getClassName(), id.c_str());

    do {
        T *gobj = itr->second;
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
template<class T>
void Renderer::deleteAllGraphicsObjects()
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap =
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
template<class T>
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

        T *gobj;
        if ((gobj = getGraphicsObject<T>(dsID)) != NULL) {
            WARN("Replacing existing %s %s", gobj->getClassName(), dsID.c_str());
            deleteGraphicsObject<T>(dsID);
        }

        gobj = new T();
 
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

        getGraphicsObjectHashmap<T>()[dsID] = gobj;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the prop orientation with a quaternion
 */
template<class T>
void Renderer::setGraphicsObjectTransform(const DataSetId& id, vtkMatrix4x4 *trans)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectOrientation(const DataSetId& id, double quat[4])
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectOrientation(const DataSetId& id, double angle, double axis[3])
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientation(angle, axis);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the prop origin (center of rotation)
 */
template<class T>
void Renderer::setGraphicsObjectOrigin(const DataSetId& id, double origin[3])
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrigin(origin);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the prop position in world coords
 */
template<class T>
void Renderer::setGraphicsObjectPosition(const DataSetId& id, double pos[3])
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectAspect(const DataSetId& id, double aspect)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectScale(const DataSetId& id, double scale[3])
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScale(scale);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set visibility of GraphicsObject for the given DataSet
 */
template<class T>
void Renderer::setGraphicsObjectVisibility(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap =
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectVolumeSlice(const DataSetId& id, Axis axis, double ratio)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectColor(const DataSetId& id, float color[3])
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColor(color);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set cull face for graphics object
 */
template<class T>
void Renderer::setGraphicsObjectCullFace(const DataSetId& id,
                                         GraphicsObject::CullFace state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setCullFace(state);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set face culling for graphics object
 */
template<class T>
void Renderer::setGraphicsObjectCulling(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setCulling(state);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off edges for the given DataSet
 */
template<class T>
void Renderer::setGraphicsObjectEdgeVisibility(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectEdgeColor(const DataSetId& id, float color[3])
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectEdgeWidth(const DataSetId& id, float edgeWidth)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setEdgeWidth(edgeWidth);
    } while (doAll && ++itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Flip normals and front/back faces of the shape geometry
 */
template<class T>
void Renderer::setGraphicsObjectFlipNormals(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->flipNormals(state);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set ambient lighting/shading coefficient for the specified DataSet
 */
template<class T>
void Renderer::setGraphicsObjectAmbient(const DataSetId& id, double coeff)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectDiffuse(const DataSetId& id, double coeff)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setDiffuse(coeff);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set the shading of the object (flat or smooth)
 *
 * Currently Phong shading is not implemented
 */
template<class T>
void Renderer::setGraphicsObjectShadingModel(const DataSetId& id,
                                             GraphicsObject::ShadingModel state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setShadingModel(state);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set specular lighting/shading coefficient and power for the specified DataSet
 */
template<class T>
void Renderer::setGraphicsObjectSpecular(const DataSetId& id, double coeff, double power)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectLighting(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLighting(state);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Set opacity of GraphicsObject for the given DataSet
 */
template<class T>
void Renderer::setGraphicsObjectOpacity(const DataSetId& id, double opacity)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectPointSize(const DataSetId& id, float size)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectWireframe(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectInterpolateBeforeMapping(const DataSetId& id, bool state)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
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
template<class T>
void Renderer::setGraphicsObjectColorMap(const DataSetId& id, const ColorMapId& colorMapId)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
        GO_ERROR(T, "not found: %s", id.c_str());
        return;
    }

    ColorMap *cmap = getColorMap(colorMapId);
    if (cmap == NULL && colorMapId.compare("none") != 0) {
        ERROR("Unknown colormap: %s", colorMapId.c_str());
        return;
    }

    do {
        GO_TRACE(T, "Set color map: %s for dataset %s",
                 colorMapId.c_str(),
                 itr->second->getDataSet()->getName().c_str());

        itr->second->setColorMap(cmap);
    } while (doAll && ++itr != hashmap.end());

    _needsRedraw = true;
}

/**
 * \brief Notify graphics object that color map has changed
 */
template<class T>
void Renderer::updateGraphicsObjectColorMap(ColorMap *cmap)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
template<class T>
bool Renderer::graphicsObjectColorMapUsed(ColorMap *cmap)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        if (itr->second->getColorMap() == cmap)
            return true;
    }
    return false;
}

/**
 * \brief Compute union of bounds and GO's bounds
 */
template<class T>
void Renderer::mergeGraphicsObjectBounds(double *bounds, bool onlyVisible)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getBounds());
#ifdef DEBUG
        double *bPtr = itr->second->getBounds();
        assert(bPtr != NULL);
        GO_TRACE(T,
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
template<class T>
void Renderer::mergeGraphicsObjectUnscaledBounds(double *bounds, bool onlyVisible)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        if ((!onlyVisible || itr->second->getVisibility()) &&
            itr->second->getProp() != NULL)
            mergeBounds(bounds, bounds, itr->second->getUnscaledBounds());
#ifdef DEBUG
        double *bPtr = itr->second->getUnscaledBounds();
        assert(bPtr != NULL);
        GO_TRACE(T,
                 "%s bounds: %g %g %g %g %g %g",
                 itr->first.c_str(),
                 bPtr[0], bPtr[1], bPtr[2], bPtr[3], bPtr[4], bPtr[5]);
#endif
    }
}

/**
 * \brief Notify object that field value ranges have changed
 */
template<class T>
void Renderer::updateGraphicsObjectFieldRanges()
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        itr->second->updateRanges(this);
    }
}

/**
 * \brief Pass global clip planes to graphics object
 */
template<class T>
void Renderer::setGraphicsObjectClippingPlanes(vtkPlaneCollection *planes)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

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
template<class T>
void Renderer::setGraphicsObjectAspect(double aspectRatio)
{
    std::tr1::unordered_map<DataSetId, T *>& hashmap = 
        getGraphicsObjectHashmap<T>();
    typename std::tr1::unordered_map<DataSetId, T *>::iterator itr;

    for (itr = hashmap.begin(); itr != hashmap.end(); ++itr) {
        itr->second->setAspect(aspectRatio);
    }

    sceneBoundsChanged();
    _needsRedraw = true;
}

}

#endif
