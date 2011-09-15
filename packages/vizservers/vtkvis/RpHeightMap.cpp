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
#include <vtkCellDataToPointData.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkTransform.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkContourFilter.h>
#include <vtkStripper.h>
#include <vtkWarpScalar.h>
#include <vtkPropAssembly.h>
#include <vtkCutter.h>
#include <vtkPlane.h>

#include "RpHeightMap.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

#define MESH_POINT_CLOUDS

HeightMap::HeightMap(int numContours, double heightScale) :
    VtkGraphicsObject(),
    _numContours(numContours),
    _colorMap(NULL),
    _contourEdgeWidth(1.0),
    _warpScale(heightScale),
    _sliceAxis(Z_AXIS),
    _pipelineInitialized(false)
{
    _contourEdgeColor[0] = 1.0f;
    _contourEdgeColor[1] = 0.0f;
    _contourEdgeColor[2] = 0.0f;
}

HeightMap::HeightMap(const std::vector<double>& contours, double heightScale) :
    VtkGraphicsObject(),
    _numContours(contours.size()),
    _contours(contours),
    _colorMap(NULL),
    _contourEdgeWidth(1.0),
    _warpScale(heightScale),
    _sliceAxis(Z_AXIS),
    _pipelineInitialized(false)
{
    _contourEdgeColor[0] = 1.0f;
    _contourEdgeColor[1] = 0.0f;
    _contourEdgeColor[2] = 0.0f;
}

HeightMap::~HeightMap()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting HeightMap for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting HeightMap with NULL DataSet");
#endif
}

void HeightMap::setDataSet(DataSet *dataSet,
                           bool useCumulative,
                           double scalarRange[2],
                           double vectorMagnitudeRange[2],
                           double vectorComponentRange[3][2])
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        if (_dataSet != NULL) {
            if (useCumulative) {
                _dataRange[0] = scalarRange[0];
                _dataRange[1] = scalarRange[1];
            } else {
                _dataSet->getScalarRange(_dataRange);
            }

            // Compute a data scaling factor to make maximum
            // height (at default _warpScale) equivalent to 
            // largest dimension of data set
            double bounds[6];
            double boundsRange = 0.0;
            _dataSet->getBounds(bounds);
            for (int i = 0; i < 6; i+=2) {
                double r = bounds[i+1] - bounds[i];
                if (r > boundsRange)
                    boundsRange = r;
            }
            double datRange = _dataRange[1] - _dataRange[0];
            if (datRange < 1.0e-10) {
                _dataScale = 1.0;
            } else {
                _dataScale = boundsRange / datRange;
            }
            TRACE("Bounds range: %g data range: %g scaling: %g",
                  boundsRange, datRange, _dataScale);
        }

        update();
    }
}

/**
 * \brief Create and initialize VTK Props to render the colormapped dataset
 */
void HeightMap::initProp()
{
    if (_dsActor == NULL) {
        _dsActor = vtkSmartPointer<vtkActor>::New();
        _dsActor->GetProperty()->SetOpacity(_opacity);
        _dsActor->GetProperty()->SetEdgeColor(_edgeColor[0],
                                              _edgeColor[1],
                                              _edgeColor[2]);
        _dsActor->GetProperty()->SetLineWidth(_edgeWidth);
        _dsActor->GetProperty()->EdgeVisibilityOff();
        _dsActor->GetProperty()->SetAmbient(.2);
        _dsActor->GetProperty()->LightingOn();
    }
    if (_contourActor == NULL) {
        _contourActor = vtkSmartPointer<vtkActor>::New();
        _contourActor->GetProperty()->SetOpacity(_opacity);
        _contourActor->GetProperty()->SetColor(_contourEdgeColor[0],
                                               _contourEdgeColor[1],
                                               _contourEdgeColor[2]);
        _contourActor->GetProperty()->SetEdgeColor(_contourEdgeColor[0],
                                                   _contourEdgeColor[1],
                                                   _contourEdgeColor[2]);
        _contourActor->GetProperty()->SetLineWidth(_contourEdgeWidth);
        _contourActor->GetProperty()->EdgeVisibilityOff();
        _contourActor->GetProperty()->SetAmbient(.2);
        _contourActor->GetProperty()->LightingOff();
    }
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkAssembly>::New();
        getAssembly()->AddPart(_dsActor);
        getAssembly()->AddPart(_contourActor);
    }
}

/**
 * \brief Internal method to set up pipeline after a state change
 */
void HeightMap::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    TRACE("DataSet type: %s", _dataSet->getVtkType());

    // Contour filter to generate isolines
    if (_contourFilter == NULL) {
        _contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    }

    // Mapper, actor to render color-mapped data set
    if (_dsMapper == NULL) {
        _dsMapper = vtkSmartPointer<vtkDataSetMapper>::New();
        // Map scalars through lookup table regardless of type
        _dsMapper->SetColorModeToMapScalars();
    }

    if (!_pipelineInitialized) {
        vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

        if (ds->GetPointData() == NULL ||
            ds->GetPointData()->GetScalars() == NULL) {
            WARN("No scalar point data in dataset %s", _dataSet->getName().c_str());
            if (ds->GetCellData() != NULL &&
                ds->GetCellData()->GetScalars() != NULL) {
                cellToPtData = 
                    vtkSmartPointer<vtkCellDataToPointData>::New();
                cellToPtData->SetInput(ds);
                //cellToPtData->PassCellDataOn();
                cellToPtData->Update();
                ds = cellToPtData->GetOutput();
            } else {
                ERROR("No scalar cell data in dataset %s", _dataSet->getName().c_str());
                return;
            }
        }

        vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
        if (pd != NULL) {
            // DataSet is a vtkPolyData
            if (pd->GetNumberOfLines() == 0 &&
                pd->GetNumberOfPolys() == 0 &&
                pd->GetNumberOfStrips() == 0) {
                // DataSet is a point cloud
                DataSet::PrincipalPlane plane;
                double offset;
                if (_dataSet->is2D(&plane, &offset)) {
#ifdef MESH_POINT_CLOUDS
                    // Result of Delaunay2D is a PolyData
                    vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                    if (plane == DataSet::PLANE_ZY) {
                        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                        trans->RotateWXYZ(90, 0, 1, 0);
                        if (offset != 0.0) {
                            trans->Translate(-offset, 0, 0);
                        }
                        mesher->SetTransform(trans);
                        _sliceAxis = X_AXIS;
                    } else if (plane == DataSet::PLANE_XZ) {
                        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                        trans->RotateWXYZ(-90, 1, 0, 0);
                        if (offset != 0.0) {
                            trans->Translate(0, -offset, 0);
                        }
                        mesher->SetTransform(trans);
                        _sliceAxis = Y_AXIS;
                    } else if (offset != 0.0) {
                        // XY with Z offset
                        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                        trans->Translate(0, 0, -offset);
                        mesher->SetTransform(trans);
                    }
                    mesher->SetInput(pd);
                    vtkAlgorithmOutput *warpOutput = initWarp(mesher->GetOutputPort());
                    _dsMapper->SetInputConnection(warpOutput);
                    _contourFilter->SetInputConnection(warpOutput);
#else
                    if (_pointSplatter == NULL)
                        _pointSplatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                    if (_volumeSlicer == NULL)
                        _volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
                    _pointSplatter->SetInput(pd);
                    int dims[3];
                    _pointSplatter->GetSampleDimensions(dims);
                    TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                    if (plane == DataSet::PLANE_ZY) {
                        dims[0] = 3;
                        _volumeSlicer->SetVOI(1, 1, 0, dims[1]-1, 0, dims[1]-1);
                        _sliceAxis = X_AXIS;
                    } else if (plane == DataSet::PLANE_XZ) {
                        dims[1] = 3;
                        _volumeSlicer->SetVOI(0, dims[0]-1, 1, 1, 0, dims[2]-1);
                        _sliceAxis = Y_AXIS;
                    } else {
                        dims[2] = 3;
                        _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                    }
                    _pointSplatter->SetSampleDimensions(dims);
                    double bounds[6];
                    _pointSplatter->Update();
                    _pointSplatter->GetModelBounds(bounds);
                    TRACE("Model bounds: %g %g %g %g %g %g",
                          bounds[0], bounds[1],
                          bounds[2], bounds[3],
                          bounds[4], bounds[5]);
                    _volumeSlicer->SetInputConnection(_pointSplatter->GetOutputPort());
                    _volumeSlicer->SetSampleRate(1, 1, 1);
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(_volumeSlicer->GetOutputPort());
                    vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
                    _dsMapper->SetInputConnection(warpOutput);
                    _contourFilter->SetInputConnection(warpOutput);
#endif
                } else {
#ifdef MESH_POINT_CLOUDS
                    // Data Set is a 3D point cloud
                    // Result of Delaunay3D mesher is unstructured grid
                    vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
                    mesher->SetInput(pd);
                    // Run the mesher
                    mesher->Update();
                    // Get bounds of resulting grid
                    double bounds[6];
                    mesher->GetOutput()->GetBounds(bounds);
                    // Sample a plane within the grid bounding box
                    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
                    cutter->SetInputConnection(mesher->GetOutputPort());
                    if (_cutPlane == NULL) {
                        _cutPlane = vtkSmartPointer<vtkPlane>::New();
                    }
                    _cutPlane->SetNormal(0, 0, 1);
                    _cutPlane->SetOrigin(0,
                                         0,
                                         bounds[4] + (bounds[5]-bounds[4])/2.);
                    cutter->SetCutFunction(_cutPlane);
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(cutter->GetOutputPort());
#else
                    if (_pointSplatter == NULL)
                        _pointSplatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                    _pointSplatter->SetInput(pd);
                    int dims[3];
                    _pointSplatter->GetSampleDimensions(dims);
                    TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                    dims[2] = 3;
                    _pointSplatter->SetSampleDimensions(dims);
                    double bounds[6];
                    _pointSplatter->Update();
                    _pointSplatter->GetModelBounds(bounds);
                    TRACE("Model bounds: %g %g %g %g %g %g",
                          bounds[0], bounds[1],
                          bounds[2], bounds[3],
                          bounds[4], bounds[5]);
                    if (_volumeSlicer == NULL)
                        _volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
                    _volumeSlicer->SetInputConnection(_pointSplatter->GetOutputPort());
                    _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                    _volumeSlicer->SetSampleRate(1, 1, 1);
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(_volumeSlicer->GetOutputPort());
#endif
                    vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
                    _dsMapper->SetInputConnection(warpOutput);
                    _contourFilter->SetInputConnection(warpOutput);
                }
            } else {
                // DataSet is a vtkPolyData with lines and/or polygons
                vtkAlgorithmOutput *warpOutput = initWarp(pd);
                if (warpOutput != NULL) {
                    _dsMapper->SetInputConnection(warpOutput);
                    _contourFilter->SetInputConnection(warpOutput);
                } else {
                    _dsMapper->SetInput(pd);
                    _contourFilter->SetInput(pd);
                }
            }
        } else {
            // DataSet is NOT a vtkPolyData
            // Can be: image/volume/uniform grid, structured grid, unstructured grid, rectilinear grid
            vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
            gf->UseStripsOn();
            vtkImageData *imageData = vtkImageData::SafeDownCast(ds);
            if (!_dataSet->is2D() && imageData != NULL) {
                // 3D image/volume/uniform grid
                if (_volumeSlicer == NULL)
                    _volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
                int dims[3];
                imageData->GetDimensions(dims);
                TRACE("Image data dimensions: %d %d %d", dims[0], dims[1], dims[2]);
                _volumeSlicer->SetInput(ds);
                _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, (dims[2]-1)/2, (dims[2]-1)/2);
                _volumeSlicer->SetSampleRate(1, 1, 1);
                gf->SetInputConnection(_volumeSlicer->GetOutputPort());
            } else if (!_dataSet->is2D() && imageData == NULL) {
                // 3D structured grid, unstructured grid, or rectilinear grid
                double bounds[6];
                ds->GetBounds(bounds);
                // Sample a plane within the grid bounding box
                vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
                cutter->SetInput(ds);
                if (_cutPlane == NULL) {
                    _cutPlane = vtkSmartPointer<vtkPlane>::New();
                }
                _cutPlane->SetNormal(0, 0, 1);
                _cutPlane->SetOrigin(0,
                                     0,
                                     bounds[4] + (bounds[5]-bounds[4])/2.);
                cutter->SetCutFunction(_cutPlane);
                gf->SetInputConnection(cutter->GetOutputPort());
            } else {
                // 2D data
                gf->SetInput(ds);
            }
            vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
            _dsMapper->SetInputConnection(warpOutput);
            _contourFilter->SetInputConnection(warpOutput);
        }
    }

    _pipelineInitialized = true;

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }
    //_dsMapper->InterpolateScalarsBeforeMappingOn();

    initProp();

    _contourFilter->ComputeNormalsOff();
    _contourFilter->ComputeGradientsOff();

    // Speed up multiple contour computation at cost of extra memory use
    if (_numContours > 1) {
        _contourFilter->UseScalarTreeOn();
    } else {
        _contourFilter->UseScalarTreeOff();
    }

    _contourFilter->SetNumberOfContours(_numContours);

    if (_contours.empty()) {
        // Evenly spaced isovalues
        _contourFilter->GenerateValues(_numContours, _dataRange[0], _dataRange[1]);
    } else {
        // User-supplied isovalues
        for (int i = 0; i < _numContours; i++) {
            _contourFilter->SetValue(i, _contours[i]);
        }
    }
    if (_contourMapper == NULL) {
        _contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _contourMapper->ScalarVisibilityOff();
        _contourMapper->SetResolveCoincidentTopologyToPolygonOffset();
        vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
        stripper->SetInputConnection(_contourFilter->GetOutputPort());
        _contourMapper->SetInputConnection(stripper->GetOutputPort());
        _contourActor->SetMapper(_contourMapper);
    }

    _dsActor->SetMapper(_dsMapper);

    _dsMapper->Update();
    _contourMapper->Update();
}

vtkAlgorithmOutput *HeightMap::initWarp(vtkAlgorithmOutput *input)
{
    TRACE("Warp scale: %g", _warpScale);
    if (_warpScale == 0.0) {
        _warp = NULL;
        return input;
    } else if (input == NULL) {
        ERROR("NULL input");
        return input;
    } else {
        if (_warp == NULL)
            _warp = vtkSmartPointer<vtkWarpScalar>::New();
        switch (_sliceAxis) {
        case X_AXIS:
            _warp->SetNormal(1, 0, 0);
            break;
        case Y_AXIS:
            _warp->SetNormal(0, 1, 0);
            break;
        default:
            _warp->SetNormal(0, 0, 1);
        }
        _warp->UseNormalOn();
        _warp->SetScaleFactor(_warpScale * _dataScale);
        _warp->SetInputConnection(input);
        return _warp->GetOutputPort();
    }
}

vtkAlgorithmOutput *HeightMap::initWarp(vtkPolyData *pdInput)
{
    TRACE("Warp scale: %g", _warpScale);
    if (_warpScale == 0.0) {
        _warp = NULL;
        return NULL;
    } else if (pdInput == NULL) {
        ERROR("NULL input");
        return NULL;
    } else {
        if (_warp == NULL)
            _warp = vtkSmartPointer<vtkWarpScalar>::New();
        switch (_sliceAxis) {
        case X_AXIS:
            _warp->SetNormal(1, 0, 0);
            break;
        case Y_AXIS:
            _warp->SetNormal(0, 1, 0);
            break;
        default:
            _warp->SetNormal(0, 0, 1);
        }
        _warp->UseNormalOn();
        _warp->SetScaleFactor(_warpScale * _dataScale);
        _warp->SetInput(pdInput);
        return _warp->GetOutputPort();
    }
}

/**
 * \brief Controls relative scaling of height of mountain plot
 */
void HeightMap::setHeightScale(double scale)
{
    if (_warpScale == scale)
        return;

    _warpScale = scale;
    if (_warp == NULL) {
        vtkAlgorithmOutput *warpOutput = initWarp(_dsMapper->GetInputConnection(0, 0));
        _dsMapper->SetInputConnection(warpOutput);
        _contourFilter->SetInputConnection(warpOutput);
    } else if (scale == 0.0) {
        vtkAlgorithmOutput *warpInput = _warp->GetInputConnection(0, 0);
        _dsMapper->SetInputConnection(warpInput);
        _contourFilter->SetInputConnection(warpInput);
        _warp = NULL;
    } else {
        _warp->SetScaleFactor(_warpScale * _dataScale);
    }

    if (_dsMapper != NULL)
        _dsMapper->Update();
    if (_contourMapper != NULL)
        _contourMapper->Update();
}

/**
 * \brief Select a 2D slice plane from a 3D DataSet
 *
 * \param[in] axis Axis of slice plane
 * \param[in] ratio Position [0,1] of slice plane along axis
 */
void HeightMap::selectVolumeSlice(Axis axis, double ratio)
{
    if (_dataSet->is2D()) {
        WARN("DataSet not 3D, returning");
        return;
    }

    if (_volumeSlicer == NULL &&
        _cutPlane == NULL) {
        WARN("Called before update() or DataSet is not a volume");
        return;
    }

    _sliceAxis = axis;
    if (_warp != NULL) {
        switch (axis) {
        case X_AXIS:
            _warp->SetNormal(1, 0, 0);
            break;
        case Y_AXIS:
            _warp->SetNormal(0, 1, 0);
            break;
        case Z_AXIS:
            _warp->SetNormal(0, 0, 1);
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }
    }

    if (_cutPlane != NULL) {
        double bounds[6];
        _dataSet->getBounds(bounds);
        switch (axis) {
        case X_AXIS:
            _cutPlane->SetNormal(1, 0, 0);
            _cutPlane->SetOrigin(bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                 0,
                                 0);
            break;
        case Y_AXIS:
            _cutPlane->SetNormal(0, 1, 0);
            _cutPlane->SetOrigin(0,
                                 bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                 0);
            break;
        case Z_AXIS:
            _cutPlane->SetNormal(0, 0, 1);
            _cutPlane->SetOrigin(0,
                                 0,
                                 bounds[4] + (bounds[5]-bounds[4]) * ratio);
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }
    } else {
        int dims[3];
        if (_pointSplatter != NULL) {
            _pointSplatter->GetSampleDimensions(dims);
        } else {
            vtkImageData *imageData = vtkImageData::SafeDownCast(_dataSet->getVtkDataSet());
            if (imageData == NULL) {
                ERROR("Not a volume data set");
                return;
            }
            imageData->GetDimensions(dims);
        }
        int voi[6];

        switch (axis) {
        case X_AXIS:
            voi[0] = voi[1] = (int)((dims[0]-1) * ratio);
            voi[2] = 0;
            voi[3] = dims[1]-1;
            voi[4] = 0;
            voi[5] = dims[2]-1;
            break;
        case Y_AXIS:
            voi[0] = 0;
            voi[1] = dims[0]-1;
            voi[2] = voi[3] = (int)((dims[1]-1) * ratio);
            voi[4] = 0;
            voi[5] = dims[2]-1;
            break;
        case Z_AXIS:
            voi[0] = 0;
            voi[1] = dims[0]-1;
            voi[2] = 0;
            voi[3] = dims[1]-1;
            voi[4] = voi[5] = (int)((dims[2]-1) * ratio);
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }

        _volumeSlicer->SetVOI(voi);
    }

    if (_dsMapper != NULL)
        _dsMapper->Update();
    if (_contourMapper != NULL)
        _contourMapper->Update();
}

/**
 * \brief Called when the color map has been edited
 */
void HeightMap::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void HeightMap::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        if (_dsMapper != NULL) {
            _dsMapper->UseLookupTableScalarRangeOn();
            _dsMapper->SetLookupTable(_lut);
        }
    }

    _lut->DeepCopy(cmap->getLookupTable());
    _lut->SetRange(_dataRange);
}

void HeightMap::updateRanges(bool useCumulative,
                             double scalarRange[2],
                             double vectorMagnitudeRange[2],
                             double vectorComponentRange[3][2])
{
    if (useCumulative) {
        _dataRange[0] = scalarRange[0];
        _dataRange[1] = scalarRange[1];
    } else if (_dataSet != NULL) {
        _dataSet->getScalarRange(_dataRange);
    }

    if (_lut != NULL) {
        _lut->SetRange(_dataRange);
    }

    if (_contours.empty() && _numContours > 0) {
        // Need to recompute isovalues
        update();
    }
}

/**
 * \brief Specify number of evenly spaced contour lines to render
 *
 * Will override any existing contours
 */
void HeightMap::setNumContours(int numContours)
{
    _contours.clear();
    _numContours = numContours;

    update();
}

/**
 * \brief Specify an explicit list of contour isovalues
 *
 * Will override any existing contours
 */
void HeightMap::setContourList(const std::vector<double>& contours)
{
    _contours = contours;
    _numContours = (int)_contours.size();

    update();
}

/**
 * \brief Get the number of contours
 */
int HeightMap::getNumContours() const
{
    return _numContours;
}

/**
 * \brief Get the contour list (may be empty if number of contours
 * was specified in place of a list)
 */
const std::vector<double>& HeightMap::getContourList() const
{
    return _contours;
}

/**
 * \brief Turn on/off lighting of this object
 */
void HeightMap::setLighting(bool state)
{
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetLighting((state ? 1 : 0));
}

/**
 * \brief Turn on/off rendering of mesh edges
 */
void HeightMap::setEdgeVisibility(bool state)
{
    if (_dsActor != NULL) {
        _dsActor->GetProperty()->SetEdgeVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Turn on/off rendering of colormaped surface
 */
void HeightMap::setContourSurfaceVisibility(bool state)
{
    if (_dsActor != NULL) {
        _dsActor->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Turn on/off rendering of contour isolines
 */
void HeightMap::setContourLineVisibility(bool state)
{
    if (_contourActor != NULL) {
        _contourActor->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Set RGB color of polygon edges
 */
void HeightMap::setEdgeColor(float color[3])
{
    _edgeColor[0] = color[0];
    _edgeColor[1] = color[1];
    _edgeColor[2] = color[2];
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
}

/**
 * \brief Set pixel width of polygon edges (may be a no-op)
 */
void HeightMap::setEdgeWidth(float edgeWidth)
{
    _edgeWidth = edgeWidth;
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetLineWidth(_edgeWidth);
}

/**
 * \brief Set RGB color of contour isolines
 */
void HeightMap::setContourEdgeColor(float color[3])
{
    _contourEdgeColor[0] = color[0];
    _contourEdgeColor[1] = color[1];
    _contourEdgeColor[2] = color[2];
    if (_contourActor != NULL) {
        _contourActor->GetProperty()->SetColor(_contourEdgeColor[0],
                                               _contourEdgeColor[1],
                                               _contourEdgeColor[2]);
        _contourActor->GetProperty()->SetEdgeColor(_contourEdgeColor[0],
                                                   _contourEdgeColor[1],
                                                   _contourEdgeColor[2]);
    }
}

/**
 * \brief Set pixel width of contour isolines (may be a no-op)
 */
void HeightMap::setContourEdgeWidth(float edgeWidth)
{
    _contourEdgeWidth = edgeWidth;
    if (_contourActor != NULL)
        _contourActor->GetProperty()->SetLineWidth(_contourEdgeWidth);
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void HeightMap::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_dsMapper != NULL) {
        _dsMapper->SetClippingPlanes(planes);
    }
    if (_contourMapper != NULL) {
        _contourMapper->SetClippingPlanes(planes);
    }
}

