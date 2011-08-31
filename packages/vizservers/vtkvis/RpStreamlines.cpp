/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdlib>
#include <ctime>
#include <cfloat>
#include <cmath>

#include <vtkMath.h>
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
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "RpStreamlines.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Streamlines::Streamlines() :
    VtkGraphicsObject(),
    _lineType(LINES),
    _colorMode(COLOR_BY_VECTOR_MAGNITUDE),
    _colorMap(NULL),
    _seedVisible(true),
    _dataScale(1)
{
    _faceCulling = true;
    _color[0] = 1.0f;
    _color[1] = 1.0f;
    _color[2] = 1.0f;
    _seedColor[0] = 1.0f;
    _seedColor[1] = 1.0f;
    _seedColor[2] = 1.0f;
    vtkMath::RandomSeed((int)time(NULL));
    srand((unsigned int)time(NULL));
}

Streamlines::~Streamlines()
{
}

void Streamlines::setDataSet(DataSet *dataSet,
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
 * \brief Create and initialize a VTK Prop to render Streamlines
 */
void Streamlines::initProp()
{
    if (_linesActor == NULL) {
        _linesActor = vtkSmartPointer<vtkActor>::New();
        _linesActor->GetProperty()->SetColor(_color[0], _color[1], _color[2]);
        _linesActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _linesActor->GetProperty()->SetLineWidth(_edgeWidth);
        _linesActor->GetProperty()->SetOpacity(_opacity);
        _linesActor->GetProperty()->SetAmbient(.2);
        if (!_lighting)
            _linesActor->GetProperty()->LightingOff();
        switch (_lineType) {
        case LINES:
            setCulling(_linesActor->GetProperty(), false);
            _linesActor->GetProperty()->SetRepresentationToWireframe();
            _linesActor->GetProperty()->EdgeVisibilityOff();
            break;
        case TUBES:
            if (_faceCulling && _opacity == 1.0)
                setCulling(_linesActor->GetProperty(), true);
            _linesActor->GetProperty()->SetRepresentationToSurface();
            _linesActor->GetProperty()->EdgeVisibilityOff();
            break;
        case RIBBONS:
            setCulling(_linesActor->GetProperty(), false);
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

/**
 * \brief Get a pseudo-random number in range [min,max]
 */
double Streamlines::getRandomNum(double min, double max)
{
#if 1
    return vtkMath::Random(min, max);
#else
    int r = rand();
    return (min + ((double)r / RAND_MAX) * (max - min));
#endif
}

/**
 * \brief Get a random 3D point within an AABB
 *
 * \param[out] pt The random point
 * \param[in] bounds The bounds of the AABB
 */
void Streamlines::getRandomPoint(double pt[3], const double bounds[6])
{
    pt[0] = getRandomNum(bounds[0], bounds[1]);
    pt[1] = getRandomNum(bounds[2], bounds[3]);
    pt[2] = getRandomNum(bounds[4], bounds[5]);
}

/**
 * \brief Get a random point within a triangle (including edges)
 *
 * \param[out] pt The random point
 * \param[in] v1 Triangle vertex 1
 * \param[in] v2 Triangle vertex 2
 * \param[in] v3 Triangle vertex 3
 */
void Streamlines::getRandomPointInTriangle(double pt[3],
                                           const double v1[3],
                                           const double v2[3],
                                           const double v3[3])
{
    // Choose random barycentric coordinates
    double bary[3];
    bary[0] = getRandomNum(0, 1);
    bary[1] = getRandomNum(0, 1);
    if (bary[0] + bary[1] > 1.0) {
        bary[0] = 1.0 - bary[0];
        bary[1] = 1.0 - bary[1];
    }
    bary[2] = 1.0 - bary[0] - bary[1];

    TRACE("bary %g %g %g", bary[0], bary[1], bary[2]);
    // Convert to cartesian coords
    for (int i = 0; i < 3; i++) {
        pt[i] = v1[i] * bary[0] + v2[i] * bary[1] + v3[i] * bary[2];
    }
}

/**
 * \brief Get a random point on a line segment (including endpoints)
 */
void Streamlines::getRandomPointOnLineSegment(double pt[3],
                                              const double endpt[3],
                                              const double endpt2[3])
{
    double ratio = getRandomNum(0, 1);
    pt[0] = endpt[0] + ratio * (endpt2[0] - endpt[0]);
    pt[1] = endpt[1] + ratio * (endpt2[1] - endpt[1]);
    pt[2] = endpt[2] + ratio * (endpt2[2] - endpt[2]);
}

/**
 * \brief Get a random point within a vtkDataSet's mesh
 *
 * Note: This currently doesn't give a uniform distribution of
 * points in space and can generate points outside the mesh
 */
void Streamlines::getRandomCellPt(double pt[3], vtkDataSet *ds)
{
    int numCells = (int)ds->GetNumberOfCells();
    // XXX: Not uniform distribution (shouldn't use mod, and assumes
    // all cells are equal area/volume)
    int cell = rand() % numCells;
    double bounds[6];
    ds->GetCellBounds(cell, bounds);
    // Note: point is inside AABB of cell, but may be outside the cell
    getRandomPoint(pt, bounds);
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

    double cellSizeRange[2];
    double avgSize;
    _dataSet->getCellSizeRange(cellSizeRange, &avgSize);
    _dataScale = avgSize / 8.;

    vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetVectors() == NULL) {
        TRACE("No vector point data found in DataSet %s", _dataSet->getName().c_str());
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
    _streamTracer->SetMaximumPropagation(maxBound);

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOn();
    }
    if (_seedMapper == NULL) {
        _seedMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _seedMapper->SetResolveCoincidentTopologyToPolygonOffset();
    }

    // Set up seed source object
    setSeedToRandomPoints(200);

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

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }

    setColorMode(_colorMode);

    _linesActor->SetMapper(_pdMapper);
    _pdMapper->Update();
    _seedMapper->Update();
}

/**
 * \brief Use randomly distributed seed points
 *
 * Note: The current implementation doesn't give a uniform 
 * distribution of points, and points outside the mesh bounds
 * may be generated
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

        for (int i = 0; i < numPoints; i++) {
            double pt[3];
            getRandomCellPt(pt, _dataSet->getVtkDataSet());
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
 * \brief Create seed points inside a disk with an optional hole
 *
 * \param[in] center Center point of disk
 * \param[in] normal Normal vector to orient disk
 * \param[in] radius Radius of disk
 * \param[in] innerRadius Radius of hole at center of disk
 * \param[in] numPoints Number of random points to generate
 */
void Streamlines::setSeedToDisk(double center[3],
                                double normal[3],
                                double radius,
                                double innerRadius,
                                int numPoints)
{
    if (_streamTracer != NULL) {
        // Set up seed source object
        vtkSmartPointer<vtkPolyData> seed = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

        // The following code is based on vtkRegularPolygonSource::RequestData

        double px[3];
        double py[3];
        double axis[3] = {1., 0., 0.};

        if (vtkMath::Normalize(normal) == 0.0) {
            normal[0] = 0.0;
            normal[1] = 0.0;
            normal[2] = 1.0;
        }

        // Find axis in plane (orthogonal to normal)
        bool done = false;
        vtkMath::Cross(normal, axis, px);
        if (vtkMath::Normalize(px) > 1.0e-3) {
            done = true;
        }
        if (!done) {
            axis[0] = 0.0;
            axis[1] = 1.0;
            axis[2] = 0.0;
            vtkMath::Cross(normal, axis, px);
            if (vtkMath::Normalize(px) > 1.0e-3) {
                done = true;
            }
        }
        if (!done) {
            axis[0] = 0.0;
            axis[1] = 0.0;
            axis[2] = 1.0;
            vtkMath::Cross(normal, axis, px);
            vtkMath::Normalize(px);
        }
        // Create third orthogonal basis vector
        vtkMath::Cross(px, normal, py);

        double minSquared = (innerRadius*innerRadius)/(radius*radius);
        for (int j = 0; j < numPoints; j++) {
            // Get random sweep angle and radius
            double angle = getRandomNum(0, 2.0 * vtkMath::DoublePi());
            // Need sqrt to get uniform distribution
            double r = sqrt(getRandomNum(minSquared, 1)) * radius;
            double pt[3];
            for (int i = 0; i < 3; i++) {
                pt[i] = center[i] + r * (px[i] * cos(angle) + py[i] * sin(angle));
            }
            TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
            pts->InsertNextPoint(pt);
            cells->InsertNextCell(1);
            cells->InsertCellPoint(j);
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
 * \brief Use seed points from an n-sided polygon
 *
 * \param[in] center Center point of polygon
 * \param[in] normal Normal vector to orient polygon
 * \param[in] angle Angle in degrees to rotate about normal
 * \param[in] radius Radius of circumscribing circle
 * \param[in] numSides Number of polygon sides (and points) to generate
 */
void Streamlines::setSeedToPolygon(double center[3],
                                   double normal[3],
                                   double angle,
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

        if (angle != 0.0) {
            vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
            trans->RotateWXYZ(angle, normal);
            vtkSmartPointer<vtkTransformPolyDataFilter> transFilt =
                vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            transFilt->SetInputConnection(seed->GetOutputPort());
            transFilt->SetTransform(trans);
        }

        TRACE("Seed points: %d", numSides);
        vtkSmartPointer<vtkDataSet> oldSeed;
        if (_streamTracer->GetSource() != NULL) {
            oldSeed = _streamTracer->GetSource();
        }

        if (angle != 0.0) {
            vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
            trans->Translate(+center[0], +center[1], +center[2]);
            trans->RotateWXYZ(angle, normal);
            trans->Translate(-center[0], -center[1], -center[2]);
            vtkSmartPointer<vtkTransformPolyDataFilter> transFilt =
                vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            transFilt->SetInputConnection(seed->GetOutputPort());
            transFilt->SetTransform(trans);
            _streamTracer->SetSourceConnection(transFilt->GetOutputPort());
            _seedMapper->SetInputConnection(transFilt->GetOutputPort());
        } else {
            _streamTracer->SetSourceConnection(seed->GetOutputPort());
            _seedMapper->SetInputConnection(seed->GetOutputPort());
        }

        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }
    }
}

/**
 * \brief Use seed points from an n-sided polygon
 *
 * \param[in] center Center point of polygon
 * \param[in] normal Normal vector to orient polygon
 * \param[in] angle Angle in degrees to rotate about normal
 * \param[in] radius Radius of circumscribing circle
 * \param[in] numSides Number of polygon sides (and points) to generate
 * \param[in] numPoints Number of random points to generate
 */
void Streamlines::setSeedToFilledPolygon(double center[3],
                                         double normal[3],
                                         double angle,
                                         double radius,
                                         int numSides,
                                         int numPoints)
{
    if (_streamTracer != NULL) {
         // Set up seed source object
        vtkSmartPointer<vtkPolyData> seed = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

        // The following code is based on vtkRegularPolygonSource::RequestData

        double px[3];
        double py[3];
        double axis[3] = {1., 0., 0.};

        if (vtkMath::Normalize(normal) == 0.0) {
            normal[0] = 0.0;
            normal[1] = 0.0;
            normal[2] = 1.0;
        }

        // Find axis in plane (orthogonal to normal)
        bool done = false;
        vtkMath::Cross(normal, axis, px);
        if (vtkMath::Normalize(px) > 1.0e-3) {
            done = true;
        }
        if (!done) {
            axis[0] = 0.0;
            axis[1] = 1.0;
            axis[2] = 0.0;
            vtkMath::Cross(normal, axis, px);
            if (vtkMath::Normalize(px) > 1.0e-3) {
                done = true;
            }
        }
        if (!done) {
            axis[0] = 0.0;
            axis[1] = 0.0;
            axis[2] = 1.0;
            vtkMath::Cross(normal, axis, px);
            vtkMath::Normalize(px);
        }
        // Create third orthogonal basis vector
        vtkMath::Cross(px, normal, py);

        double verts[numSides][3];
        double sliceTheta = 2.0 * vtkMath::DoublePi() / (double)numSides;
        angle = vtkMath::RadiansFromDegrees(angle);
        for (int j = 0; j < numSides; j++) {
            for (int i = 0; i < 3; i++) {
                double theta = sliceTheta * (double)j - angle;
                verts[j][i] = center[i] + radius * (px[i] * cos(theta) + 
                                                    py[i] * sin(theta));
            }
            TRACE("Vert %d: %g %g %g", j, verts[j][0], verts[j][1], verts[j][2]);
        }

        // Note: this gives a uniform distribution because the polygon is regular and
        // the triangular sections have equal area
        if (numSides == 3) {
            for (int j = 0; j < numPoints; j++) {
                double pt[3];
                getRandomPointInTriangle(pt, verts[0], verts[1], verts[2]);
                TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
                pts->InsertNextPoint(pt);
                cells->InsertNextCell(1);
                cells->InsertCellPoint(j);
            }
        } else {
            for (int j = 0; j < numPoints; j++) {
                // Get random triangle section
                int tri = rand() % numSides;
                double pt[3];
                getRandomPointInTriangle(pt, center, verts[tri], verts[(tri+1) % numSides]);
                TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
                pts->InsertNextPoint(pt);
                cells->InsertNextCell(1);
                cells->InsertCellPoint(j);
            }
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
        setCulling(_linesActor->GetProperty(), false);
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
        tubeFilter->SetRadius(_dataScale * radius);
        _pdMapper->SetInputConnection(_lineFilter->GetOutputPort());
        if (_faceCulling && _opacity == 1.0)
            setCulling(_linesActor->GetProperty(), true);
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
        ribbonFilter->SetWidth(_dataScale * width);
        ribbonFilter->SetAngle(angle);
        ribbonFilter->UseDefaultNormalOn();
        _pdMapper->SetInputConnection(_lineFilter->GetOutputPort());
        setCulling(_linesActor->GetProperty(), false);
        _linesActor->GetProperty()->SetRepresentationToSurface();
        _linesActor->GetProperty()->LightingOn();
    }
}

void Streamlines::updateRanges(bool useCumulative,
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
    } else {
        _dataSet->getScalarRange(_dataRange);
        _dataSet->getVectorRange(_vectorMagnitudeRange);
        for (int i = 0; i < 3; i++) {
            _dataSet->getVectorRange(_vectorComponentRange[i], i);
        }
    }

    // Need to update color map ranges and/or active vector field
    setColorMode(_colorMode);
}

void Streamlines::setColorMode(ColorMode mode)
{
    _colorMode = mode;
    if (_dataSet == NULL || _pdMapper == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    switch (mode) {
    case COLOR_BY_SCALAR: {
        _pdMapper->ScalarVisibilityOn();
        _pdMapper->SetScalarModeToDefault();
        if (_lut != NULL) {
            _lut->SetRange(_dataRange);
        }
    }
        break;
    case COLOR_BY_VECTOR_MAGNITUDE: {
        _pdMapper->ScalarVisibilityOn();
        _pdMapper->SetScalarModeToUsePointFieldData();
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            _pdMapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorMagnitudeRange);
            _lut->SetVectorModeToMagnitude();
        }
    }
        break;
    case COLOR_BY_VECTOR_X:
        _pdMapper->ScalarVisibilityOn();
        _pdMapper->SetScalarModeToUsePointFieldData();
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            _pdMapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[0]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        _pdMapper->ScalarVisibilityOn();
        _pdMapper->SetScalarModeToUsePointFieldData();
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            _pdMapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[1]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        _pdMapper->ScalarVisibilityOn();
        _pdMapper->SetScalarModeToUsePointFieldData();
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            _pdMapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[2]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    case COLOR_CONSTANT:
    default:
        _pdMapper->ScalarVisibilityOff();
        break;
    }
}

/**
 * \brief Called when the color map has been edited
 */
void Streamlines::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Streamlines::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        if (_pdMapper != NULL) {
            _pdMapper->UseLookupTableScalarRangeOn();
            _pdMapper->SetLookupTable(_lut);
        }
    }

    _lut->DeepCopy(cmap->getLookupTable());

    switch (_colorMode) {
    case COLOR_CONSTANT:
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
 * \brief Turn on/off lighting of this object
 */
void Streamlines::setLighting(bool state)
{
    _lighting = state;
    if (_linesActor != NULL)
        _linesActor->GetProperty()->SetLighting((state ? 1 : 0));
}

/**
 * \brief Set opacity of this object
 */
void Streamlines::setOpacity(double opacity)
{
    _opacity = opacity;
    if (_linesActor != NULL) {
        _linesActor->GetProperty()->SetOpacity(_opacity);
        if (_opacity < 1.0)
            setCulling(_linesActor->GetProperty(), false);
        else if (_faceCulling && _lineType == TUBES)
            setCulling(_linesActor->GetProperty(), true);
    }
    if (_seedActor != NULL) {
        _seedActor->GetProperty()->SetOpacity(_opacity);
    }
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
void Streamlines::setColor(float color[3])
{
    _color[0] = color[0];
    _color[1] = color[1];
    _color[2] = color[2];
    if (_linesActor != NULL)
        _linesActor->GetProperty()->SetColor(_color[0], _color[1], _color[2]);
}

/**
 * \brief Set RGB color of stream line edges
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
