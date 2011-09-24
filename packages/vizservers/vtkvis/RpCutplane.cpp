/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataSetMapper.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkTransform.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkCutter.h>
#include <vtkDataSetSurfaceFilter.h>

#include "RpCutplane.h"
#include "Trace.h"

#define MESH_POINT_CLOUDS

using namespace Rappture::VtkVis;

Cutplane::Cutplane() :
    VtkGraphicsObject(),
    _colorMode(COLOR_BY_SCALAR),
    _colorMap(NULL)
{
}

Cutplane::~Cutplane()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Cutplane for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Cutplane with NULL DataSet");
#endif
}

void Cutplane::setDataSet(DataSet *dataSet,
                          bool useCumulative,
                          double scalarRange[2],
                          double vectorMagnitudeRange[2],
                          double vectorComponentRange[3][2])
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        if (useCumulative) {
            _dataRange[0] = scalarRange[0];
            _dataRange[1] = scalarRange[1];
            _vectorMagnitudeRange[0] = vectorMagnitudeRange[0];
            _vectorMagnitudeRange[1] = vectorMagnitudeRange[1];
            for (int i = 0; i < 3; i++) {
                _vectorComponentRange[i][0] = vectorComponentRange[i][0];
                _vectorComponentRange[i][1] = vectorComponentRange[i][1];
            }
        } else {
            _dataSet->getScalarRange(_dataRange);
            _dataSet->getVectorRange(_vectorMagnitudeRange);
            for (int i = 0; i < 3; i++) {
                _dataSet->getVectorRange(_vectorComponentRange[i], i);
            }
        }

        update();
    }
}

/**
 * \brief Create and initialize a VTK Prop to render the object
 */
void Cutplane::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkAssembly>::New();

        for (int i = 0; i < 3; i++) {
            _actor[i] = vtkSmartPointer<vtkActor>::New();
            //_actor[i]->VisibilityOff();
            vtkProperty *property = _actor[i]->GetProperty();
            property->SetColor(_color[0], _color[1], _color[2]);
            property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
            property->SetLineWidth(_edgeWidth);
            property->SetPointSize(_pointSize);
            property->EdgeVisibilityOff();
            property->SetOpacity(_opacity);
            property->SetAmbient(.2);
            if (!_lighting)
                property->LightingOff();
            if (_faceCulling && _opacity == 1.0) {
                setCulling(property, true);
            }
            getAssembly()->AddPart(_actor[i]);
        }
    }
}

void Cutplane::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    double bounds[6];
    _dataSet->getBounds(bounds);
    // Mapper, actor to render color-mapped data set
    for (int i = 0; i < 3; i++) {
        if (_mapper[i] == NULL) {
            _mapper[i] = vtkSmartPointer<vtkDataSetMapper>::New();
            // Map scalars through lookup table regardless of type
            _mapper[i]->SetColorModeToMapScalars();
            //_mapper->InterpolateScalarsBeforeMappingOn();
        }
        _cutPlane[i] = vtkSmartPointer<vtkPlane>::New();
        switch (i) {
        case 0:
            _cutPlane[i]->SetNormal(1, 0, 0);
            _cutPlane[i]->SetOrigin(bounds[0] + (bounds[1]-bounds[0])/2.,
                                    0,
                                    0);
            break;
        case 1:
            _cutPlane[i]->SetNormal(0, 1, 0);
            _cutPlane[i]->SetOrigin(0,
                                    bounds[2] + (bounds[3]-bounds[2])/2.,
                                    0);
            break;
        case 2:
        default:
            _cutPlane[i]->SetNormal(0, 0, 1);
            _cutPlane[i]->SetOrigin(0,
                                    0,
                                    bounds[4] + (bounds[5]-bounds[4])/2.);
            break;
        }
        _cutter[i] = vtkSmartPointer<vtkCutter>::New();
        _cutter[i]->SetCutFunction(_cutPlane[i]);
    }

    initProp();

    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd && 
        pd->GetNumberOfLines() == 0 &&
        pd->GetNumberOfPolys() == 0 &&
        pd->GetNumberOfStrips() == 0) {
        // DataSet is a point cloud
        DataSet::PrincipalPlane plane;
        double offset;
        if (_dataSet->is2D(&plane, &offset)) {
            // DataSet is a 2D point cloud
#ifdef MESH_POINT_CLOUDS
            vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
            if (plane == DataSet::PLANE_ZY) {
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateWXYZ(90, 0, 1, 0);
                if (offset != 0.0) {
                    trans->Translate(-offset, 0, 0);
                }
                mesher->SetTransform(trans);
                _actor[1]->VisibilityOff();
                _actor[2]->VisibilityOff();
            } else if (plane == DataSet::PLANE_XZ) {
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateWXYZ(-90, 1, 0, 0);
                if (offset != 0.0) {
                    trans->Translate(0, -offset, 0);
                }
                mesher->SetTransform(trans);
                _actor[0]->VisibilityOff();
                _actor[2]->VisibilityOff();
            } else if (offset != 0.0) {
                // XY with Z offset
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->Translate(0, 0, -offset);
                mesher->SetTransform(trans);
                _actor[0]->VisibilityOff();
                _actor[1]->VisibilityOff();
            }
            mesher->SetInput(pd);
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetInputConnection(mesher->GetOutputPort());
            }
#else
            vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
            splatter->SetInput(pd);
            int dims[3];
            splatter->GetSampleDimensions(dims);
            TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
            if (plane == DataSet::PLANE_ZY) {
                dims[0] = 3;
            } else if (plane == DataSet::PLANE_XZ) {
                dims[1] = 3;
            } else {
                dims[2] = 3;
            }
            splatter->SetSampleDimensions(dims);
            for (int i = 0; i < 3; i++) {
                _cutter[i]->SetInputConnection(splatter->GetOutputPort());
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(_cutter[i]->GetOutputPort());
                _mapper[i]->SetInputConnection(gf->GetOutputPort());
            }
#endif
        } else {
#ifdef MESH_POINT_CLOUDS
            // Data Set is a 3D point cloud
            // Result of Delaunay3D mesher is unstructured grid
            vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
            mesher->SetInput(pd);
            // Sample a plane within the grid bounding box
            for (int i = 0; i < 3; i++) {
                _cutter[i]->SetInputConnection(mesher->GetOutputPort());
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(_cutter[i]->GetOutputPort());
                _mapper[i]->SetInputConnection(gf->GetOutputPort());
            }
#else
            vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
            splatter->SetInput(pd);
            int dims[3];
            splatter->GetSampleDimensions(dims);
            TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
            for (int i = 0; i < 3; i++) {
                _cutter[i]->SetInputConnection(splatter->GetOutputPort());
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(_cutter[i]->GetOutputPort());
                _mapper[i]->SetInputConnection(gf->GetOutputPort());
            }
#endif
        }
    } else {
        // DataSet can be: image/volume/uniform grid, structured grid, unstructured grid, rectilinear grid, or
        // PolyData with cells other than points
        DataSet::PrincipalPlane plane;
        double offset;
        if (!_dataSet->is2D(&plane, &offset)) {
            // Sample a plane within the grid bounding box
            for (int i = 0; i < 3; i++) {
                _cutter[i]->SetInput(ds);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(_cutter[i]->GetOutputPort());
                _mapper[i]->SetInputConnection(gf->GetOutputPort());
            }
        } else {
            // 2D data
            if (plane == DataSet::PLANE_ZY) {
                _actor[1]->VisibilityOff();
                _actor[2]->VisibilityOff();
            } else if (plane == DataSet::PLANE_XZ) {
                _actor[0]->VisibilityOff();
                _actor[2]->VisibilityOff();
            } else if (offset != 0.0) {
                // XY with Z offset
                _actor[0]->VisibilityOff();
                _actor[1]->VisibilityOff();
            }
            for (int i = 0; i < 3; i++) {
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInput(ds);
                _mapper[i]->SetInputConnection(gf->GetOutputPort());
            }
        }
    }

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }

    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL) {
            _actor[i]->SetMapper(_mapper[i]);
            _mapper[i]->Update();
        }
    }
}


/**
 * \brief Select a 2D slice plane from a 3D DataSet
 *
 * \param[in] axis Axis of slice plane
 * \param[in] ratio Position [0,1] of slice plane along axis
 */
void Cutplane::selectVolumeSlice(Axis axis, double ratio)
{
    if (_dataSet->is2D()) {
        WARN("DataSet not 3D, returning");
        return;
    }

    if ((axis == X_AXIS &&_cutPlane[0] == NULL) ||
        (axis == Y_AXIS &&_cutPlane[1] == NULL) ||
        (axis == Z_AXIS &&_cutPlane[2] == NULL)) {
        WARN("Called before update() or DataSet is not a volume");
        return;
    }

    double bounds[6];
    _dataSet->getBounds(bounds);
    switch (axis) {
    case X_AXIS:
        _cutPlane[0]->SetOrigin(bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                0,
                                0);
        if (_mapper[0] != NULL)
            _mapper[0]->Update();
        break;
    case Y_AXIS:
        _cutPlane[1]->SetOrigin(0,
                                bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                0);
        if (_mapper[1] != NULL)
            _mapper[1]->Update();
        break;
    case Z_AXIS:
        _cutPlane[2]->SetOrigin(0,
                                0,
                                bounds[4] + (bounds[5]-bounds[4]) * ratio);
        if (_mapper[2] != NULL)
            _mapper[2]->Update();
        break;
    default:
        ERROR("Invalid Axis");
        return;
    }
}

void Cutplane::updateRanges(bool useCumulative,
                            double scalarRange[2],
                            double vectorMagnitudeRange[2],
                            double vectorComponentRange[3][2])
{
    if (useCumulative) {
        _dataRange[0] = scalarRange[0];
        _dataRange[1] = scalarRange[1];
        _vectorMagnitudeRange[0] = vectorMagnitudeRange[0];
        _vectorMagnitudeRange[1] = vectorMagnitudeRange[1];
        for (int i = 0; i < 3; i++) {
            _vectorComponentRange[i][0] = vectorComponentRange[i][0];
            _vectorComponentRange[i][1] = vectorComponentRange[i][1];
        }
    } else if (_dataSet != NULL) {
        _dataSet->getScalarRange(_dataRange);
        _dataSet->getVectorRange(_vectorMagnitudeRange);
        for (int i = 0; i < 3; i++) {
            _dataSet->getVectorRange(_vectorComponentRange[i], i);
        }
    }

    // Need to update color map ranges and/or active vector field
    setColorMode(_colorMode);
}

void Cutplane::setColorMode(ColorMode mode)
{
    _colorMode = mode;
    if (_dataSet == NULL ||
        _mapper[0] == NULL ||
        _mapper[1] == NULL ||
        _mapper[2] == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    switch (mode) {
    case COLOR_BY_SCALAR: {
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
            _mapper[i]->SetScalarModeToDefault();
        }
        if (_lut != NULL) {
            _lut->SetRange(_dataRange);
        }
    }
        break;
    case COLOR_BY_VECTOR_MAGNITUDE: {
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetScalarModeToUsePointFieldData();
                _mapper[i]->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
            }
        } else if (ds->GetCellData() != NULL &&
                   ds->GetCellData()->GetVectors() != NULL) {
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetScalarModeToUseCellFieldData();
                _mapper[i]->SelectColorArray(ds->GetCellData()->GetVectors()->GetName());
            }
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorMagnitudeRange);
            _lut->SetVectorModeToMagnitude();
        }
    }
        break;
    case COLOR_BY_VECTOR_X:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetScalarModeToUsePointFieldData();
                _mapper[i]->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
            }
        } else if (ds->GetCellData() != NULL &&
                   ds->GetCellData()->GetVectors() != NULL) {
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetScalarModeToUseCellFieldData();
                _mapper[i]->SelectColorArray(ds->GetCellData()->GetVectors()->GetName());
            }
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[0]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetScalarModeToUsePointFieldData();
                _mapper[i]->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
            }
        } else if (ds->GetCellData() != NULL &&
                   ds->GetCellData()->GetVectors() != NULL) {
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetScalarModeToUseCellFieldData();
                _mapper[i]->SelectColorArray(ds->GetCellData()->GetVectors()->GetName());
            }
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[1]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetScalarModeToUsePointFieldData();
                _mapper[i]->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
            }
        } else if (ds->GetCellData() != NULL &&
                   ds->GetCellData()->GetVectors() != NULL) {
            for (int i = 0; i < 3; i++) {
                _mapper[i]->SetScalarModeToUseCellFieldData();
                _mapper[i]->SelectColorArray(ds->GetCellData()->GetVectors()->GetName());
            }
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[2]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    default:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOff();
        }
        break;
    }
}

/**
 * \brief Called when the color map has been edited
 */
void Cutplane::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Cutplane::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        for (int i = 0; i < 3; i++) {
            if (_mapper[i] != NULL) {
                _mapper[i]->UseLookupTableScalarRangeOn();
                _mapper[i]->SetLookupTable(_lut);
            }
        }
    }

    _lut->DeepCopy(cmap->getLookupTable());

    switch (_colorMode) {
    case COLOR_BY_SCALAR:
        _lut->SetRange(_dataRange);
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        _lut->SetVectorModeToMagnitude();
        _lut->SetRange(_vectorMagnitudeRange);
        break;
    case COLOR_BY_VECTOR_X:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(0);
        _lut->SetRange(_vectorComponentRange[0]);
        break;
    case COLOR_BY_VECTOR_Y:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(1);
        _lut->SetRange(_vectorComponentRange[1]);
        break;
    case COLOR_BY_VECTOR_Z:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(2);
        _lut->SetRange(_vectorComponentRange[2]);
        break;
    default:
         break;
    }
}

/**
 * \brief Set visibility of cutplane on specified axis
 */
void Cutplane::setSliceVisibility(Axis axis, bool state)
{
    switch (axis) {
    case X_AXIS:
        if (_actor[0] != NULL)
            _actor[0]->SetVisibility((state ? 1 : 0));
        break;
    case Y_AXIS:
        if (_actor[1] != NULL)
            _actor[1]->SetVisibility((state ? 1 : 0));
        break;
    case Z_AXIS:
    default:
        if (_actor[2] != NULL)
            _actor[2]->SetVisibility((state ? 1 : 0));
        break;
    }
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Cutplane::setClippingPlanes(vtkPlaneCollection *planes)
{
    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL) {
            _mapper[i]->SetClippingPlanes(planes);
        }
    }
}
