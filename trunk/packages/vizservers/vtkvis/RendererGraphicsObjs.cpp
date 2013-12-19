/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstring>
#include <typeinfo>
#include <vector>

#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkCharArray.h>
#include <vtkDataSetReader.h>

#include "RendererGraphicsObjs.h"
#include "Renderer.h"
#include "DataSet.h"
#include "Box.h"
#include "Contour2D.h"
#include "Contour3D.h"
#include "Cutplane.h"
#include "Glyphs.h"
#include "HeightMap.h"
#include "LIC.h"
#include "Line.h"
#include "Molecule.h"
#include "Parallelepiped.h"
#include "PolyData.h"
#include "PseudoColor.h"
#include "Sphere.h"
#include "Streamlines.h"
#include "Volume.h"
#include "ColorMap.h"
#include "Trace.h"

// Template specializations
namespace VtkVis {

template<>
Renderer::ArcHashmap &
Renderer::getGraphicsObjectHashmap<Arc>()
{ return _arcs; }

template<>
Renderer::ArrowHashmap &
Renderer::getGraphicsObjectHashmap<Arrow>()
{ return _arrows; }

template<>
Renderer::BoxHashmap &
Renderer::getGraphicsObjectHashmap<Box>()
{ return _boxes; }

template<>
Renderer::ConeHashmap &
Renderer::getGraphicsObjectHashmap<Cone>()
{ return _cones; }

template<>
Renderer::Contour2DHashmap &
Renderer::getGraphicsObjectHashmap<Contour2D>()
{ return _contour2Ds; }

template<>
Renderer::Contour3DHashmap &
Renderer::getGraphicsObjectHashmap<Contour3D>()
{ return _contour3Ds; }

template<>
Renderer::CutplaneHashmap &
Renderer::getGraphicsObjectHashmap<Cutplane>()
{ return _cutplanes; }

template<>
Renderer::CylinderHashmap &
Renderer::getGraphicsObjectHashmap<Cylinder>()
{ return _cylinders; }

template<>
Renderer::DiskHashmap &
Renderer::getGraphicsObjectHashmap<Disk>()
{ return _disks; }

template<>
Renderer::GlyphsHashmap &
Renderer::getGraphicsObjectHashmap<Glyphs>()
{ return _glyphs; }

template<>
Renderer::GroupHashmap &
Renderer::getGraphicsObjectHashmap<Group>()
{ return _groups; }

template<>
Renderer::HeightMapHashmap &
Renderer::getGraphicsObjectHashmap<HeightMap>()
{ return _heightMaps; }

template<>
Renderer::ImageHashmap &
Renderer::getGraphicsObjectHashmap<Image>()
{ return _images; }

template<>
Renderer::ImageCutplaneHashmap &
Renderer::getGraphicsObjectHashmap<ImageCutplane>()
{ return _imageCutplanes; }

template<>
Renderer::LICHashmap &
Renderer::getGraphicsObjectHashmap<LIC>()
{ return _lics; }

template<>
Renderer::LineHashmap &
Renderer::getGraphicsObjectHashmap<Line>()
{ return _lines; }

template<>
Renderer::MoleculeHashmap &
Renderer::getGraphicsObjectHashmap<Molecule>()
{ return _molecules; }

template<>
Renderer::OutlineHashmap &
Renderer::getGraphicsObjectHashmap<Outline>()
{ return _outlines; }

template<>
Renderer::ParallelepipedHashmap &
Renderer::getGraphicsObjectHashmap<Parallelepiped>()
{ return _parallelepipeds; }

template<>
Renderer::PolyDataHashmap &
Renderer::getGraphicsObjectHashmap<PolyData>()
{ return _polyDatas; }

template<>
Renderer::PolygonHashmap &
Renderer::getGraphicsObjectHashmap<Polygon>()
{ return _polygons; }

template<>
Renderer::PseudoColorHashmap &
Renderer::getGraphicsObjectHashmap<PseudoColor>()
{ return _pseudoColors; }

template<>
Renderer::SphereHashmap &
Renderer::getGraphicsObjectHashmap<Sphere>()
{ return _spheres; }

template<>
Renderer::StreamlinesHashmap &
Renderer::getGraphicsObjectHashmap<Streamlines>()
{ return _streamlines; }

template<>
Renderer::Text3DHashmap &
Renderer::getGraphicsObjectHashmap<Text3D>()
{ return _text3Ds; }

template<>
Renderer::VolumeHashmap &
Renderer::getGraphicsObjectHashmap<Volume>()
{ return _volumes; }

template<>
Renderer::WarpHashmap &
Renderer::getGraphicsObjectHashmap<Warp>()
{ return _warps; }

#if 0
template Arc *Renderer::getGraphicsObject(const DataSetId&);
template Arrow *Renderer::getGraphicsObject(const DataSetId&);
template Box *Renderer::getGraphicsObject(const DataSetId&);
template Cone *Renderer::getGraphicsObject(const DataSetId&);
template Cylinder *Renderer::getGraphicsObject(const DataSetId&);
template Disk *Renderer::getGraphicsObject(const DataSetId&);
template Group *Renderer::getGraphicsObject(const DataSetId&);
template Line *Renderer::getGraphicsObject(const DataSetId&);
template Parallelepiped *Renderer::getGraphicsObject(const DataSetId&);
template Polygon *Renderer::getGraphicsObject(const DataSetId&);
template Sphere *Renderer::getGraphicsObject(const DataSetId&);
template Text3D *Renderer::getGraphicsObject(const DataSetId&);
#endif

template <>
void Renderer::deleteGraphicsObject<Group>(const DataSetId& id)
{
    GroupHashmap& hashmap = getGraphicsObjectHashmap<Group>();
    GroupHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = hashmap.begin();
        doAll = true;
    } else {
        itr = hashmap.find(id);
    }
    if (itr == hashmap.end()) {
        ERROR("Group not found: %s", id.c_str());
        return;
    }

    TRACE("Deleting Group: %s", id.c_str());

    do {
        Group *gobj = itr->second;
        if (gobj->getProp())
            _renderer->RemoveViewProp(gobj->getProp());
        if (gobj->getOverlayProp())
            _renderer->RemoveViewProp(gobj->getOverlayProp());

        std::vector<GraphicsObject *> children;
        gobj->getChildren(children);

        // Un-grouping children
        for (std::vector<GraphicsObject *>::iterator citr = children.begin();
             citr != children.end(); ++citr) {
            if ((*citr)->getProp())
                _renderer->AddViewProp((*citr)->getProp());
            if ((*citr)->getOverlayProp())
                _renderer->AddViewProp((*citr)->getOverlayProp());
        }

        delete gobj;

        itr = hashmap.erase(itr);
    } while (doAll && itr != hashmap.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

}

using namespace VtkVis;

GraphicsObject *
Renderer::getGenericGraphicsObject(const DataSetId& id)
{
    GraphicsObject *gobj = NULL;

    if ((gobj = getGraphicsObject<Arc>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Arrow>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Box>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Cone>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Cylinder>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Disk>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Line>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Parallelepiped>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Polygon>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Sphere>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Text3D>(id)) != NULL) {
        return gobj;
    }
    //
    if ((gobj = getGraphicsObject<Contour2D>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Contour3D>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Cutplane>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Glyphs>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<HeightMap>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Image>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<ImageCutplane>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<LIC>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Molecule>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Outline>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<PolyData>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<PseudoColor>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Streamlines>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Volume>(id)) != NULL) {
        return gobj;
    }
    if ((gobj = getGraphicsObject<Warp>(id)) != NULL) {
        return gobj;
    }
    //
    if ((gobj = getGraphicsObject<Group>(id)) != NULL) {
        return gobj;
    }

    return NULL;
}

/**
 * \brief Create a new Arc and associate it with an ID
 */
bool Renderer::addArc(const DataSetId& id,
                      double center[3],
                      double pt1[3],
                      double normal[3],
                      double angle)
{
    Arc *gobj;
    if ((gobj = getGraphicsObject<Arc>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Arc>(id);
    }

    gobj = new Arc();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setCenter(center);
    gobj->setStartPoint(pt1);
    gobj->setNormal(normal);
    gobj->setAngle(angle);

    getGraphicsObjectHashmap<Arc>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set Arc resolution
 */
void Renderer::setArcResolution(const DataSetId& id, int res)
{
    ArcHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _arcs.begin();
        if (itr == _arcs.end())
            return;
        doAll = true;
    } else {
        itr = _arcs.find(id);
    }
    if (itr == _arcs.end()) {
        ERROR("Arc not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setResolution(res);
    } while (doAll && ++itr != _arcs.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Create a new Arrow and associate it with an ID
 */
bool Renderer::addArrow(const DataSetId& id, double tipRadius,
                        double shaftRadius, double tipLength,
                        bool flipNormals)
{
    Arrow *gobj;
    if ((gobj = getGraphicsObject<Arrow>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Arrow>(id);
    }

    gobj = new Arrow();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setRadii(tipRadius, shaftRadius);
    gobj->setTipLength(tipLength);
    if (flipNormals)
        gobj->flipNormals(flipNormals);

    getGraphicsObjectHashmap<Arrow>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set Arrow resolution
 */
void Renderer::setArrowResolution(const DataSetId& id, int tipRes, int shaftRes)
{
    ArrowHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _arrows.begin();
        if (itr == _arrows.end())
            return;
        doAll = true;
    } else {
        itr = _arrows.find(id);
    }
    if (itr == _arrows.end()) {
        ERROR("Arrow not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setResolution(tipRes, shaftRes);
    } while (doAll && ++itr != _arrows.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Create a new Box and associate it with an ID
 */
bool Renderer::addBox(const DataSetId& id,
                      double xLen, double yLen, double zLen,
                      bool flipNormals)
{
    Box *gobj;
    if ((gobj = getGraphicsObject<Box>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Box>(id);
    }

    gobj = new Box();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setSize(xLen, yLen, zLen);
    if (flipNormals)
        gobj->flipNormals(flipNormals);

    getGraphicsObjectHashmap<Box>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new Cone and associate it with an ID
 */
bool Renderer::addCone(const DataSetId& id, double radius, double height,
                       bool cap, bool flipNormals)
{
    Cone *gobj;
    if ((gobj = getGraphicsObject<Cone>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Cone>(id);
    }

    gobj = new Cone();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setRadius(radius);
    gobj->setHeight(height);
    gobj->setCapping(cap);
    if (flipNormals)
        gobj->flipNormals(flipNormals);

    getGraphicsObjectHashmap<Cone>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set Cone resolution
 */
void Renderer::setConeResolution(const DataSetId& id, int res)
{
    ConeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _cones.begin();
        if (itr == _cones.end())
            return;
        doAll = true;
    } else {
        itr = _cones.find(id);
    }
    if (itr == _cones.end()) {
        ERROR("Cone not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setResolution(res);
    } while (doAll && ++itr != _cones.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Create a new Contour2D and associate it with the named DataSet
 */
bool Renderer::addContour2D(const DataSetId& id, int numContours)
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

        if (getGraphicsObject<Contour2D>(dsID)) {
            WARN("Replacing existing Contour2D %s", dsID.c_str());
            deleteGraphicsObject<Contour2D>(dsID);
        }

        Contour2D *contour = new Contour2D(numContours);
 
        contour->setDataSet(ds, this);

        if (contour->getProp() == NULL) {
            delete contour;
            return false;
        } else {
            _renderer->AddViewProp(contour->getProp());
        }

        _contour2Ds[dsID] = contour;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new Contour2D and associate it with the named DataSet
 */
bool Renderer::addContour2D(const DataSetId& id, const std::vector<double>& contours)
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

        if (getGraphicsObject<Contour2D>(dsID)) {
            WARN("Replacing existing Contour2D %s", dsID.c_str());
            deleteGraphicsObject<Contour2D>(dsID);
        }

        Contour2D *contour = new Contour2D(contours);

        contour->setDataSet(ds, this);

        if (contour->getProp() == NULL) {
            delete contour;
            return false;
        } else {
            _renderer->AddViewProp(contour->getProp());
        }

        _contour2Ds[dsID] = contour;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the number of equally spaced contour isolines for the given DataSet
 */
void Renderer::setContour2DNumContours(const DataSetId& id, int numContours)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setNumContours(numContours);
    } while (doAll && ++itr != _contour2Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the number of equally spaced isosurfaces for the given DataSet
 */
void Renderer::setContour2DContourField(const DataSetId& id, const char *fieldName)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourField(fieldName);
     } while (doAll && ++itr != _contour2Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set a list of isovalues for the given DataSet
 */
void Renderer::setContour2DContourList(const DataSetId& id, const std::vector<double>& contours)
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourList(contours);
    } while (doAll && ++itr != _contour2Ds.end());

    sceneBoundsChanged();
     _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setContour2DColorMode(const DataSetId& id,
                                     Contour2D::ColorMode mode,
                                     DataSet::DataAttributeType type,
                                     const char *name, double range[2])
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        if (itr == _contour2Ds.end())
            return;
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _contour2Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setContour2DColorMode(const DataSetId& id,
                                     Contour2D::ColorMode mode,
                                     const char *name, double range[2])
{
    Contour2DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour2Ds.begin();
        if (itr == _contour2Ds.end())
            return;
        doAll = true;
    } else {
        itr = _contour2Ds.find(id);
    }
    if (itr == _contour2Ds.end()) {
        ERROR("Contour2D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _contour2Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new Contour3D and associate it with the named DataSet
 */
bool Renderer::addContour3D(const DataSetId& id, int numContours)
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

        if (getGraphicsObject<Contour3D>(dsID)) {
            WARN("Replacing existing Contour3D %s", dsID.c_str());
            deleteGraphicsObject<Contour3D>(dsID);
        }

        Contour3D *contour = new Contour3D(numContours);

        contour->setDataSet(ds, this);

        if (contour->getProp() == NULL) {
            delete contour;
            return false;
        } else {
            _renderer->AddViewProp(contour->getProp());
        }

        _contour3Ds[dsID] = contour;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new Contour3D and associate it with the named DataSet
 */
bool Renderer::addContour3D(const DataSetId& id,const std::vector<double>& contours)
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

        if (getGraphicsObject<Contour3D>(dsID)) {
            WARN("Replacing existing Contour3D %s", dsID.c_str());
            deleteGraphicsObject<Contour3D>(dsID);
        }

        Contour3D *contour = new Contour3D(contours);

        contour->setDataSet(ds, this);

        if (contour->getProp() == NULL) {
            delete contour;
            return false;
        } else {
            _renderer->AddViewProp(contour->getProp());
        }

        _contour3Ds[dsID] = contour;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the number of equally spaced isosurfaces for the given DataSet
 */
void Renderer::setContour3DContourField(const DataSetId& id, const char *fieldName)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourField(fieldName);
     } while (doAll && ++itr != _contour3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the number of equally spaced isosurfaces for the given DataSet
 */
void Renderer::setContour3DNumContours(const DataSetId& id, int numContours)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setNumContours(numContours);
     } while (doAll && ++itr != _contour3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set a list of isovalues for the given DataSet
 */
void Renderer::setContour3DContourList(const DataSetId& id, const std::vector<double>& contours)
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourList(contours);
    } while (doAll && ++itr != _contour3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setContour3DColorMode(const DataSetId& id,
                                     Contour3D::ColorMode mode,
                                     DataSet::DataAttributeType type,
                                     const char *name, double range[2])
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        if (itr == _contour3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setContour3DColorMode(const DataSetId& id,
                                     Contour3D::ColorMode mode,
                                     const char *name, double range[2])
{
    Contour3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _contour3Ds.begin();
        if (itr == _contour3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _contour3Ds.find(id);
    }
    if (itr == _contour3Ds.end()) {
        ERROR("Contour3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _contour3Ds.end());

    _needsRedraw = true;
}

/**
 * \brief Set the visibility of cutplane outlines
 */
void Renderer::setCutplaneOutlineVisibility(const DataSetId& id, bool state)
{
    CutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _cutplanes.begin();
        if (itr == _cutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _cutplanes.find(id);
    }

    if (itr == _cutplanes.end()) {
        ERROR("Cutplane not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOutlineVisibility(state);
     } while (doAll && ++itr != _cutplanes.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the visibility of slices in one of the three axes
 */
void Renderer::setCutplaneSliceVisibility(const DataSetId& id, Axis axis, bool state)
{
    CutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _cutplanes.begin();
        if (itr == _cutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _cutplanes.find(id);
    }

    if (itr == _cutplanes.end()) {
        ERROR("Cutplane not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSliceVisibility(axis, state);
     } while (doAll && ++itr != _cutplanes.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the point cloud render style for the specified DataSet
 */
void Renderer::setCutplaneCloudStyle(const DataSetId& id,
                                     Cutplane::CloudStyle style)
{
    CutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _cutplanes.begin();
        if (itr == _cutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _cutplanes.find(id);
    }
    if (itr == _cutplanes.end()) {
        ERROR("Cutplane not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setCloudStyle(style);
    } while (doAll && ++itr != _cutplanes.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setCutplaneColorMode(const DataSetId& id,
                                    Cutplane::ColorMode mode,
                                    DataSet::DataAttributeType type,
                                    const char *name, double range[2])
{
    CutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _cutplanes.begin();
        if (itr == _cutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _cutplanes.find(id);
    }
    if (itr == _cutplanes.end()) {
        ERROR("Cutplane not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _cutplanes.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setCutplaneColorMode(const DataSetId& id,
                                    Cutplane::ColorMode mode,
                                    const char *name, double range[2])
{
    CutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _cutplanes.begin();
        if (itr == _cutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _cutplanes.find(id);
    }
    if (itr == _cutplanes.end()) {
        ERROR("Cutplane not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _cutplanes.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new Cylinder and associate it with an ID
 */
bool Renderer::addCylinder(const DataSetId& id, double radius, double height,
                           bool cap, bool flipNormals)
{
    Cylinder *gobj;
    if ((gobj = getGraphicsObject<Cylinder>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Cylinder>(id);
    }

    gobj = new Cylinder();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setRadius(radius);
    gobj->setHeight(height);
    gobj->setCapping(cap);
    if (flipNormals)
        gobj->flipNormals(flipNormals);

    getGraphicsObjectHashmap<Cylinder>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set Cylinder capping
 */
void Renderer::setCylinderCapping(const DataSetId& id, bool state)
{
    CylinderHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _cylinders.begin();
        if (itr == _cylinders.end())
            return;
        doAll = true;
    } else {
        itr = _cylinders.find(id);
    }
    if (itr == _cylinders.end()) {
        ERROR("Cylinder not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setCapping(state);
    } while (doAll && ++itr != _cylinders.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set Cylinder resolution
 */
void Renderer::setCylinderResolution(const DataSetId& id, int res)
{
    CylinderHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _cylinders.begin();
        if (itr == _cylinders.end())
            return;
        doAll = true;
    } else {
        itr = _cylinders.find(id);
    }
    if (itr == _cylinders.end()) {
        ERROR("Cylinder not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setResolution(res);
    } while (doAll && ++itr != _cylinders.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Create a new Disk and associate it with an ID
 */
bool Renderer::addDisk(const DataSetId& id,
                       double innerRadius,
                       double outerRadius,
                       bool flipNormals)
{
    Disk *gobj;
    if ((gobj = getGraphicsObject<Disk>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Disk>(id);
    }

    gobj = new Disk();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setRadii(innerRadius, outerRadius);
    if (flipNormals)
        gobj->flipNormals(flipNormals);

    getGraphicsObjectHashmap<Disk>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set Disk resolution
 */
void Renderer::setDiskResolution(const DataSetId& id, int resRadial, int resCircum)
{
    DiskHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _disks.begin();
        if (itr == _disks.end())
            return;
        doAll = true;
    } else {
        itr = _disks.find(id);
    }
    if (itr == _disks.end()) {
        ERROR("Disk not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setResolution(resRadial, resCircum);
    } while (doAll && ++itr != _disks.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Create a new Glyphs and associate it with the named DataSet
 */
bool Renderer::addGlyphs(const DataSetId& id, Glyphs::GlyphShape shape)
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

        if (getGraphicsObject<Glyphs>(dsID)) {
            WARN("Replacing existing Glyphs %s", dsID.c_str());
            deleteGraphicsObject<Glyphs>(dsID);
        }

        Glyphs *glyphs = new Glyphs(shape);

        glyphs->setDataSet(ds, this);

        if (glyphs->getProp() == NULL) {
            delete glyphs;
            return false;
        } else {
            _renderer->AddViewProp(glyphs->getProp());
        }

        _glyphs[dsID] = glyphs;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setGlyphsColorMode(const DataSetId& id,
                                  Glyphs::ColorMode mode,
                                  const char *name, double range[2])
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        if (itr == _glyphs.end())
            return;
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _glyphs.end());

    _needsRedraw = true;
}

/**
 * \brief Controls the array used to scale glyphs for the given DataSet
 */
void Renderer::setGlyphsScalingMode(const DataSetId& id,
                                    Glyphs::ScalingMode mode,
                                    const char *name, double range[2])
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        if (itr == _glyphs.end())
            return;
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScalingMode(mode, name, range);
    } while (doAll && ++itr != _glyphs.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Limit the number of glyphs displayed
 *
 * The choice of glyphs to display can be based on sampling every
 * n-th point (ratio) or by random sample
 *
 * \param id DataSet ID
 * \param max Maximum number of glyphs to display, negative means display all
 * \param random Flag to enable/disable random sampling
 * \param offset If random is false, this controls the first sample point
 * \param ratio If random is false, this ratio controls every n-th point sampling
 */
void Renderer::setGlyphsMaximumNumberOfGlyphs(const DataSetId& id, int max,
                                              bool random, int offset, int ratio)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        if (itr == _glyphs.end())
            return;
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setMaximumNumberOfGlyphs(max, random, offset, ratio);
    } while (doAll && ++itr != _glyphs.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Controls if field data range is normalized to [0,1] before
 * applying scale factor for the given DataSet
 */
void Renderer::setGlyphsNormalizeScale(const DataSetId& id, bool normalize)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        if (itr == _glyphs.end())
            return;
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setNormalizeScale(normalize);
    } while (doAll && ++itr != _glyphs.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Controls if glyphs are oriented from a vector field for the 
 * given DataSet
 */
void Renderer::setGlyphsOrientMode(const DataSetId& id, bool state,
                                   const char *name)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        if (itr == _glyphs.end())
            return;
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOrientMode(state, name);
    } while (doAll && ++itr != _glyphs.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set glyph shape resolution
 */
void Renderer::setGlyphsQuality(const DataSetId& id, double quality)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        if (itr == _glyphs.end())
            return;
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setQuality(quality);
    } while (doAll && ++itr != _glyphs.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the shape of Glyphs for the given DataSet
 */
void Renderer::setGlyphsShape(const DataSetId& id, Glyphs::GlyphShape shape)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        if (itr == _glyphs.end())
            return;
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setGlyphShape(shape);
    } while (doAll && ++itr != _glyphs.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the glyph scaling factor for the given DataSet
 */
void Renderer::setGlyphsScaleFactor(const DataSetId& id, double scale)
{
    GlyphsHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _glyphs.begin();
        if (itr == _glyphs.end())
            return;
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setScaleFactor(scale);
    } while (doAll && ++itr != _glyphs.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

bool Renderer::addGroup(const DataSetId& id, const std::vector<Group::NodeId>& nodeList)
{
    if (id.compare("all") == 0) {
        addChildrenToGroup(id, nodeList);
        return true;
    }

    Group *gobj;
    if ((gobj = getGraphicsObject<Group>(id)) != NULL) {
        // Group exists, so add nodes to it
        addChildrenToGroup(id, nodeList);
        return true;
    }

    gobj = new Group();
 
    gobj->setDataSet(NULL, this);

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

    for (std::vector<Group::NodeId>::const_iterator itr = nodeList.begin();
         itr != nodeList.end(); ++itr) {
        GraphicsObject *node = getGenericGraphicsObject(*itr);
        if (node != NULL) {
            if (node->getProp())
                _renderer->RemoveViewProp(node->getProp());
            if (node->getOverlayProp())
                _renderer->RemoveViewProp(node->getOverlayProp());
            gobj->addChild(*itr, node);
        } else {
            ERROR("Can't find node: %s", itr->c_str());
        }
    }

    getGraphicsObjectHashmap<Group>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true; 
}

void Renderer::addChildrenToGroup(const DataSetId& id, const std::vector<Group::NodeId>& nodeList)
{
    GroupHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _groups.begin();
        if (itr == _groups.end())
            return;
        doAll = true;
    } else {
        itr = _groups.find(id);
    }

    if (itr == _groups.end()) {
        ERROR("Group not found: %s", id.c_str());
        return;
    }

    do {
        for (std::vector<Group::NodeId>::const_iterator citr = nodeList.begin();
             citr != nodeList.end(); ++citr) {
            GraphicsObject *node = getGenericGraphicsObject(*citr);
            if (node != NULL) {
                if (node->getProp())
                    _renderer->RemoveViewProp(node->getProp());
                if (node->getOverlayProp())
                    _renderer->RemoveViewProp(node->getOverlayProp());
                itr->second->addChild(*citr, node);
            } else {
                ERROR("Can't find node: %s", citr->c_str());
            }
        }
     } while (doAll && ++itr != _groups.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

void Renderer::removeChildrenFromGroup(const DataSetId& id, const std::vector<Group::NodeId>& nodeList)
{
    GroupHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _groups.begin();
        if (itr == _groups.end())
            return;
        doAll = true;
    } else {
        itr = _groups.find(id);
    }

    if (itr == _groups.end()) {
        ERROR("Group not found: %s", id.c_str());
        return;
    }

    do {
        for (std::vector<Group::NodeId>::const_iterator citr = nodeList.begin();
             citr != nodeList.end(); ++citr) {
            GraphicsObject *node = getGenericGraphicsObject(*citr);
            if (node != NULL) {
                if (node->getProp())
                    _renderer->AddViewProp(node->getProp());
                if (node->getOverlayProp())
                    _renderer->AddViewProp(node->getOverlayProp());
                assert(node == itr->second->removeChild(*citr));
            } else {
                ERROR("Can't find node: %s", citr->c_str());
            }
        }
     } while (doAll && ++itr != _groups.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Create a new HeightMap and associate it with the named DataSet
 */
bool Renderer::addHeightMap(const DataSetId& id, int numContours, double heightScale)
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

        if (getGraphicsObject<HeightMap>(dsID)) {
            WARN("Replacing existing HeightMap %s", dsID.c_str());
            deleteGraphicsObject<HeightMap>(dsID);
        }

        HeightMap *hmap = new HeightMap(numContours, heightScale);

        hmap->setDataSet(ds, this);

        if (hmap->getProp() == NULL) {
            delete hmap;
            return false;
        } else {
            _renderer->AddViewProp(hmap->getProp());
        }

        _heightMaps[dsID] = hmap;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new HeightMap and associate it with the named DataSet
 */
bool Renderer::addHeightMap(const DataSetId& id, const std::vector<double>& contours, double heightScale)
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

        if (getGraphicsObject<HeightMap>(dsID)) {
            WARN("Replacing existing HeightMap %s", dsID.c_str());
            deleteGraphicsObject<HeightMap>(dsID);
        }

        HeightMap *hmap = new HeightMap(contours, heightScale);

        hmap->setDataSet(ds, this);

        if (hmap->getProp() == NULL) {
            delete hmap;
            return false;
        } else {
            _renderer->AddViewProp(hmap->getProp());
        }

        _heightMaps[dsID] = hmap;
    } while (doAll && ++itr != _dataSets.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set amount to scale scalar values when creating elevations
 * in the height map
 */
void Renderer::setHeightMapHeightScale(const DataSetId& id, double scale)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }

    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setHeightScale(scale);
     } while (doAll && ++itr != _heightMaps.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the point cloud render style for the specified DataSet
 */
void Renderer::setHeightMapCloudStyle(const DataSetId& id,
                                      HeightMap::CloudStyle style)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setCloudStyle(style);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the number of equally spaced contour isolines for the given DataSet
 */
void Renderer::setHeightMapNumContours(const DataSetId& id, int numContours)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setNumContours(numContours);
    } while (doAll && ++itr != _heightMaps.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set a list of height map contour isovalues for the given DataSet
 */
void Renderer::setHeightMapContourList(const DataSetId& id, const std::vector<double>& contours)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourList(contours);
    } while (doAll && ++itr != _heightMaps.end());

    sceneBoundsChanged();
     _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering height map contour lines for the given DataSet
 */
void Renderer::setHeightMapContourLineVisibility(const DataSetId& id, bool state)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourLineVisibility(state);
    } while (doAll && ++itr != _heightMaps.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering height map colormap surface for the given DataSet
 */
void Renderer::setHeightMapContourSurfaceVisibility(const DataSetId& id, bool state)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourSurfaceVisibility(state);
    } while (doAll && ++itr != _heightMaps.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the RGB height map isoline color for the specified DataSet
 */
void Renderer::setHeightMapContourEdgeColor(const DataSetId& id, float color[3])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourEdgeColor(color);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the height map isoline width for the specified DataSet (may be a no-op)
 *
 * If the OpenGL implementation/hardware does not support wide lines, 
 * this function may not have an effect.
 */
void Renderer::setHeightMapContourEdgeWidth(const DataSetId& id, float edgeWidth)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourEdgeWidth(edgeWidth);
    } while (doAll && ++itr != _heightMaps.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Toggle colormapping of contour lines
 */
void Renderer::setHeightMapContourLineColorMapEnabled(const DataSetId& id, bool mode)
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setContourLineColorMapEnabled(mode);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;    
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setHeightMapColorMode(const DataSetId& id,
                                     HeightMap::ColorMode mode,
                                     DataSet::DataAttributeType type,
                                     const char *name, double range[2])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setHeightMapColorMode(const DataSetId& id,
                                     HeightMap::ColorMode mode,
                                     const char *name, double range[2])
{
    HeightMapHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _heightMaps.begin();
        if (itr == _heightMaps.end())
            return;
        doAll = true;
    } else {
        itr = _heightMaps.find(id);
    }
    if (itr == _heightMaps.end()) {
        ERROR("HeightMap not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _heightMaps.end());

    _needsRedraw = true;
}

void Renderer::setImageBackground(const DataSetId& id, bool state)
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBackground(state);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

void Renderer::setImageBacking(const DataSetId& id, bool state)
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBacking(state);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

void Renderer::setImageBorder(const DataSetId& id, bool state)
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBorder(state);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

void Renderer::setImageExtents(const DataSetId& id, int extents[6])
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setExtents(extents);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

void Renderer::setImageLevel(const DataSetId& id, double level)
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLevel(level);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

void Renderer::setImageWindow(const DataSetId& id, double window)
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWindow(window);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

void Renderer::setImageSlicePlane(const DataSetId& id, double normal[3], double origin[3])
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSlicePlane(normal, origin);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

void Renderer::setImageSliceFollowsCamera(const DataSetId& id, bool state)
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSliceFollowsCamera(state);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

void Renderer::setImageZSlice(const DataSetId& id, int z)
{
    ImageHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _images.begin();
        if (itr == _images.end())
            return;
        doAll = true;
    } else {
        itr = _images.find(id);
    }
    if (itr == _images.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setZSlice(z);
    } while (doAll && ++itr != _images.end());

    _needsRedraw = true;
}

/**
 * \brief Set the visibility of cutplane outlines
 */
void Renderer::setImageCutplaneOutlineVisibility(const DataSetId& id, bool state)
{
    ImageCutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _imageCutplanes.begin();
        if (itr == _imageCutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _imageCutplanes.find(id);
    }

    if (itr == _imageCutplanes.end()) {
        ERROR("ImageCutplane not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setOutlineVisibility(state);
     } while (doAll && ++itr != _imageCutplanes.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the visibility of slices in one of the three axes
 */
void Renderer::setImageCutplaneSliceVisibility(const DataSetId& id, Axis axis, bool state)
{
    ImageCutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _imageCutplanes.begin();
        if (itr == _imageCutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _imageCutplanes.find(id);
    }

    if (itr == _imageCutplanes.end()) {
        ERROR("ImageCutplane not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSliceVisibility(axis, state);
     } while (doAll && ++itr != _imageCutplanes.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

void Renderer::setImageCutplaneLevel(const DataSetId& id, double level)
{
    ImageCutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _imageCutplanes.begin();
        if (itr == _imageCutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _imageCutplanes.find(id);
    }
    if (itr == _imageCutplanes.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLevel(level);
    } while (doAll && ++itr != _imageCutplanes.end());

    _needsRedraw = true;
}

void Renderer::setImageCutplaneWindow(const DataSetId& id, double window)
{
    ImageCutplaneHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _imageCutplanes.begin();
        if (itr == _imageCutplanes.end())
            return;
        doAll = true;
    } else {
        itr = _imageCutplanes.find(id);
    }
    if (itr == _imageCutplanes.end()) {
        ERROR("Image not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWindow(window);
    } while (doAll && ++itr != _imageCutplanes.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new Line and associate it with an ID
 */
bool Renderer::addLine(const DataSetId& id, double pt1[3], double pt2[3])
{
    Line *gobj;
    if ((gobj = getGraphicsObject<Line>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Line>(id);
    }

    gobj = new Line();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setEndPoints(pt1, pt2);

    getGraphicsObjectHashmap<Line>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new Line and associate it with an ID
 */
bool Renderer::addLine(const DataSetId& id, std::vector<double> points)
{
    Line *gobj;
    if ((gobj = getGraphicsObject<Line>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Line>(id);
    }

    gobj = new Line();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setPoints(points);

    getGraphicsObjectHashmap<Line>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set atom sphere resolution
 */
void Renderer::setMoleculeAtomQuality(const DataSetId& id, double quality)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAtomQuality(quality);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set bond cylinder resolution
 */
void Renderer::setMoleculeBondQuality(const DataSetId& id, double quality)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBondQuality(quality);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set radius scale factor for atoms
 */
void Renderer::setMoleculeAtomRadiusScale(const DataSetId& id, double scale)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAtomRadiusScale(scale);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set radius standard for scaling atoms
 */
void Renderer::setMoleculeAtomScaling(const DataSetId& id, Molecule::AtomScaling scaling)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAtomScaling(scaling);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Molecule atoms for the given DataSet
 */
void Renderer::setMoleculeAtomVisibility(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAtomVisibility(state);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the field used to label atoms for the given DataSet
 */
void Renderer::setMoleculeAtomLabelField(const DataSetId& id, const char *fieldName)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAtomLabelField(fieldName);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Molecule atom labels for the given DataSet
 */
void Renderer::setMoleculeAtomLabelVisibility(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setAtomLabelVisibility(state);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set radius scale factor for atoms
 */
void Renderer::setMoleculeBondRadiusScale(const DataSetId& id, double scale)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBondRadiusScale(scale);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Molecule bonds for the given DataSet
 */
void Renderer::setMoleculeBondVisibility(const DataSetId& id, bool state)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBondVisibility(state);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set render style of the Molecule bonds for the given DataSet
 */
void Renderer::setMoleculeBondStyle(const DataSetId& id, Molecule::BondStyle style)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBondStyle(style);
    } while (doAll && ++itr != _molecules.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set coloring mode of the Molecule bonds for the given DataSet
 */
void Renderer::setMoleculeBondColorMode(const DataSetId& id, Molecule::BondColorMode mode)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBondColorMode(mode);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set constant color of the Molecule bonds for the given DataSet
 */
void Renderer::setMoleculeBondColor(const DataSetId& id, float color[3])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBondColor(color);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setMoleculeColorMode(const DataSetId& id,
                                    Molecule::ColorMode mode,
                                    DataSet::DataAttributeType type,
                                    const char *name, double range[2])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setMoleculeColorMode(const DataSetId& id,
                                    Molecule::ColorMode mode,
                                    const char *name, double range[2])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
        if (itr == _molecules.end())
            return;
        doAll = true;
    } else {
        itr = _molecules.find(id);
    }
    if (itr == _molecules.end()) {
        ERROR("Molecule not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _molecules.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new Parallelepiped and associate it with an ID
 */
bool Renderer::addParallelepiped(const DataSetId& id,
                                 double vec1[3], double vec2[3], double vec3[3],
                                 bool flipNormals)
{
    Parallelepiped *gobj;
    if ((gobj = getGraphicsObject<Parallelepiped>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Parallelepiped>(id);
    }

    gobj = new Parallelepiped();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setVectors(vec1, vec2, vec3);
    if (flipNormals)
        gobj->flipNormals(flipNormals);

    getGraphicsObjectHashmap<Parallelepiped>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new n-sided regular Polygon and associate it with an ID
 */
bool Renderer::addPolygon(const DataSetId& id, int numSides,
                          double center[3], double normal[3], double radius)
{
    Polygon *gobj;
    if ((gobj = getGraphicsObject<Polygon>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Polygon>(id);
    }

    gobj = new Polygon();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setNumberOfSides(numSides);
    gobj->setCenter(center);
    gobj->setNormal(normal);
    gobj->setRadius(radius);

    getGraphicsObjectHashmap<Polygon>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the  point cloud render style for the specified DataSet
 */
void Renderer::setPolyDataCloudStyle(const DataSetId& id,
                                     PolyData::CloudStyle style)
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        if (itr == _polyDatas.end())
            return;
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setCloudStyle(style);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setPolyDataColorMode(const DataSetId& id,
                                    PolyData::ColorMode mode,
                                    DataSet::DataAttributeType type,
                                    const char *name, double range[2])
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        if (itr == _polyDatas.end())
            return;
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setPolyDataColorMode(const DataSetId& id,
                                    PolyData::ColorMode mode,
                                    const char *name, double range[2])
{
    PolyDataHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _polyDatas.begin();
        if (itr == _polyDatas.end())
            return;
        doAll = true;
    } else {
        itr = _polyDatas.find(id);
    }
    if (itr == _polyDatas.end()) {
        ERROR("PolyData not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _polyDatas.end());

    _needsRedraw = true;
}

/**
 * \brief Set the  point cloud render style for the specified DataSet
 */
void Renderer::setPseudoColorCloudStyle(const DataSetId& id,
                                        PseudoColor::CloudStyle style)
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        if (itr == _pseudoColors.end())
            return;
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setCloudStyle(style);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setPseudoColorColorMode(const DataSetId& id,
                                       PseudoColor::ColorMode mode,
                                       DataSet::DataAttributeType type,
                                       const char *name, double range[2])
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        if (itr == _pseudoColors.end())
            return;
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setPseudoColorColorMode(const DataSetId& id,
                                       PseudoColor::ColorMode mode,
                                       const char *name, double range[2])
{
    PseudoColorHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _pseudoColors.begin();
        if (itr == _pseudoColors.end())
            return;
        doAll = true;
    } else {
        itr = _pseudoColors.find(id);
    }
    if (itr == _pseudoColors.end()) {
        ERROR("PseudoColor not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _pseudoColors.end());

    _needsRedraw = true;
}

/**
 * \brief Create a new Sphere and associate it with an ID
 */
bool Renderer::addSphere(const DataSetId& id, double center[3], double radius, bool flipNormals)
{
    Sphere *gobj;
    if ((gobj = getGraphicsObject<Sphere>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Sphere>(id);
    }

    gobj = new Sphere();
 
    gobj->setDataSet(NULL, this);

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

    gobj->setCenter(center);
    gobj->setRadius(radius);
    if (flipNormals)
        gobj->flipNormals(flipNormals);

    getGraphicsObjectHashmap<Sphere>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Create a new Text3D label and associate it with an ID
 */
bool Renderer::addText3D(const DataSetId& id, const char *string,
                         const char *fontFamily, int fontSize,
                         bool bold, bool italic, bool shadow)
{
    Text3D *gobj;
    if ((gobj = getGraphicsObject<Text3D>(id)) != NULL) {
        WARN("Replacing existing %s %s", gobj->getClassName(), id.c_str());
        deleteGraphicsObject<Text3D>(id);
    }

    gobj = new Text3D();

    gobj->setDataSet(NULL, this);

    gobj->setText(string);
    gobj->setFont(fontFamily);
    gobj->setFontSize(fontSize);
    gobj->setBold(bold);
    gobj->setItalic(italic);
    gobj->setShadow(shadow);

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

    getGraphicsObjectHashmap<Text3D>()[id] = gobj;

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set Sphere resolution
 */
void Renderer::setSphereResolution(const DataSetId& id, int thetaRes, int phiRes)
{
    SphereHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _spheres.begin();
        if (itr == _spheres.end())
            return;
        doAll = true;
    } else {
        itr = _spheres.find(id);
    }
    if (itr == _spheres.end()) {
        ERROR("Sphere not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setThetaResolution(thetaRes);
        itr->second->setPhiResolution(phiRes);
    } while (doAll && ++itr != _spheres.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set Sphere section
 */
void Renderer::setSphereSection(const DataSetId& id, double thetaStart, double thetaEnd,
                                double phiStart, double phiEnd)
{
    SphereHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _spheres.begin();
        if (itr == _spheres.end())
            return;
        doAll = true;
    } else {
        itr = _spheres.find(id);
    }
    if (itr == _spheres.end()) {
        ERROR("Sphere not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setStartTheta(thetaStart);
        itr->second->setEndTheta(thetaEnd);
        itr->second->setStartPhi(phiStart);
        itr->second->setEndPhi(phiEnd);
    } while (doAll && ++itr != _spheres.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points of the streamlines DataSet
 */
void Renderer::setStreamlinesNumberOfSeedPoints(const DataSetId& id, int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setNumberOfSeedPoints(numPoints);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed point size (may be a no-op)
 */
void Renderer::setStreamlinesSeedPointSize(const DataSetId& id, float size)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedPointSize(size);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points of the streamlines DataSet
 */
void Renderer::setStreamlinesSeedToMeshPoints(const DataSetId& id,
                                              int maxPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToMeshPoints(maxPoints);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points distributed randomly inside
 * cells of the streamlines DataSet
 */
void Renderer::setStreamlinesSeedToFilledMesh(const DataSetId& id, int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToFilledMesh(numPoints);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points of a DataSet
 *
 * \param[in] id DataSet identifier
 * \param[in] data Bytes of VTK DataSet file
 * \param[in] nbytes Length of data array
 * \param[in] maxPoints Maximum number of points to be used as seeds
 *
 * \return boolean indicating success or failure
 */
bool Renderer::setStreamlinesSeedToMeshPoints(const DataSetId& id,
                                              char *data, size_t nbytes,
                                              int maxPoints)
{
    vtkSmartPointer<vtkDataSetReader> reader = vtkSmartPointer<vtkDataSetReader>::New();
    vtkSmartPointer<vtkCharArray> dataSetString = vtkSmartPointer<vtkCharArray>::New();

    dataSetString->SetArray(data, nbytes, 1);
    reader->SetInputArray(dataSetString);
    reader->ReadFromInputStringOn();
    reader->Update();

    vtkSmartPointer<vtkDataSet> dataSet = reader->GetOutput();
    if (dataSet == NULL) {
        return false;
    }
#ifndef USE_VTK6
    dataSet->SetPipelineInformation(NULL);
#endif
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return false;
    }

    do {
        itr->second->setSeedToMeshPoints(dataSet, maxPoints);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the streamlines seed to points distributed randomly inside
 * cells of DataSet
 *
 * \param[in] id DataSet identifier
 * \param[in] data Bytes of VTK DataSet file
 * \param[in] nbytes Length of data array
 * \param[in] numPoints Number of random seed points to generate
 *
 * \return boolean indicating success or failure
 */
bool Renderer::setStreamlinesSeedToFilledMesh(const DataSetId& id,
                                              char *data, size_t nbytes,
                                              int numPoints)
{
    vtkSmartPointer<vtkDataSetReader> reader = vtkSmartPointer<vtkDataSetReader>::New();
    vtkSmartPointer<vtkCharArray> dataSetString = vtkSmartPointer<vtkCharArray>::New();

    dataSetString->SetArray(data, nbytes, 1);
    reader->SetInputArray(dataSetString);
    reader->ReadFromInputStringOn();
    reader->Update();

    vtkSmartPointer<vtkDataSet> dataSet = reader->GetOutput();
    if (dataSet == NULL) {
        return false;
    }
#ifndef USE_VTK6
    dataSet->SetPipelineInformation(NULL);
#endif
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return false;
    }

    do {
        itr->second->setSeedToFilledMesh(dataSet, numPoints);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the streamlines seed to points along a line
 */
void Renderer::setStreamlinesSeedToRake(const DataSetId& id,
                                        double start[3], double end[3],
                                        int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToRake(start, end, numPoints);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points inside a disk, with optional
 * hole
 */
void Renderer::setStreamlinesSeedToDisk(const DataSetId& id,
                                        double center[3], double normal[3],
                                        double radius, double innerRadius,
                                        int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToDisk(center, normal, radius, innerRadius, numPoints);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to vertices of an n-sided polygon
 */
void Renderer::setStreamlinesSeedToPolygon(const DataSetId& id,
                                           double center[3], double normal[3],
                                           double angle, double radius,
                                           int numSides)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToPolygon(center, normal, angle, radius, numSides);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to vertices of an n-sided polygon
 */
void Renderer::setStreamlinesSeedToFilledPolygon(const DataSetId& id,
                                                 double center[3],
                                                 double normal[3],
                                                 double angle, double radius,
                                                 int numSides, int numPoints)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedToFilledPolygon(center, normal, angle,
                                            radius, numSides, numPoints);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

void Renderer::setStreamlinesTerminalSpeed(const DataSetId& id, double speed)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setTerminalSpeed(speed);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set Streamlines rendering to polylines for the specified DataSet
 */
void Renderer::setStreamlinesTypeToLines(const DataSetId& id)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLineTypeToLines();
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set Streamlines rendering to tubes for the specified DataSet
 */
void Renderer::setStreamlinesTypeToTubes(const DataSetId& id, int numSides, double radius)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLineTypeToTubes(numSides, radius);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set Streamlines rendering to ribbons for the specified DataSet
 */
void Renderer::setStreamlinesTypeToRibbons(const DataSetId& id, double width, double angle)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setLineTypeToRibbons(width, angle);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set Streamlines maximum length for the specified DataSet
 */
void Renderer::setStreamlinesLength(const DataSetId& id, double length)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setMaxPropagation(length);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Turn on/off rendering of the Streamlines seed geometry for the given DataSet
 */
void Renderer::setStreamlinesSeedVisibility(const DataSetId& id, bool state)
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedVisibility(state);
    } while (doAll && ++itr != _streamlines.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setStreamlinesColorMode(const DataSetId& id,
                                       Streamlines::ColorMode mode,
                                       DataSet::DataAttributeType type,
                                       const char *name, double range[2])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setStreamlinesColorMode(const DataSetId& id,
                                       Streamlines::ColorMode mode,
                                       const char *name, double range[2])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

/**
 * \brief Set the RGB line/edge color for the specified DataSet
 */
void Renderer::setStreamlinesSeedColor(const DataSetId& id, float color[3])
{
    StreamlinesHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _streamlines.begin();
        if (itr == _streamlines.end())
            return;
        doAll = true;
    } else {
        itr = _streamlines.find(id);
    }
    if (itr == _streamlines.end()) {
        ERROR("Streamlines not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setSeedColor(color);
    } while (doAll && ++itr != _streamlines.end());

    _needsRedraw = true;
}

void Renderer::setText3DBold(const DataSetId& id, bool state)
{
    Text3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _text3Ds.begin();
        if (itr == _text3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _text3Ds.find(id);
    }
    if (itr == _text3Ds.end()) {
        ERROR("Text3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBold(state);
    } while (doAll && ++itr != _text3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the font family for the Text3D
 */
void Renderer::setText3DFont(const DataSetId& id, const char *fontFamily)
{
    Text3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _text3Ds.begin();
        if (itr == _text3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _text3Ds.find(id);
    }
    if (itr == _text3Ds.end()) {
        ERROR("Text3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setFont(fontFamily);
    } while (doAll && ++itr != _text3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

/**
 * \brief Set the font family for the Text3D
 */
void Renderer::setText3DFontSize(const DataSetId& id, int size)
{
    Text3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _text3Ds.begin();
        if (itr == _text3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _text3Ds.find(id);
    }
    if (itr == _text3Ds.end()) {
        ERROR("Text3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setFontSize(size);
    } while (doAll && ++itr != _text3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

void Renderer::setText3DFollowCamera(const DataSetId& id, bool state)
{
    Text3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _text3Ds.begin();
        if (itr == _text3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _text3Ds.find(id);
    }
    if (itr == _text3Ds.end()) {
        ERROR("Text3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setFollowCamera(state, _renderer);
    } while (doAll && ++itr != _text3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

void Renderer::setText3DItalic(const DataSetId& id, bool state)
{
    Text3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _text3Ds.begin();
        if (itr == _text3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _text3Ds.find(id);
    }
    if (itr == _text3Ds.end()) {
        ERROR("Text3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setItalic(state);
    } while (doAll && ++itr != _text3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

void Renderer::setText3DShadow(const DataSetId& id, bool state)
{
    Text3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _text3Ds.begin();
        if (itr == _text3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _text3Ds.find(id);
    }
    if (itr == _text3Ds.end()) {
        ERROR("Text3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setShadow(state);
    } while (doAll && ++itr != _text3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

void Renderer::setText3DText(const DataSetId& id, const char *text)
{
    Text3DHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _text3Ds.begin();
        if (itr == _text3Ds.end())
            return;
        doAll = true;
    } else {
        itr = _text3Ds.find(id);
    }
    if (itr == _text3Ds.end()) {
        ERROR("Text3D not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setText(text);
    } while (doAll && ++itr != _text3Ds.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}

void Renderer::setVolumeBlendMode(const DataSetId& id, Volume::BlendMode mode)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        if (itr == _volumes.end())
            return;
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setBlendMode(mode);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
}

/**
 * \brief Set the sample rate for the volume shader
 *
 * Smaller values will give better rendering at the cost
 * of slower rendering
 */
void Renderer::setVolumeSampleDistance(const DataSetId& id, double distance)
{
    VolumeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _volumes.begin();
        if (itr == _volumes.end())
            return;
        doAll = true;
    } else {
        itr = _volumes.find(id);
    }
    if (itr == _volumes.end()) {
        ERROR("Volume not found: %s", id.c_str());
        return;
    }

    do {
        distance *= itr->second->getAverageSpacing();
        itr->second->setSampleDistance((float)distance);
    } while (doAll && ++itr != _volumes.end());

    _needsRedraw = true;
}

/**
 * \brief Set the point cloud render style for the specified DataSet
 */
void Renderer::setWarpCloudStyle(const DataSetId& id,
                                 Warp::CloudStyle style)
{
    WarpHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _warps.begin();
        if (itr == _warps.end())
            return;
        doAll = true;
    } else {
        itr = _warps.find(id);
    }
    if (itr == _warps.end()) {
        ERROR("Warp not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setCloudStyle(style);
    } while (doAll && ++itr != _warps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setWarpColorMode(const DataSetId& id,
                                Warp::ColorMode mode,
                                DataSet::DataAttributeType type,
                                const char *name, double range[2])
{
    WarpHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _warps.begin();
        if (itr == _warps.end())
            return;
        doAll = true;
    } else {
        itr = _warps.find(id);
    }
    if (itr == _warps.end()) {
        ERROR("Warp not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, type, name, range);
    } while (doAll && ++itr != _warps.end());

    _needsRedraw = true;
}

/**
 * \brief Set the color mode for the specified DataSet
 */
void Renderer::setWarpColorMode(const DataSetId& id,
                                Warp::ColorMode mode,
                                const char *name, double range[2])
{
    WarpHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _warps.begin();
        if (itr == _warps.end())
            return;
        doAll = true;
    } else {
        itr = _warps.find(id);
    }
    if (itr == _warps.end()) {
        ERROR("Warp not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setColorMode(mode, name, range);
    } while (doAll && ++itr != _warps.end());

    _needsRedraw = true;
}

/**
 * \brief Set amount to scale vector magnitudes when warping
 * a mesh
 */
void Renderer::setWarpWarpScale(const DataSetId& id, double scale)
{
    WarpHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _warps.begin();
        if (itr == _warps.end())
            return;
        doAll = true;
    } else {
        itr = _warps.find(id);
    }

    if (itr == _warps.end()) {
        ERROR("Warp not found: %s", id.c_str());
        return;
    }

    do {
        itr->second->setWarpScale(scale);
     } while (doAll && ++itr != _warps.end());

    sceneBoundsChanged();
    _needsRedraw = true;
}
