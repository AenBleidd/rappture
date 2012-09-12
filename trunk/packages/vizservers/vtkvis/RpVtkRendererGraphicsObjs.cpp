/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstring>
#include <typeinfo>

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkCharArray.h>
#include <vtkDataSetReader.h>

#include "RpVtkRendererGraphicsObjs.h"
#include "RpVtkRenderer.h"
#include "RpVtkDataSet.h"
#include "RpBox.h"
#include "RpContour2D.h"
#include "RpContour3D.h"
#include "RpCutplane.h"
#include "RpGlyphs.h"
#include "RpHeightMap.h"
#include "RpLIC.h"
#include "RpLine.h"
#include "RpMolecule.h"
#include "RpPolyData.h"
#include "RpPseudoColor.h"
#include "RpSphere.h"
#include "RpStreamlines.h"
#include "RpVolume.h"
#include "ColorMap.h"
#include "Trace.h"

// Template specializations
namespace Rappture {
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
Renderer::VolumeHashmap &
Renderer::getGraphicsObjectHashmap<Volume>()
{ return _volumes; }

template<>
Renderer::WarpHashmap &
Renderer::getGraphicsObjectHashmap<Warp>()
{ return _warps; }

template Arc *Renderer::getGraphicsObject(const DataSetId&);
template Arrow *Renderer::getGraphicsObject(const DataSetId&);
template Box *Renderer::getGraphicsObject(const DataSetId&);
template Cone *Renderer::getGraphicsObject(const DataSetId&);
template Cylinder *Renderer::getGraphicsObject(const DataSetId&);
template Disk *Renderer::getGraphicsObject(const DataSetId&);
template Group *Renderer::getGraphicsObject(const DataSetId&);
template Line *Renderer::getGraphicsObject(const DataSetId&);
template Polygon *Renderer::getGraphicsObject(const DataSetId&);
template Sphere *Renderer::getGraphicsObject(const DataSetId&);

template <>
bool Renderer::addGraphicsObject<Box>(const DataSetId& id)
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

    getGraphicsObjectHashmap<Box>()[id] = gobj;

    initCamera();
    _needsRedraw = true;
    return true;
}

template <>
bool Renderer::addGraphicsObject<Sphere>(const DataSetId& id)
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

    getGraphicsObjectHashmap<Sphere>()[id] = gobj;

    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the volume slice used for mapping volumetric data
 */
template <>
void Renderer::setGraphicsObjectVolumeSlice<HeightMap>(const DataSetId& id, Axis axis, double ratio)
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

    bool initCam = false;
    do {
        itr->second->selectVolumeSlice(axis, ratio);
        if (itr->second->getHeightScale() > 0.0)
            initCam = true;
     } while (doAll && ++itr != _heightMaps.end());

    if (initCam)
        initCamera();
    else
        _renderer->ResetCameraClippingRange();
    _needsRedraw = true;
}

}
}

using namespace Rappture::VtkVis;

/**
 * \brief Create a new Arc and associate it with an ID
 */
bool Renderer::addArc(const DataSetId& id, double pt1[3], double pt2[3])
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

    gobj->setEndPoints(pt1, pt2);

    getGraphicsObjectHashmap<Arc>()[id] = gobj;

    initCamera();
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

    _needsRedraw = true;
}

/**
 * \brief Create a new Arrow and associate it with an ID
 */
bool Renderer::addArrow(const DataSetId& id, double tipRadius, double shaftRadius, double tipLength)
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

    getGraphicsObjectHashmap<Arrow>()[id] = gobj;

    initCamera();
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

    _needsRedraw = true;
}

/**
 * \brief Create a new Cone and associate it with an ID
 */
bool Renderer::addCone(const DataSetId& id, double radius, double height, bool cap)
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

    getGraphicsObjectHashmap<Cone>()[id] = gobj;

    initCamera();
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

    initCamera();
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

    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the number of equally spaced contour isolines for the given DataSet
 */
void Renderer::setContour2DContours(const DataSetId& id, int numContours)
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
        itr->second->setContours(numContours);
    } while (doAll && ++itr != _contour2Ds.end());

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

    if (_cameraMode == IMAGE)
        setCameraMode(PERSPECTIVE);
    initCamera();
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

    initCamera();
    _needsRedraw = true;
    return true;
}

/**
 * \brief Set the number of equally spaced isosurfaces for the given DataSet
 */
void Renderer::setContour3DContours(const DataSetId& id, int numContours)
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
        itr->second->setContours(numContours);
     } while (doAll && ++itr != _contour3Ds.end());

    initCamera();
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

    initCamera();
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

    _renderer->ResetCameraClippingRange();
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

    _renderer->ResetCameraClippingRange();
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
bool Renderer::addCylinder(const DataSetId& id, double radius, double height, bool cap)
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

    getGraphicsObjectHashmap<Cylinder>()[id] = gobj;

    initCamera();
    _needsRedraw = true;
    return true;
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

    _needsRedraw = true;
}

/**
 * \brief Create a new Disk and associate it with an ID
 */
bool Renderer::addDisk(const DataSetId& id, double innerRadius, double outerRadius)
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

    getGraphicsObjectHashmap<Disk>()[id] = gobj;

    initCamera();
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

    initCamera();

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
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
#ifdef HAVE_GLYPH3D_MAPPER
        itr->second->setColorMode(mode, name, range);
#else
        if (name != NULL && strlen(name) > 0) {
            WARN("Glyphs color mode doesn't support named fields for VTK < 5.8.0");
        }
        itr->second->setColorMode(mode);
#endif
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
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
#ifdef HAVE_GLYPH3D_MAPPER
        itr->second->setScalingMode(mode, name, range);
#else
        if (name != NULL && strlen(name) > 0) {
            WARN("Glyphs scaling mode doesn't support named fields for VTK < 5.8.0");
        }
        itr->second->setScalingMode(mode);
#endif
    } while (doAll && ++itr != _glyphs.end());

    _renderer->ResetCameraClippingRange();
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

    _renderer->ResetCameraClippingRange();
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
        doAll = true;
    } else {
        itr = _glyphs.find(id);
    }
    if (itr == _glyphs.end()) {
        ERROR("Glyphs not found: %s", id.c_str());
        return;
    }

    do {
#ifdef HAVE_GLYPH3D_MAPPER
        itr->second->setOrientMode(state, name);
#else
        if (name != NULL && strlen(name) > 0) {
            WARN("Glyphs orient mode doesn't support named fields for VTK < 5.8.0");
        }
        itr->second->setOrient(state);
#endif
    } while (doAll && ++itr != _glyphs.end());

    _renderer->ResetCameraClippingRange();
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

    _renderer->ResetCameraClippingRange();
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

    initCamera();
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

    initCamera();

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

    initCamera();

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

    _renderer->ResetCameraClippingRange();
    resetAxes();
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

    initCamera();
    _needsRedraw = true;
    return true;
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

    _renderer->ResetCameraClippingRange();
    resetAxes();
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

    _renderer->ResetCameraClippingRange();
    resetAxes();
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

    _renderer->ResetCameraClippingRange();
    resetAxes();
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

    _needsRedraw = true;
}

void Renderer::setMoleculeBondStyle(const DataSetId& id, Molecule::BondStyle style)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
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

    _needsRedraw = true;
}

void Renderer::setMoleculeBondColorMode(const DataSetId& id, Molecule::BondColorMode mode)
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
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

void Renderer::setMoleculeBondColor(const DataSetId& id, float color[3])
{
    MoleculeHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _molecules.begin();
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
 * \brief Create a new n-sided regular Polygon and associate it with an ID
 */
bool Renderer::addPolygon(const DataSetId& id, int numSides)
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

    getGraphicsObjectHashmap<Polygon>()[id] = gobj;

    initCamera();
    _needsRedraw = true;
    return true;
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
 * \brief Set Sphere resolution
 */
void Renderer::setSphereResolution(const DataSetId& id, int thetaRes, int phiRes)
{
    SphereHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _spheres.begin();
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

    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points of the streamlines DataSet
 */
void Renderer::setStreamlinesSeedToMeshPoints(const DataSetId& id)
{
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
        return;
    }

    do {
        itr->second->setSeedToMeshPoints();
    } while (doAll && ++itr != _streamlines.end());

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

    _needsRedraw = true;
}

/**
 * \brief Set the streamlines seed to points of a DataSet
 *
 * \param[in] id DataSet identifier
 * \param[in] data Bytes of VTK DataSet file
 * \param[in] nbytes Length of data array
 *
 * \return boolean indicating success or failure
 */
bool Renderer::setStreamlinesSeedToMeshPoints(const DataSetId& id,
                                              char *data, size_t nbytes)
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
    dataSet->SetPipelineInformation(NULL);

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
        itr->second->setSeedToMeshPoints(dataSet);
    } while (doAll && ++itr != _streamlines.end());

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
    dataSet->SetPipelineInformation(NULL);

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
 * \brief Set amount to scale vector magnitudes when warping
 * a mesh
 */
void Renderer::setWarpWarpScale(const DataSetId& id, double scale)
{
    WarpHashmap::iterator itr;

    bool doAll = false;

    if (id.compare("all") == 0) {
        itr = _warps.begin();
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

    _renderer->ResetCameraClippingRange();
    resetAxes();
    _needsRedraw = true;
}
