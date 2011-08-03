/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdlib>
#include <ctime>
#include <cfloat>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkRegularPolygonSource.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkPolyData.h>
#include <vtkTubeFilter.h>
#include <vtkRibbonFilter.h>

#include "RpStreamlines.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Streamlines::Streamlines() :
    VtkGraphicsObject(),
    _lineType(LINES),
    _seedVisible(true)
{
    _seedColor[0] = 1.0f;
    _seedColor[1] = 1.0f;
    _seedColor[2] = 1.0f;
}

Streamlines::~Streamlines()
{
}

/**
 * \brief Create and initialize a VTK Prop to render Streamlines
 */
void Streamlines::initProp()
{
    if (_linesActor == NULL) {
        _linesActor = vtkSmartPointer<vtkActor>::New();
        _linesActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _linesActor->GetProperty()->SetLineWidth(_edgeWidth);
        _linesActor->GetProperty()->SetOpacity(_opacity);
        _linesActor->GetProperty()->SetAmbient(.2);
        if (!_lighting)
            _linesActor->GetProperty()->LightingOff();
        switch (_lineType) {
        case LINES:
            _linesActor->GetProperty()->BackfaceCullingOff();
            _linesActor->GetProperty()->SetRepresentationToWireframe();
            _linesActor->GetProperty()->EdgeVisibilityOff();
            break;
        case TUBES:
            _linesActor->GetProperty()->BackfaceCullingOn();
            _linesActor->GetProperty()->SetRepresentationToSurface();
            _linesActor->GetProperty()->EdgeVisibilityOff();
            break;
        case RIBBONS:
            _linesActor->GetProperty()->BackfaceCullingOff();
            _linesActor->GetProperty()->SetRepresentationToSurface();
            _linesActor->GetProperty()->EdgeVisibilityOff();
            break;
        default:
            ;
        }
    }
    if (_seedActor == NULL) {
        _seedActor = vtkSmartPointer<vtkActor>::New();
        _seedActor->GetProperty()->SetColor(_seedColor[0], _seedColor[1], _seedColor[2]);
        _seedActor->GetProperty()->SetLineWidth(1);
        _seedActor->GetProperty()->SetPointSize(2);
        _seedActor->GetProperty()->SetOpacity(_opacity);
        _seedActor->GetProperty()->SetRepresentationToWireframe();
        _seedActor->GetProperty()->LightingOff();
    }
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkAssembly>::New();
        getAssembly()->AddPart(_linesActor);
        getAssembly()->AddPart(_seedActor);
    }
}

void Streamlines::getRandomPoint(double pt[3], const double bounds[6])
{
    int r = rand();
    pt[0] = bounds[0] + ((double)r / RAND_MAX) * (bounds[1] - bounds[0]);
    r = rand();
    pt[1] = bounds[2] + ((double)r / RAND_MAX) * (bounds[3] - bounds[2]);
    r = rand();
    pt[2] = bounds[4] + ((double)r / RAND_MAX) * (bounds[5] - bounds[4]);
}

void Streamlines::getRandomCellPt(vtkDataSet *ds, double pt[3])
{
    int numCells = (int)ds->GetNumberOfCells();
    int cell = rand() % numCells;
    double bounds[6];
    ds->GetCellBounds(cell, bounds);
    int r = rand();
    pt[0] = bounds[0] + ((double)r / RAND_MAX) * (bounds[1] - bounds[0]);
    r = rand();
    pt[1] = bounds[2] + ((double)r / RAND_MAX) * (bounds[3] - bounds[2]);
    r = rand();
    pt[2] = bounds[4] + ((double)r / RAND_MAX) * (bounds[5] - bounds[4]);
}

/**
 * \brief Internal method to set up pipeline after a state change
 */
void Streamlines::update()
{
    if (_dataSet == NULL) {
        return;
    }

    vtkDataSet *ds = _dataSet->getVtkDataSet();
    double dataRange[2];
    _dataSet->getDataRange(dataRange);
    double bounds[6];
    _dataSet->getBounds(bounds);
    double maxBound = 0.0;
    if (bounds[1] - bounds[0] > maxBound) {
        maxBound = bounds[1] - bounds[0];
    }
    if (bounds[3] - bounds[2] > maxBound) {
        maxBound = bounds[3] - bounds[2];
    }
    if (bounds[5] - bounds[4] > maxBound) {
        maxBound = bounds[5] - bounds[4];
    }

    vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetVectors() == NULL) {
        WARN("No vector point data found in DataSet %s", _dataSet->getName().c_str());
        if (ds->GetCellData() == NULL ||
            ds->GetCellData()->GetVectors() == NULL) {
            ERROR("No vectors found in DataSet %s", _dataSet->getName().c_str());
        } else {
            cellToPtData = 
                vtkSmartPointer<vtkCellDataToPointData>::New();
            cellToPtData->SetInput(ds);
            //cellToPtData->PassCellDataOn();
            cellToPtData->Update();
            ds = cellToPtData->GetOutput();
        }
    }

    if (_streamTracer == NULL) {
        _streamTracer = vtkSmartPointer<vtkStreamTracer>::New();
    }

    _streamTracer->SetInput(ds);

    // Set up seed source object
    vtkSmartPointer<vtkPolyData> seed = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    int numPoints = 200;
    srand((unsigned int)time(NULL));
    for (int i = 0; i < numPoints; i++) {
        double pt[3];
        getRandomCellPt(ds, pt);
        TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
        pts->InsertNextPoint(pt);
        cells->InsertNextCell(1);
        cells->InsertCellPoint(i);
    }

    seed->SetPoints(pts);
    seed->SetVerts(cells);

    TRACE("Seed points: %d", seed->GetNumberOfPoints());

    _streamTracer->SetMaximumPropagation(maxBound);
    _streamTracer->SetSource(seed);

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOn();
        // _pdMapper->ScalarVisibilityOff();
    }
    if (_seedMapper == NULL) {
        _seedMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _seedMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _seedMapper->SetInput(seed);
    }

    switch (_lineType) {
    case LINES: {
        _streamTracer->SetComputeVorticity(false);
        _pdMapper->SetInputConnection(_streamTracer->GetOutputPort());
    }
        break;
    case TUBES: {
        _streamTracer->SetComputeVorticity(true);
        _lineFilter = vtkSmartPointer<vtkTubeFilter>::New();
        vtkTubeFilter *tubeFilter = vtkTubeFilter::SafeDownCast(_lineFilter);
        tubeFilter->SetNumberOfSides(5);
        _lineFilter->SetInputConnection(_streamTracer->GetOutputPort());
        _pdMapper->SetInputConnection(_lineFilter->GetOutputPort());
    }
        break;
    case RIBBONS: {
        _streamTracer->SetComputeVorticity(true);
        _lineFilter = vtkSmartPointer<vtkRibbonFilter>::New();
        _lineFilter->SetInputConnection(_streamTracer->GetOutputPort());
        _pdMapper->SetInputConnection(_lineFilter->GetOutputPort());
    }
        break;
    default:
        ERROR("Unknown LineType: %d", _lineType);
    }

#if defined(DEBUG) && defined(WANT_TRACE)
    _streamTracer->Update();
    vtkPolyData *pd = _streamTracer->GetOutput();
    TRACE("Verts: %d Lines: %d Polys: %d Strips: %d",
                  pd->GetNumberOfVerts(),
                  pd->GetNumberOfLines(),
                  pd->GetNumberOfPolys(),
                  pd->GetNumberOfStrips());
#endif

    initProp();

    _seedActor->SetMapper(_seedMapper);

    _lut = vtkSmartPointer<vtkLookupTable>::New();
    _lut->SetVectorModeToMagnitude();

    _pdMapper->SetScalarModeToUsePointFieldData();
    if (ds->GetPointData() != NULL &&
        ds->GetPointData()->GetVectors() != NULL) {
        TRACE("Vector name: '%s'", ds->GetPointData()->GetVectors()->GetName());
        _pdMapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
    }
    _pdMapper->SetColorModeToMapScalars();
    _pdMapper->UseLookupTableScalarRangeOff();
    _pdMapper->SetLookupTable(_lut);

    _linesActor->SetMapper(_pdMapper);
    _pdMapper->Update();
    _seedMapper->Update();
}

/**
 * \brief Use randomly distributed seed points
 *
 * \param[in] numPoints Number of random seed points to generate
 */
void Streamlines::setSeedToRandomPoints(int numPoints)
{
    if (_streamTracer != NULL) {
        // Set up seed source object
        vtkSmartPointer<vtkPolyData> seed = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

        srand((unsigned int)time(NULL));
        for (int i = 0; i < numPoints; i++) {
            double pt[3];
            getRandomCellPt(_dataSet->getVtkDataSet(), pt);
            TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
            pts->InsertNextPoint(pt);
            cells->InsertNextCell(1);
            cells->InsertCellPoint(i);
        }

        seed->SetPoints(pts);
        seed->SetVerts(cells);

        TRACE("Seed points: %d", seed->GetNumberOfPoints());
        vtkSmartPointer<vtkDataSet> oldSeed;
        if (_streamTracer->GetSource() != NULL) {
            oldSeed = _streamTracer->GetSource();
        }

        _streamTracer->SetSource(seed);
        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }

        _seedMapper->SetInput(seed);
    }
}

/**
 * \brief Use seed points along a line
 *
 * \param[in] start Starting point of rake line
 * \param[in] end End point of rake line
 * \param[in] numPoints Number of points along line to generate
 */
void Streamlines::setSeedToRake(double start[3], double end[3], int numPoints)
{
    if (numPoints < 2)
        return;
    if (_streamTracer != NULL) {
        // Set up seed source object
        vtkSmartPointer<vtkPolyData> seed = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        vtkSmartPointer<vtkPolyLine> polyline = vtkSmartPointer<vtkPolyLine>::New();

        double dir[3];
        for (int i = 0; i < 3; i++) {
            dir[i] = end[i] - start[i];
        }

        polyline->GetPointIds()->SetNumberOfIds(numPoints);
        for (int i = 0; i < numPoints; i++) {
            double pt[3];
            for (int ii = 0; ii < 3; ii++) {
                pt[ii] = start[ii] + dir[ii] * ((double)i / (numPoints-1));
            }
            TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
            pts->InsertNextPoint(pt);
            polyline->GetPointIds()->SetId(i, i);
        }

        cells->InsertNextCell(polyline);
        seed->SetPoints(pts);
        seed->SetLines(cells);

        TRACE("Seed points: %d", seed->GetNumberOfPoints());
        vtkSmartPointer<vtkDataSet> oldSeed;
        if (_streamTracer->GetSource() != NULL) {
            oldSeed = _streamTracer->GetSource();
        }

        _streamTracer->SetSource(seed);
        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }

        _seedMapper->SetInput(seed);
    }
}

/**
 * \brief Use seed points from an n-sided polygon
 *
 * \param[in] center Center point of polygon
 * \param[in] normal Normal vector to orient polygon
 * \param[in] radius Radius of circumscribing circle
 * \param[in] numSides Number of polygon sides (and points) to generate
 */
void Streamlines::setSeedToPolygon(double center[3],
                                   double normal[3],
                                   double radius,
                                   int numSides)
{
    if (_streamTracer != NULL) {
        // Set up seed source object
        vtkSmartPointer<vtkRegularPolygonSource> seed = vtkSmartPointer<vtkRegularPolygonSource>::New();

        seed->SetCenter(center);
        seed->SetNormal(normal);
        seed->SetRadius(radius);
        seed->SetNumberOfSides(numSides);
        seed->GeneratePolygonOn();

        TRACE("Seed points: %d", numSides);
        vtkSmartPointer<vtkDataSet> oldSeed;
        if (_streamTracer->GetSource() != NULL) {
            oldSeed = _streamTracer->GetSource();
        }

        _streamTracer->SetSourceConnection(seed->GetOutputPort());
        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }

        _seedMapper->SetInputConnection(seed->GetOutputPort());
    }
}

/**
 * \brief Set maximum length of stream lines in world coordinates
 */
void Streamlines::setMaxPropagation(double length)
{
    if (_streamTracer != NULL) {
        _streamTracer->SetMaximumPropagation(length);
    }
}

/**
 * \brief Set streamline type to polylines
 */
void Streamlines::setLineTypeToLines()
{
    _lineType = LINES;
    if (_streamTracer != NULL &&
        _pdMapper != NULL) {
        _streamTracer->SetComputeVorticity(false);
        _pdMapper->SetInputConnection(_streamTracer->GetOutputPort());
        _lineFilter = NULL;
        _linesActor->GetProperty()->BackfaceCullingOff();
        _linesActor->GetProperty()->SetRepresentationToWireframe();
        _linesActor->GetProperty()->LightingOff();
    }
}

/**
 * \brief Set streamline type to 3D tubes
 *
 * \param[in] numSides Number of sides (>=3) for tubes
 * \param[in] radius World coordinate minimum tube radius
 */
void Streamlines::setLineTypeToTubes(int numSides, double radius)
{
    _lineType = TUBES;
    if (_streamTracer != NULL) {
        _streamTracer->SetComputeVorticity(true);
        if (vtkTubeFilter::SafeDownCast(_lineFilter) == NULL) {
            _lineFilter = vtkSmartPointer<vtkTubeFilter>::New();
            _lineFilter->SetInputConnection(_streamTracer->GetOutputPort());
        }
        vtkTubeFilter *tubeFilter = vtkTubeFilter::SafeDownCast(_lineFilter);
        if (numSides < 3)
            numSides = 3;
        tubeFilter->SetNumberOfSides(numSides);
        tubeFilter->SetRadius(radius);
        _pdMapper->SetInputConnection(_lineFilter->GetOutputPort());
        _linesActor->GetProperty()->BackfaceCullingOn();
        _linesActor->GetProperty()->SetRepresentationToSurface();
        _linesActor->GetProperty()->LightingOn();
     }
}

/**
 * \brief Set streamline type to 3D ribbons
 *
 * \param[in] width Minimum half-width of ribbons
 * \param[in] angle Default ribbon angle in degrees from normal
 */
void Streamlines::setLineTypeToRibbons(double width, double angle)
{
    _lineType = RIBBONS;
    if (_streamTracer != NULL) {
        _streamTracer->SetComputeVorticity(true);
        if (vtkRibbonFilter::SafeDownCast(_lineFilter) == NULL) {
            _lineFilter = vtkSmartPointer<vtkRibbonFilter>::New();
            _lineFilter->SetInputConnection(_streamTracer->GetOutputPort());
        }
        vtkRibbonFilter *ribbonFilter = vtkRibbonFilter::SafeDownCast(_lineFilter);
        ribbonFilter->SetWidth(width);
        ribbonFilter->SetAngle(angle);
        ribbonFilter->UseDefaultNormalOn();
        _pdMapper->SetInputConnection(_lineFilter->GetOutputPort());
        _linesActor->GetProperty()->BackfaceCullingOff();
        _linesActor->GetProperty()->SetRepresentationToSurface();
        _linesActor->GetProperty()->LightingOn();
    }
}

/**
 * \brief Get the VTK colormap lookup table in use
 */
vtkLookupTable *Streamlines::getLookupTable()
{ 
    return _lut;
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Streamlines::setLookupTable(vtkLookupTable *lut)
{
    if (lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
    } else {
        _lut = lut;
    }

    if (_pdMapper != NULL) {
        _pdMapper->SetLookupTable(_lut);
    }
}

/**
 * \brief Turn on/off lighting of this object
 */
void Streamlines::setLighting(bool state)
{
    _lighting = state;
    if (_linesActor != NULL)
        _linesActor->GetProperty()->SetLighting((state ? 1 : 0));
}

/**
 * \brief Turn on/off rendering of this Streamlines
 */
void Streamlines::setVisibility(bool state)
{
    if (_linesActor != NULL) {
        _linesActor->SetVisibility((state ? 1 : 0));
    }
    if (_seedActor != NULL) {
        if (!state ||
            (state && _seedVisible)) {
            _seedActor->SetVisibility((state ? 1 : 0));
        }
    }
}

/**
 * \brief Turn on/off rendering of the seed geometry
 */
void Streamlines::setSeedVisibility(bool state)
{
    _seedVisible = state;
    if (_seedActor != NULL) {
        _seedActor->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Get visibility state of the Streamlines
 * 
 * \return Are the Streamlines visible?
 */
bool Streamlines::getVisibility()
{
    if (_linesActor == NULL) {
        return false;
    } else {
        return (_linesActor->GetVisibility() != 0);
    }
}

/**
 * \brief Turn on/off rendering of edges
 */
void Streamlines::setEdgeVisibility(bool state)
{
    if (_linesActor != NULL) {
        _linesActor->GetProperty()->SetEdgeVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Set RGB color of stream lines
 */
void Streamlines::setEdgeColor(float color[3])
{
    _edgeColor[0] = color[0];
    _edgeColor[1] = color[1];
    _edgeColor[2] = color[2];
    if (_linesActor != NULL)
        _linesActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
}

/**
 * \brief Set RGB color of seed geometry
 */
void Streamlines::setSeedColor(float color[3])
{
    _seedColor[0] = color[0];
    _seedColor[1] = color[1];
    _seedColor[2] = color[2];
    if (_seedActor != NULL)
        _seedActor->GetProperty()->SetColor(_seedColor[0], _seedColor[1], _seedColor[2]);
}

/**
 * \brief Set pixel width of stream lines (may be a no-op)
 */
void Streamlines::setEdgeWidth(float edgeWidth)
{
    _edgeWidth = edgeWidth;
    if (_linesActor != NULL)
        _linesActor->GetProperty()->SetLineWidth(_edgeWidth);
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Streamlines::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_pdMapper != NULL) {
        _pdMapper->SetClippingPlanes(planes);
    }
    if (_seedMapper != NULL) {
        _seedMapper->SetClippingPlanes(planes);
    }
}
