/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
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

#include "HeightMap.h"
#include "Renderer.h"
#include "Trace.h"

using namespace VtkVis;

#define TRANSLATE_TO_ZORIGIN

HeightMap::HeightMap(int numContours, double heightScale) :
    GraphicsObject(),
    _numContours(numContours),
    _contourColorMap(false),
    _contourEdgeWidth(1.0),
    _warpScale(heightScale),
    _sliceAxis(Z_AXIS),
    _pipelineInitialized(false),
    _cloudStyle(CLOUD_MESH),
    _colorMap(NULL),
    _colorMode(COLOR_BY_SCALAR),
    _colorFieldType(DataSet::POINT_DATA),
    _renderer(NULL)
{
    _contourEdgeColor[0] = 0.0f;
    _contourEdgeColor[1] = 0.0f;
    _contourEdgeColor[2] = 0.0f;
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
}

HeightMap::HeightMap(const std::vector<double>& contours, double heightScale) :
    GraphicsObject(),
    _numContours(contours.size()),
    _contours(contours),
    _contourColorMap(false),
    _contourEdgeWidth(1.0),
    _warpScale(heightScale),
    _sliceAxis(Z_AXIS),
    _pipelineInitialized(false),
    _cloudStyle(CLOUD_MESH),
    _colorMap(NULL),
    _colorMode(COLOR_BY_SCALAR),
    _colorFieldType(DataSet::POINT_DATA),
    _renderer(NULL)
{
    _contourEdgeColor[0] = 0.0f;
    _contourEdgeColor[1] = 0.0f;
    _contourEdgeColor[2] = 0.0f;
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
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
                           Renderer *renderer)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;
        _renderer = renderer;

        if (_dataSet != NULL) {
            TRACE("DataSet name: '%s' type: %s",
                  _dataSet->getName().c_str(),
                  _dataSet->getVtkType());

            if (renderer->getUseCumulativeRange()) {
                renderer->getCumulativeDataRange(_dataRange,
                                                 _dataSet->getActiveScalarsName(),
                                                 1);
                const char *activeVectors = _dataSet->getActiveVectorsName();
                if (activeVectors != NULL) {
                    renderer->getCumulativeDataRange(_vectorMagnitudeRange,
                                                     activeVectors,
                                                     3);
                    for (int i = 0; i < 3; i++) {
                        renderer->getCumulativeDataRange(_vectorComponentRange[i],
                                                         activeVectors,
                                                         3, i);
                    }
                }
            } else {
                _dataSet->getScalarRange(_dataRange);
                _dataSet->getVectorRange(_vectorMagnitudeRange);
                for (int i = 0; i < 3; i++) {
                    _dataSet->getVectorRange(_vectorComponentRange[i], i);
                }
            }
        }

        update();
    }
}

/**
 * Compute a data scaling factor to make maximum height (at default _warpScale) 
 * equivalent to largest dimension of data set
 */
void HeightMap::computeDataScale()
{
    if (_dataSet == NULL)
        return;

    double bounds[6];
    double boundsRange = 0.0;
    _dataSet->getBounds(bounds);
    for (int i = 0; i < 6; i+=2) {
        double r = bounds[i+1] - bounds[i];
        if (r > boundsRange)
            boundsRange = r;
    }
    double datRange = _dataRange[1] - _dataRange[0];
    if (datRange < 1.0e-17) {
        _dataScale = 1.0;
    } else {
        _dataScale = boundsRange / datRange;
    }
    TRACE("Bounds range: %g data range: %g scaling: %g",
          boundsRange, datRange, _dataScale);
}

/**
 * \brief Create and initialize VTK Props to render the colormapped dataset
 */
void HeightMap::initProp()
{
    if (_dsActor == NULL) {
        _dsActor = vtkSmartPointer<vtkActor>::New();
        _dsActor->GetProperty()->SetOpacity(_opacity);
        _dsActor->GetProperty()->SetColor(_color[0],
                                          _color[1],
                                          _color[2]);
        _dsActor->GetProperty()->SetEdgeColor(_edgeColor[0],
                                              _edgeColor[1],
                                              _edgeColor[2]);
        _dsActor->GetProperty()->SetLineWidth(_edgeWidth);
        _dsActor->GetProperty()->EdgeVisibilityOff();
        _dsActor->GetProperty()->SetAmbient(0.2);
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
        _contourActor->GetProperty()->SetAmbient(1.0);
        _contourActor->GetProperty()->LightingOff();
    }
    // Contour actor is added in update() if contours are produced
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkAssembly>::New();
        getAssembly()->AddPart(_dsActor);
    } else {
        getAssembly()->RemovePart(_contourActor);
    }
}

/**
 * \brief Internal method to set up pipeline after a state change
 */
void HeightMap::update()
{
    if (_dataSet == NULL)
        return;

    TRACE("DataSet: %s", _dataSet->getName().c_str());

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    computeDataScale();

    // Contour filter to generate isolines
    if (_contourFilter == NULL) {
        _contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    }

    // Mapper, actor to render color-mapped data set
    if (_mapper == NULL) {
        _mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        // Map scalars through lookup table regardless of type
        _mapper->SetColorModeToMapScalars();
    }

    if (!_pipelineInitialized) {
        vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

        if (ds->GetPointData() == NULL ||
            ds->GetPointData()->GetScalars() == NULL) {
            TRACE("No scalar point data in dataset %s", _dataSet->getName().c_str());
            if (ds->GetCellData() != NULL &&
                ds->GetCellData()->GetScalars() != NULL) {
                cellToPtData = 
                    vtkSmartPointer<vtkCellDataToPointData>::New();
#ifdef USE_VTK6
                cellToPtData->SetInputData(ds);
#else
                cellToPtData->SetInput(ds);
#endif
                //cellToPtData->PassCellDataOn();
                cellToPtData->Update();
                ds = cellToPtData->GetOutput();
            } else {
                USER_ERROR("No scalar field was found in the data set.");
                return;
            }
        }

        _splatter = NULL;

        if (_dataSet->isCloud()) {
            // DataSet is a point cloud
            PrincipalPlane plane;
            double offset;
            if (_dataSet->is2D(&plane, &offset)) {
                if (_cloudStyle == CLOUD_MESH) {
                    // Result of Delaunay2D is a PolyData
                    vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                    if (plane == PLANE_ZY) {
                        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                        trans->RotateWXYZ(90, 0, 1, 0);
                        if (offset != 0.0) {
                            trans->Translate(-offset, 0, 0);
                        }
                        mesher->SetTransform(trans);
                        _sliceAxis = X_AXIS;
                    } else if (plane == PLANE_XZ) {
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
#ifdef USE_VTK6
                    mesher->SetInputData(ds);
#else
                    mesher->SetInput(ds);
#endif
                    vtkAlgorithmOutput *warpOutput = initWarp(mesher->GetOutputPort());
                    _mapper->SetInputConnection(warpOutput);
                    _contourFilter->SetInputConnection(warpOutput);
                } else {
                    // _cloudStyle == CLOUD_SPLAT
                    if (_splatter == NULL)
                        _splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                    if (_volumeSlicer == NULL)
                        _volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
#ifdef USE_VTK6
                    _splatter->SetInputData(ds);
#else
                    _splatter->SetInput(ds);
#endif
                    int dims[3];
                    _splatter->GetSampleDimensions(dims);
                    TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                    if (plane == PLANE_ZY) {
                        dims[0] = 3;
                        _volumeSlicer->SetVOI(1, 1, 0, dims[1]-1, 0, dims[1]-1);
                        _sliceAxis = X_AXIS;
                    } else if (plane == PLANE_XZ) {
                        dims[1] = 3;
                        _volumeSlicer->SetVOI(0, dims[0]-1, 1, 1, 0, dims[2]-1);
                        _sliceAxis = Y_AXIS;
                    } else {
                        dims[2] = 3;
                        _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                    }
                    _splatter->SetSampleDimensions(dims);
                    double bounds[6];
                    _splatter->Update();
                    _splatter->GetModelBounds(bounds);
                    TRACE("Model bounds: %g %g %g %g %g %g",
                          bounds[0], bounds[1],
                          bounds[2], bounds[3],
                          bounds[4], bounds[5]);
                    _volumeSlicer->SetInputConnection(_splatter->GetOutputPort());
                    _volumeSlicer->SetSampleRate(1, 1, 1);
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(_volumeSlicer->GetOutputPort());
                    vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
                    _mapper->SetInputConnection(warpOutput);
                    _contourFilter->SetInputConnection(warpOutput);
                }
            } else {
                // 3D point cloud
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                if (_cloudStyle == CLOUD_MESH) {
                     // Result of Delaunay3D mesher is unstructured grid
                    vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
                    mesher->SetInputData(ds);
#else
                    mesher->SetInput(ds);
#endif
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
                    gf->SetInputConnection(cutter->GetOutputPort());
                } else {
                    // _cloudStyle == CLOUD_SPLAT
                    if (_splatter == NULL)
                        _splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
#ifdef USE_VTK6
                    _splatter->SetInputData(ds);
#else
                    _splatter->SetInput(ds);
#endif
                    int dims[3];
                    _splatter->GetSampleDimensions(dims);
                    TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                    dims[2] = 3;
                    _splatter->SetSampleDimensions(dims);
                    double bounds[6];
                    _splatter->Update();
                    _splatter->GetModelBounds(bounds);
                    TRACE("Model bounds: %g %g %g %g %g %g",
                          bounds[0], bounds[1],
                          bounds[2], bounds[3],
                          bounds[4], bounds[5]);
                    if (_volumeSlicer == NULL)
                        _volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
                    _volumeSlicer->SetInputConnection(_splatter->GetOutputPort());
                    _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                    _volumeSlicer->SetSampleRate(1, 1, 1);
                    gf->SetInputConnection(_volumeSlicer->GetOutputPort());
                }
                vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
                _mapper->SetInputConnection(warpOutput);
                _contourFilter->SetInputConnection(warpOutput);
            }
        } else if (vtkPolyData::SafeDownCast(ds) != NULL) {
            // DataSet is a vtkPolyData with lines and/or polygons
            vtkAlgorithmOutput *warpOutput = initWarp(ds);
            if (warpOutput != NULL) {
                _mapper->SetInputConnection(warpOutput);
                _contourFilter->SetInputConnection(warpOutput);
            } else {
#ifdef USE_VTK6
                _mapper->SetInputData(vtkPolyData::SafeDownCast(ds));
                _contourFilter->SetInputData(ds);
#else
                _mapper->SetInput(vtkPolyData::SafeDownCast(ds));
                _contourFilter->SetInput(ds);
#endif
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
#ifdef USE_VTK6
                _volumeSlicer->SetInputData(ds);
#else
                _volumeSlicer->SetInput(ds);
#endif
                _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, (dims[2]-1)/2, (dims[2]-1)/2);
                _volumeSlicer->SetSampleRate(1, 1, 1);
                gf->SetInputConnection(_volumeSlicer->GetOutputPort());
            } else if (!_dataSet->is2D() && imageData == NULL) {
                // 3D structured grid, unstructured grid, or rectilinear grid
                double bounds[6];
                ds->GetBounds(bounds);
                // Sample a plane within the grid bounding box
                vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
#ifdef USE_VTK6
                cutter->SetInputData(ds);
#else
                cutter->SetInput(ds);
#endif
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
#ifdef USE_VTK6
                gf->SetInputData(ds);
#else
                gf->SetInput(ds);
#endif
            }
            vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
            _mapper->SetInputConnection(warpOutput);
            _contourFilter->SetInputConnection(warpOutput);
        }
    }

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

    initProp();

    if (_contourMapper == NULL) {
        _contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _contourMapper->ScalarVisibilityOff();
        _contourMapper->SetResolveCoincidentTopologyToPolygonOffset();
        vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
        stripper->SetInputConnection(_contourFilter->GetOutputPort());
        _contourMapper->SetInputConnection(stripper->GetOutputPort());
        _contourActor->SetMapper(_contourMapper);
        //_contourActor->InterpolateScalarsBeforeMappingOn();
    }

    setInterpolateBeforeMapping(true);

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
        setColorMode(_colorMode);
    } else if (!_pipelineInitialized) {
        double *rangePtr = _colorFieldRange;
        if (_colorFieldRange[0] > _colorFieldRange[1]) {
            rangePtr = NULL;
        }
        setColorMode(_colorMode, _colorFieldType, _colorFieldName.c_str(), rangePtr);
    }

    _pipelineInitialized = true;

    // Ensure updated dataScale is applied
    if (_warp != NULL) {
        _warp->SetScaleFactor(_warpScale * _dataScale);
    }
#ifdef TRANSLATE_TO_ZORIGIN
    double pos[3];
    pos[0] = 0;
    pos[1] = 0;
    pos[2] = - _warpScale * _dataScale * _dataRange[0];
    setPosition(pos);
    TRACE("Z_POS: %g", pos[2]);
#endif
    _dsActor->SetMapper(_mapper);

    _mapper->Update();
    _contourMapper->Update();

    // Only add contour actor to assembly if contours were
    // produced, in order to prevent messing up assembly bounds
    double bounds[6];
    _contourActor->GetBounds(bounds);
    if (bounds[0] <= bounds[1]) {
        getAssembly()->AddPart(_contourActor);
    }
}

void HeightMap::setCloudStyle(CloudStyle style)
{
    if (style != _cloudStyle) {
        _cloudStyle = style;
        if (_dataSet != NULL) {
            _pipelineInitialized = false;
            update();
        }
    }
}

void HeightMap::setInterpolateBeforeMapping(bool state)
{
    if (_mapper != NULL) {
        _mapper->SetInterpolateScalarsBeforeMapping((state ? 1 : 0));
    }
}

void HeightMap::setAspect(double aspect)
{
    double bounds[6];
    vtkDataSet *ds = _dataSet->getVtkDataSet();
    ds->GetBounds(bounds);
    double size[3];
    size[0] = bounds[1] - bounds[0];
    size[1] = bounds[3] - bounds[2];
    size[2] = bounds[5] - bounds[4];
    double scale[3];
    scale[0] = scale[1] = scale[2] = 1.;

    if (aspect == 1.0) {
        // Square
        switch (_sliceAxis) {
        case X_AXIS: {
            if (size[1] > size[2] && size[2] > 1.0e-6) {
                scale[2] = size[1] / size[2];
            } else if (size[2] > size[1] && size[1] > 1.0e-6) {
                scale[1] = size[2] / size[1];
            }
        }
            break;
        case Y_AXIS: {
            if (size[0] > size[2] && size[2] > 1.0e-6) {
                scale[2] = size[0] / size[2];
            } else if (size[2] > size[0] && size[0] > 1.0e-6) {
                scale[0] = size[2] / size[0];
            }
        }
            break;
        case Z_AXIS: {
            if (size[0] > size[1] && size[1] > 1.0e-6) {
                scale[1] = size[0] / size[1];
            } else if (size[1] > size[0] && size[0] > 1.0e-6) {
                scale[0] = size[1] / size[0];
            }
        }
            break;
        }
    } else if (aspect != 0.0) {
        switch (_sliceAxis) {
        case X_AXIS: {
            if (aspect > 1.0) {
                if (size[2] > size[1]) {
                    scale[1] = (size[2] / aspect) / size[1];
                } else {
                    scale[2] = (size[1] * aspect) / size[2];
                }
            } else {
                if (size[1] > size[2]) {
                    scale[2] = (size[1] * aspect) / size[2];
                } else {
                    scale[1] = (size[2] / aspect) / size[1];
                }
            }
        }
            break;
        case Y_AXIS: {
            if (aspect > 1.0) {
                if (size[0] > size[2]) {
                    scale[2] = (size[0] / aspect) / size[2];
                } else {
                    scale[0] = (size[2] * aspect) / size[0];
                }
            } else {
                if (size[2] > size[0]) {
                    scale[0] = (size[2] * aspect) / size[0];
                } else {
                    scale[2] = (size[0] / aspect) / size[2];
                }
            }
        }
            break;
        case Z_AXIS: {
            if (aspect > 1.0) {
                if (size[0] > size[1]) {
                    scale[1] = (size[0] / aspect) / size[1];
                } else {
                    scale[0] = (size[1] * aspect) / size[0];
                }
            } else {
                if (size[1] > size[0]) {
                    scale[0] = (size[1] * aspect) / size[0];
                } else {
                    scale[1] = (size[0] / aspect) / size[1];
                }
            }
        }
            break;
        }
    }

    TRACE("%s dims %g,%g,%g", _dataSet->getName().c_str(),
          size[0], size[1], size[2]);
    TRACE("Setting scale to %g,%g,%g", scale[0], scale[1], scale[2]);
    setScale(scale);
}

vtkAlgorithmOutput *HeightMap::initWarp(vtkAlgorithmOutput *input)
{
    TRACE("Warp scale: %g", _warpScale);
    if (_warpScale == 0.0) {
        _warp = NULL;
        _normalsGenerator = NULL;
        return input;
    } else if (input == NULL) {
        ERROR("NULL input");
        return input;
    } else {
        if (_warp == NULL)
            _warp = vtkSmartPointer<vtkWarpScalar>::New();
        if (_normalsGenerator == NULL)
            _normalsGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
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
        _normalsGenerator->SetInputConnection(_warp->GetOutputPort());
        _normalsGenerator->SetFeatureAngle(90.);
        _normalsGenerator->AutoOrientNormalsOff();
        _normalsGenerator->ComputePointNormalsOn();
        return _normalsGenerator->GetOutputPort();
    }
}

vtkAlgorithmOutput *HeightMap::initWarp(vtkDataSet *dsInput)
{
    TRACE("Warp scale: %g", _warpScale);
    if (_warpScale == 0.0) {
        _warp = NULL;
        _normalsGenerator = NULL;
        return NULL;
    } else if (dsInput == NULL) {
        ERROR("NULL input");
        return NULL;
    } else {
        if (_warp == NULL)
            _warp = vtkSmartPointer<vtkWarpScalar>::New();
        if (_normalsGenerator == NULL)
            _normalsGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
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
#ifdef USE_VTK6
        _warp->SetInputData(dsInput);
#else
        _warp->SetInput(dsInput);
#endif
        _normalsGenerator->SetInputConnection(_warp->GetOutputPort());
        _normalsGenerator->SetFeatureAngle(90.);
        _normalsGenerator->AutoOrientNormalsOff();
        _normalsGenerator->ComputePointNormalsOn();
        return _normalsGenerator->GetOutputPort();
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
        vtkAlgorithmOutput *warpOutput = initWarp(_mapper->GetInputConnection(0, 0));
        _mapper->SetInputConnection(warpOutput);
        _contourFilter->SetInputConnection(warpOutput);
    } else if (scale == 0.0) {
        vtkAlgorithmOutput *warpInput = _warp->GetInputConnection(0, 0);
        _mapper->SetInputConnection(warpInput);
        _contourFilter->SetInputConnection(warpInput);
        _warp = NULL;
    } else {
        _warp->SetScaleFactor(_warpScale * _dataScale);
    }
#ifdef TRANSLATE_TO_ZORIGIN
    double pos[3];
    pos[0] = 0;
    pos[1] = 0;
    pos[2] = - _warpScale * _dataScale * _dataRange[0];
    setPosition(pos);
    TRACE("Z_POS: %g", pos[2]);
#endif
    if (_mapper != NULL)
        _mapper->Update();
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
        if (_splatter != NULL) {
            _splatter->GetSampleDimensions(dims);
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

    if (_mapper != NULL)
        _mapper->Update();
    if (_contourMapper != NULL)
        _contourMapper->Update();
}

void HeightMap::updateRanges(Renderer *renderer)
{
    TRACE("Enter");

    if (_dataSet == NULL) {
        ERROR("called before setDataSet");
        return;
    }

    if (renderer->getUseCumulativeRange()) {
        renderer->getCumulativeDataRange(_dataRange,
                                         _dataSet->getActiveScalarsName(),
                                         1);
        const char *activeVectors = _dataSet->getActiveVectorsName();
        if (activeVectors != NULL) {
            renderer->getCumulativeDataRange(_vectorMagnitudeRange,
                                             activeVectors,
                                             3);
            for (int i = 0; i < 3; i++) {
                renderer->getCumulativeDataRange(_vectorComponentRange[i],
                                                 activeVectors,
                                                 3, i);
            }
        }
    } else {
        _dataSet->getScalarRange(_dataRange);
        _dataSet->getVectorRange(_vectorMagnitudeRange);
        for (int i = 0; i < 3; i++) {
            _dataSet->getVectorRange(_vectorComponentRange[i], i);
        }
    }
 
    // Need to update color map ranges
    double *rangePtr = _colorFieldRange;
    if (_colorFieldRange[0] > _colorFieldRange[1]) {
        rangePtr = NULL;
    }
    setColorMode(_colorMode, _colorFieldType, _colorFieldName.c_str(), rangePtr);

    if ((_contours.empty() && _numContours > 0) ||
        _warpScale > 0.0) {
        // Contour isovalues and/or warp need to be recomputed
        update();
    }

    TRACE("Leave");
}

void HeightMap::setContourLineColorMapEnabled(bool mode)
{
    _contourColorMap = mode;

    double *rangePtr = _colorFieldRange;
    if (_colorFieldRange[0] > _colorFieldRange[1]) {
        rangePtr = NULL;
    }
    setColorMode(_colorMode, _colorFieldType, _colorFieldName.c_str(), rangePtr);
}

void HeightMap::setColorMode(ColorMode mode)
{
    _colorMode = mode;
    if (_dataSet == NULL)
        return;

    switch (mode) {
    case COLOR_BY_SCALAR:
        setColorMode(mode,
                     _dataSet->getActiveScalarsType(),
                     _dataSet->getActiveScalarsName(),
                     _dataRange);
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        setColorMode(mode,
                     _dataSet->getActiveVectorsType(),
                     _dataSet->getActiveVectorsName(),
                     _vectorMagnitudeRange);
        break;
    case COLOR_BY_VECTOR_X:
        setColorMode(mode,
                     _dataSet->getActiveVectorsType(),
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[0]);
        break;
    case COLOR_BY_VECTOR_Y:
        setColorMode(mode,
                     _dataSet->getActiveVectorsType(),
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[1]);
        break;
    case COLOR_BY_VECTOR_Z:
        setColorMode(mode,
                     _dataSet->getActiveVectorsType(),
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[2]);
        break;
    case COLOR_CONSTANT:
    default:
        setColorMode(mode, DataSet::POINT_DATA, NULL, NULL);
        break;
    }
}

void HeightMap::setColorMode(ColorMode mode,
                             const char *name, double range[2])
{
    if (_dataSet == NULL)
        return;
    DataSet::DataAttributeType type = DataSet::POINT_DATA;
    int numComponents = 1;
    if (name != NULL && strlen(name) > 0 &&
        !_dataSet->getFieldInfo(name, &type, &numComponents)) {
        ERROR("Field not found: %s", name);
        return;
    }
    setColorMode(mode, type, name, range);
}

void HeightMap::setColorMode(ColorMode mode, DataSet::DataAttributeType type,
                             const char *name, double range[2])
{
    _colorMode = mode;
    _colorFieldType = type;
    if (name == NULL)
        _colorFieldName.clear();
    else
        _colorFieldName = name;
    if (range == NULL) {
        _colorFieldRange[0] = DBL_MAX;
        _colorFieldRange[1] = -DBL_MAX;
    } else {
        memcpy(_colorFieldRange, range, sizeof(double)*2);
    }

    if (_dataSet == NULL || _mapper == NULL)
        return;

    switch (type) {
    case DataSet::POINT_DATA:
        _mapper->SetScalarModeToUsePointFieldData();
        _contourMapper->SetScalarModeToUsePointFieldData();
        break;
    case DataSet::CELL_DATA:
        _mapper->SetScalarModeToUseCellFieldData();
        _contourMapper->SetScalarModeToUseCellFieldData();
        break;
    default:
        ERROR("Unsupported DataAttributeType: %d", type);
        return;
    }

    if (_splatter != NULL) {
        if (name != NULL && strlen(name) > 0) {
            _splatter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, name);
        }
        _mapper->SelectColorArray("SplatterValues");
        _contourMapper->SelectColorArray("SplatterValues");
    } else if (name != NULL && strlen(name) > 0) {
        _mapper->SelectColorArray(name);
        _contourMapper->SelectColorArray(name);
    } else {
        _mapper->SetScalarModeToDefault();
        _contourMapper->SetScalarModeToDefault();
    }

    if (_lut != NULL) {
        if (range != NULL) {
            _lut->SetRange(range);
            TRACE("range %g, %g", range[0], range[1]);
        } else if (name != NULL && strlen(name) > 0) {
            double r[2];
            int comp = -1;
            if (mode == COLOR_BY_VECTOR_X)
                comp = 0;
            else if (mode == COLOR_BY_VECTOR_Y)
                comp = 1;
            else if (mode == COLOR_BY_VECTOR_Z)
                comp = 2;

            if (_renderer->getUseCumulativeRange()) {
                int numComponents;
                if  (!_dataSet->getFieldInfo(name, type, &numComponents)) {
                    ERROR("Field not found: %s, type: %d", name, type);
                    return;
                } else if (numComponents < comp+1) {
                    ERROR("Request for component %d in field with %d components",
                          comp, numComponents);
                    return;
                }
                _renderer->getCumulativeDataRange(r, name, type, numComponents, comp);
            } else {
                _dataSet->getDataRange(r, name, type, comp);
            }
            _lut->SetRange(r);
            TRACE("%s range %g, %g", name, r[0], r[1]);
        }
        _lut->Modified();
    }

    if (_contourColorMap) {
        _contourMapper->ScalarVisibilityOn();
    } else {
        _contourMapper->ScalarVisibilityOff();
    }

    switch (mode) {
    case COLOR_BY_SCALAR:
        _mapper->ScalarVisibilityOn();
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        _mapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToMagnitude();
        }
        break;
    case COLOR_BY_VECTOR_X:
        _mapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        _mapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        _mapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    case COLOR_CONSTANT:
    default:
        _mapper->ScalarVisibilityOff();
        break;
    }
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
        if (_mapper != NULL) {
            _mapper->UseLookupTableScalarRangeOn();
            _mapper->SetLookupTable(_lut);
        }
        if (_contourMapper != NULL) {
            _contourMapper->UseLookupTableScalarRangeOn();
            _contourMapper->SetLookupTable(_lut);
        }
        _lut->DeepCopy(cmap->getLookupTable());
        switch (_colorMode) {
        case COLOR_CONSTANT:
        case COLOR_BY_SCALAR:
            _lut->SetRange(_dataRange);
            break;
        case COLOR_BY_VECTOR_MAGNITUDE:
            _lut->SetRange(_vectorMagnitudeRange);
            break;
        case COLOR_BY_VECTOR_X:
            _lut->SetRange(_vectorComponentRange[0]);
            break;
        case COLOR_BY_VECTOR_Y:
            _lut->SetRange(_vectorComponentRange[1]);
            break;
        case COLOR_BY_VECTOR_Z:
            _lut->SetRange(_vectorComponentRange[2]);
            break;
        default:
            break;
        }
    } else {
        double range[2];
        _lut->GetTableRange(range);
        _lut->DeepCopy(cmap->getLookupTable());
        _lut->SetRange(range);
        _lut->Modified();
    }

    switch (_colorMode) {
    case COLOR_BY_VECTOR_MAGNITUDE:
        _lut->SetVectorModeToMagnitude();
        break;
    case COLOR_BY_VECTOR_X:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(0);
        break;
    case COLOR_BY_VECTOR_Y:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(1);
        break;
    case COLOR_BY_VECTOR_Z:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(2);
        break;
    default:
         break;
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
    _lighting = state;
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
 * \brief Set RGB color of mesh
 */
void HeightMap::setColor(float color[3])
{
    _color[0] = color[0];
    _color[1] = color[1];
    _color[2] = color[2];
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetColor(_color[0], _color[1], _color[2]);
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
    if (_mapper != NULL) {
        _mapper->SetClippingPlanes(planes);
    }
    if (_contourMapper != NULL) {
        _contourMapper->SetClippingPlanes(planes);
    }
}

