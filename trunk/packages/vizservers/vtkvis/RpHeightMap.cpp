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
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkProbeFilter.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkContourFilter.h>
#include <vtkWarpScalar.h>
#include <vtkPropAssembly.h>

#include "RpHeightMap.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

#define MESH_POINT_CLOUDS

HeightMap::HeightMap() :
    _dataSet(NULL),
    _numContours(0),
    _edgeWidth(1.0),
    _contourEdgeWidth(1.0),
    _opacity(1.0),
    _warpScale(1.0),
    _sliceAxis(Z_AXIS),
    _pipelineInitialized(false)
{
    _dataRange[0] = 0.0;
    _dataRange[1] = 1.0;
    _edgeColor[0] = 0.0;
    _edgeColor[1] = 0.0;
    _edgeColor[2] = 0.0;
    _contourEdgeColor[0] = 1.0;
    _contourEdgeColor[1] = 0.0;
    _contourEdgeColor[2] = 0.0;
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

/**
 * \brief Specify input DataSet with scalars to colormap
 *
 * Currently the DataSet must be image data (2D uniform grid)
 */
void HeightMap::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        if (_dataSet != NULL) {
            double dataRange[2];
            _dataSet->getDataRange(dataRange);
            _dataRange[0] = dataRange[0];
            _dataRange[1] = dataRange[1];

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
 * \brief Returns the DataSet this HeightMap renders
 */
DataSet *HeightMap::getDataSet()
{
    return _dataSet;
}

/**
 * \brief Internal method to set up color mapper after a state change
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
                if (_dataSet->is2D()) {
#ifdef MESH_POINT_CLOUDS
                    // Result of Delaunay2D is a PolyData
                    vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                    mesher->SetInput(pd);
                    vtkAlgorithmOutput *warpOutput = initWarp(mesher->GetOutputPort());
                    _dsMapper->SetInputConnection(warpOutput);
                    _contourFilter->SetInputConnection(warpOutput);
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
                    if (_probeFilter == NULL)
                        _probeFilter = vtkSmartPointer<vtkProbeFilter>::New();
                    _probeFilter->SetSourceConnection(mesher->GetOutputPort());
                    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
                    int xdim = 256;
                    int ydim = 256;
                    int zdim = 1;
                    imageData->SetDimensions(xdim, ydim, zdim);
                    imageData->SetOrigin(bounds[0], bounds[2], bounds[4] + (bounds[5]-bounds[4])/2.);
                    imageData->SetSpacing((bounds[1]-bounds[0])/((double)(xdim-1)), 
                                          (bounds[3]-bounds[2])/((double)(ydim-1)),
                                          0);
                    _probeFilter->SetInput(imageData);
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(_probeFilter->GetOutputPort());
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
            } else if (imageData == NULL) {
                // structured grid, unstructured grid, or rectilinear grid
                double bounds[6];
                ds->GetBounds(bounds);
                // Sample a plane within the grid bounding box
                if (_probeFilter == NULL)
                    _probeFilter = vtkSmartPointer<vtkProbeFilter>::New();
                _probeFilter->SetSource(ds);
                vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
                int xdim = 256;
                int ydim = 256;
                int zdim = 1;
                imageData->SetDimensions(xdim, ydim, zdim);
                imageData->SetOrigin(bounds[0], bounds[2], bounds[4] + (bounds[5]-bounds[4])/2.);
                imageData->SetSpacing((bounds[1]-bounds[0])/((double)(xdim-1)), 
                                      (bounds[3]-bounds[2])/((double)(ydim-1)),
                                      0);
                _probeFilter->SetInput(imageData);
                gf->SetInputConnection(_probeFilter->GetOutputPort());
            } else {
                // 2D image data
                gf->SetInput(ds);
            }
            vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
            _dsMapper->SetInputConnection(warpOutput);
            _contourFilter->SetInputConnection(warpOutput);
        }
    }

    _pipelineInitialized = true;

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        ERROR("No scalar point data in dataset %s", _dataSet->getName().c_str());
        if (_lut == NULL) {
            _lut = vtkSmartPointer<vtkLookupTable>::New();
        }
    } else {
        vtkLookupTable *lut = ds->GetPointData()->GetScalars()->GetLookupTable();
        TRACE("Data set scalars lookup table: %p\n", lut);
        if (_lut == NULL) {
            if (lut)
                _lut = lut;
            else {
                _lut = vtkSmartPointer<vtkLookupTable>::New();
            }
        }
    }

    _lut->SetRange(_dataRange);

    initProp();

    _dsMapper->SetColorModeToMapScalars();
    _dsMapper->UseLookupTableScalarRangeOn();
    _dsMapper->SetLookupTable(_lut);
    //_dsMapper->InterpolateScalarsBeforeMappingOn();

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
        _contourMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _contourMapper->SetInputConnection(_contourFilter->GetOutputPort());
        _contourActor->SetMapper(_contourMapper);
    }

    _dsActor->SetMapper(_dsMapper);

    if (_props == NULL) {
        _props = vtkSmartPointer<vtkPropAssembly>::New();
        _props->AddPart(_dsActor);
        _props->AddPart(_contourActor);
    }

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

void HeightMap::selectVolumeSlice(Axis axis, double ratio)
{
    if (_dataSet->is2D()) {
        WARN("DataSet not 3D, returning");
        return;
    }

    if (_volumeSlicer == NULL &&
         _probeFilter == NULL) {
        WARN("Called before update() or DataSet is not a volume");
        return;
    }

    int dims[3];

    if (_pointSplatter != NULL) {
        _pointSplatter->GetSampleDimensions(dims);
    } else {
        vtkImageData *imageData = vtkImageData::SafeDownCast(_dataSet->getVtkDataSet());
        if (imageData == NULL) {
            if (_probeFilter != NULL) {
                imageData = vtkImageData::SafeDownCast(_probeFilter->GetInput());
                if (imageData == NULL) {
                    ERROR("Couldn't get probe filter input image");
                    return;
                }
            } else {
                ERROR("Not a volume data set");
                return;
            }
        }
        imageData->GetDimensions(dims);
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

    if (_probeFilter != NULL) {
        vtkImageData *imageData = vtkImageData::SafeDownCast(_probeFilter->GetInput());
        double bounds[6];
        assert(vtkDataSet::SafeDownCast(_probeFilter->GetSource()) != NULL);
        vtkDataSet::SafeDownCast(_probeFilter->GetSource())->GetBounds(bounds);
        int dim = 256;

        switch (axis) {
        case X_AXIS:
            imageData->SetDimensions(1, dim, dim);
            imageData->SetOrigin(bounds[0] + (bounds[1]-bounds[0])*ratio, bounds[2], bounds[4]);
            imageData->SetSpacing(0,
                                  (bounds[3]-bounds[2])/((double)(dim-1)), 
                                  (bounds[5]-bounds[4])/((double)(dim-1)));
            break;
        case Y_AXIS:
            imageData->SetDimensions(dim, 1, dim);
            imageData->SetOrigin(bounds[0], bounds[2] + (bounds[3]-bounds[2])*ratio, bounds[4]);
            imageData->SetSpacing((bounds[1]-bounds[0])/((double)(dim-1)), 
                                  0,
                                  (bounds[5]-bounds[4])/((double)(dim-1)));
            break;
        case Z_AXIS:
            imageData->SetDimensions(dim, dim, 1);
            imageData->SetOrigin(bounds[0], bounds[2], bounds[4] + (bounds[5]-bounds[4])*ratio);
            imageData->SetSpacing((bounds[1]-bounds[0])/((double)(dim-1)), 
                                  (bounds[3]-bounds[2])/((double)(dim-1)),
                                  0);
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }
    } else {
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
            voi[2] = voi[3] = (int)((dims[1]-1) * ratio);
            voi[0] = 0;
            voi[1] = dims[0]-1;
            voi[4] = 0;
            voi[5] = dims[2]-1;
            break;
        case Z_AXIS:
            voi[4] = voi[5] = (int)((dims[2]-1) * ratio);
            voi[0] = 0;
            voi[1] = dims[0]-1;
            voi[2] = 0;
            voi[3] = dims[1]-1;
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }

        _volumeSlicer->SetVOI(voi[0], voi[1], voi[2], voi[3], voi[4], voi[5]);
    }

    if (_dsMapper != NULL)
        _dsMapper->Update();
    if (_contourMapper != NULL)
        _contourMapper->Update();
}

/**
 * \brief Get the VTK Prop for the colormapped dataset
 */
vtkProp *HeightMap::getProp()
{
    return _props;
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
        _contourActor->GetProperty()->SetEdgeColor(_contourEdgeColor[0],
                                                   _contourEdgeColor[1],
                                                   _contourEdgeColor[2]);
        _contourActor->GetProperty()->SetLineWidth(_contourEdgeWidth);
        _contourActor->GetProperty()->EdgeVisibilityOn();
        _contourActor->GetProperty()->SetAmbient(.2);
        _contourActor->GetProperty()->LightingOff();
    }
}

/**
 * \brief Get the VTK colormap lookup table in use
 */
vtkLookupTable *HeightMap::getLookupTable()
{ 
    return _lut;
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void HeightMap::setLookupTable(vtkLookupTable *lut)
{
    if (lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
    } else {
        _lut = lut;
    }

    if (_dsMapper != NULL) {
        _dsMapper->UseLookupTableScalarRangeOn();
        _dsMapper->SetLookupTable(_lut);
    }
}

/**
 * \brief Specify number of evenly spaced contour lines to render
 *
 * Will override any existing contours
 */
void  HeightMap::setContours(int numContours)
{
    _contours.clear();
    _numContours = numContours;

    if (_dataSet != NULL) {
        double dataRange[2];
        _dataSet->getDataRange(dataRange);
        _dataRange[0] = dataRange[0];
        _dataRange[1] = dataRange[1];
    }

    update();
}

/**
 * \brief Specify number of evenly spaced contour lines to render
 * between the given range (including range endpoints)
 *
 * Will override any existing contours
 */
void  HeightMap::setContours(int numContours, double range[2])
{
    _contours.clear();
    _numContours = numContours;

    _dataRange[0] = range[0];
    _dataRange[1] = range[1];

    update();
}

/**
 * \brief Specify an explicit list of contour isovalues
 *
 * Will override any existing contours
 */
void  HeightMap::setContourList(const std::vector<double>& contours)
{
    _contours = contours;
    _numContours = (int)_contours.size();

    update();
}

/**
 * \brief Get the number of contours
 */
int  HeightMap::getNumContours() const
{
    return _numContours;
}

/**
 * \brief Get the contour list (may be empty if number of contours
 * was specified in place of a list)
 */
const std::vector<double>&  HeightMap::getContourList() const
{
    return _contours;
}

/**
 * \brief Turn on/off rendering of this colormapped dataset
 */
void HeightMap::setVisibility(bool state)
{
    if (_dsActor != NULL) {
        _dsActor->SetVisibility((state ? 1 : 0));
    }
    if (_contourActor != NULL) {
        _contourActor->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Get visibility state of the colormapped dataset
 * 
 * \return Is HeightMap visible?
 */
bool HeightMap::getVisibility()
{
    if (_dsActor != NULL &&
        _dsActor->GetVisibility() != 0) {
        return true;
    } else if (_contourActor != NULL &&
               _contourActor->GetVisibility() != 0) {
        return true;
    }
    return false;
}

/**
 * \brief Set opacity used to render the colormapped dataset
 */
void HeightMap::setOpacity(double opacity)
{
    _opacity = opacity;
    if (_dsActor != NULL) {
        _dsActor->GetProperty()->SetOpacity(opacity);
    }
    if (_contourActor != NULL) {
        _contourActor->GetProperty()->SetOpacity(opacity);
    }
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
 * \brief Turn on/off rendering of contour isolines
 */
void HeightMap::setContourVisibility(bool state)
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
    if (_contourActor != NULL)
        _contourActor->GetProperty()->SetEdgeColor(_contourEdgeColor[0],
                                                   _contourEdgeColor[1],
                                                   _contourEdgeColor[2]);
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

/**
 * \brief Turn on/off lighting of this object
 */
void HeightMap::setLighting(bool state)
{
    if (_dsActor != NULL)
        _dsActor->GetProperty()->SetLighting((state ? 1 : 0));
}
