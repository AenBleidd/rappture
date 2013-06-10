/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdlib>
#include <ctime>
#include <cfloat>
#include <cmath>
#include <cstring>

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
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPolygon.h>
#include <vtkPolyData.h>
#include <vtkTubeFilter.h>
#include <vtkRibbonFilter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkMaskPoints.h>

#include "Streamlines.h"
#include "Renderer.h"
#include "Trace.h"

using namespace VtkVis;

Streamlines::Streamlines() :
    GraphicsObject(),
    _lineType(LINES),
    _colorMap(NULL),
    _colorMode(COLOR_BY_VECTOR_MAGNITUDE),
    _colorFieldType(DataSet::POINT_DATA),
    _seedVisible(true),
    _renderer(NULL),
    _dataScale(1)
{
    _faceCulling = true;
    _color[0] = 1.0f;
    _color[1] = 1.0f;
    _color[2] = 1.0f;
    _seedColor[0] = 1.0f;
    _seedColor[1] = 1.0f;
    _seedColor[2] = 1.0f;
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
    vtkMath::RandomSeed((int)time(NULL));
    srand((unsigned int)time(NULL));
}

Streamlines::~Streamlines()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Streamlines for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Streamlines with NULL DataSet");
#endif
}

void Streamlines::setDataSet(DataSet *dataSet,
                             Renderer *renderer)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        _renderer = renderer;

        if (renderer->getUseCumulativeRange()) {
            renderer->getCumulativeDataRange(_dataRange,
                                             _dataSet->getActiveScalarsName(),
                                             1);
            renderer->getCumulativeDataRange(_vectorMagnitudeRange,
                                             _dataSet->getActiveVectorsName(),
                                             3);
            for (int i = 0; i < 3; i++) {
                renderer->getCumulativeDataRange(_vectorComponentRange[i],
                                                 _dataSet->getActiveVectorsName(),
                                                 3, i);
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
        _seedActor->GetProperty()->SetEdgeColor(_seedColor[0], _seedColor[1], _seedColor[2]);
        _seedActor->GetProperty()->SetLineWidth(1);
        _seedActor->GetProperty()->SetPointSize(2);
        _seedActor->GetProperty()->SetOpacity(_opacity);
        _seedActor->GetProperty()->SetRepresentationToWireframe();
        _seedActor->GetProperty()->LightingOff();
        setSeedVisibility(_seedVisible);
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
                                           const double v0[3],
                                           const double v1[3],
                                           const double v2[3])
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

    //TRACE("bary %g %g %g", bary[0], bary[1], bary[2]);
    // Convert to cartesian coords
    for (int i = 0; i < 3; i++) {
        pt[i] = v0[i] * bary[0] + v1[i] * bary[1] + v2[i] * bary[2];
    }
}

void Streamlines::getRandomPointInTetrahedron(double pt[3],
                                              const double v0[3],
                                              const double v1[3],
                                              const double v2[3],
                                              const double v3[3])
{
    // Choose random barycentric coordinates
    double bary[4];
    bary[0] = getRandomNum(0, 1);
    bary[1] = getRandomNum(0, 1);
    bary[2] = getRandomNum(0, 1);
    if (bary[0] + bary[1] > 1.0) {
        bary[0] = 1.0 - bary[0];
        bary[1] = 1.0 - bary[1];
    }
    if (bary[1] + bary[2] > 1.0) {
        double tmp = bary[2];
        bary[2] = 1.0 - bary[0] - bary[1];
        bary[1] = 1.0 - tmp;
    } else if (bary[0] + bary[1] + bary[2] > 1.0) {
        double tmp = bary[2];
        bary[2] = bary[0] + bary[1] + bary[2] - 1.0;
        bary[0] = 1.0 - bary[1] - tmp;
    }
    bary[3] = 1.0 - bary[0] - bary[1] - bary[2];
    //TRACE("bary %g %g %g %g", bary[0], bary[1], bary[2], bary[3]);
    // Convert to cartesian coords
    for (int i = 0; i < 3; i++) {
#if 0
        pt[i] = (v0[i] - v3[i]) * bary[0] +
                (v1[i] - v3[i]) * bary[1] + 
                (v2[i] - v3[i]) * bary[2] + v3[i];
#else
        pt[i] = v0[i] * bary[0] + v1[i] * bary[1] +
                v2[i] * bary[2] + v3[i] * bary[3];
#endif
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
 * Note: This currently doesn't always give a uniform distribution 
 * of points in space and can generate points outside the mesh for
 * unusual cell types
 */
void Streamlines::getRandomCellPt(double pt[3], vtkDataSet *ds)
{
    int numCells = (int)ds->GetNumberOfCells();
    // XXX: Not uniform distribution (shouldn't use mod, and assumes
    // all cells are equal area/volume)
    int cell = rand() % numCells;
    int type = ds->GetCellType(cell);
    switch (type) {
    case VTK_VERTEX: {
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() == 1);
        ds->GetPoint(ptIds->GetId(0), pt);
    }
        break;
    case VTK_POLY_VERTEX: {
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() >= 1);
        int id = rand() % ptIds->GetNumberOfIds();
        ds->GetPoint(ptIds->GetId(id), pt);
    }
        break;
    case VTK_LINE: {
        double v[2][3];
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() == 2);
        for (int i = 0; i < 2; i++) {
            ds->GetPoint(ptIds->GetId(i), v[i]);
        }
        getRandomPointOnLineSegment(pt, v[0], v[1]);
    }
        break;
    case VTK_POLY_LINE: {
        double v[2][3];
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() >= 2);
        int id = rand() % (ptIds->GetNumberOfIds()-1);
        for (int i = 0; i < 2; i++) {
            ds->GetPoint(ptIds->GetId(id+i), v[i]);
        }
        getRandomPointOnLineSegment(pt, v[0], v[1]);
    }
        break;
    case VTK_TRIANGLE: {
        double v[3][3];
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() == 3);
        for (int i = 0; i < 3; i++) {
            ds->GetPoint(ptIds->GetId(i), v[i]);
        }
        getRandomPointInTriangle(pt, v[0], v[1], v[2]);
    }
        break;
    case VTK_TRIANGLE_STRIP: {
        double v[3][3];
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() >= 3);
        int id = rand() % (ptIds->GetNumberOfIds()-2);
        for (int i = 0; i < 3; i++) {
            ds->GetPoint(ptIds->GetId(id+i), v[i]);
        }
        getRandomPointInTriangle(pt, v[0], v[1], v[2]);
    }
        break;
    case VTK_POLYGON: {
        vtkPolygon *poly = vtkPolygon::SafeDownCast(ds->GetCell(cell));
        assert (poly != NULL);
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        poly->Triangulate(ptIds);
        assert(ptIds->GetNumberOfIds() >= 3 && ptIds->GetNumberOfIds() % 3 == 0);
        int tri = rand() % (ptIds->GetNumberOfIds()/3);
        double v[3][3];
        for (int i = 0; i < 3; i++) {
            ds->GetPoint(ptIds->GetId(i + tri * 3), v[i]);
        }
        getRandomPointInTriangle(pt, v[0], v[1], v[2]);
    }
        break;
    case VTK_QUAD: {
        double v[4][3];
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() == 4);
        for (int i = 0; i < 4; i++) {
            ds->GetPoint(ptIds->GetId(i), v[i]);
        }
        int tri = rand() & 0x1;
        if (tri) {
            getRandomPointInTriangle(pt, v[0], v[1], v[2]);
        } else {
            getRandomPointInTriangle(pt, v[0], v[2], v[3]);
        }
    }
        break;
    case VTK_TETRA: {
        double v[4][3];
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() == 4);
        for (int i = 0; i < 4; i++) {
            ds->GetPoint(ptIds->GetId(i), v[i]);
        }
        getRandomPointInTetrahedron(pt, v[0], v[1], v[2], v[3]);
    }
        break;
    case VTK_WEDGE: {
        double v[6][3];
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() == 6);
        for (int i = 0; i < 6; i++) {
            ds->GetPoint(ptIds->GetId(i), v[i]);
        }
        double vv[3][3];
        getRandomPointOnLineSegment(vv[0], v[0], v[3]);
        getRandomPointOnLineSegment(vv[1], v[1], v[4]);
        getRandomPointOnLineSegment(vv[2], v[2], v[5]);
        getRandomPointInTriangle(pt, vv[0], vv[1], vv[2]);
    }
        break;
    case VTK_PYRAMID: {
        double v[5][3];
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        ds->GetCellPoints(cell, ptIds);
        assert(ptIds->GetNumberOfIds() == 5);
        for (int i = 0; i < 5; i++) {
            ds->GetPoint(ptIds->GetId(i), v[i]);
        }
        int tetra = rand() & 0x1;
        if (tetra) {
            getRandomPointInTetrahedron(pt, v[0], v[1], v[2], v[4]);
        } else {
            getRandomPointInTetrahedron(pt, v[0], v[2], v[3], v[4]);
        }
    }
        break;
    case VTK_PIXEL:
    case VTK_VOXEL: {
        double bounds[6];
        ds->GetCellBounds(cell, bounds);
        getRandomPoint(pt, bounds);
    }
        break;
    default: {
        vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        ds->GetCell(cell)->Triangulate(0, ptIds, pts);
        if (ptIds->GetNumberOfIds() % 4 == 0) {
            int tetra = rand() % (ptIds->GetNumberOfIds()/4);
            double v[4][3];
            for (int i = 0; i < 4; i++) {
                ds->GetPoint(ptIds->GetId(i + tetra * 4), v[i]);
            }
            getRandomPointInTetrahedron(pt, v[0], v[1], v[2], v[4]);
        } else {
            assert(ptIds->GetNumberOfIds() % 3 == 0);
            int tri = rand() % (ptIds->GetNumberOfIds()/3);
            double v[3][3];
            for (int i = 0; i < 3; i++) {
                ds->GetPoint(ptIds->GetId(i + tri * 3), v[i]);
            }
            getRandomPointInTriangle(pt, v[0], v[1], v[2]);
        }
    }
    }
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
    double xLen = bounds[1] - bounds[0];
    double yLen = bounds[3] - bounds[2];
    double zLen = bounds[5] - bounds[4];
    double maxBound = 0.0;
    if (xLen > maxBound) {
        maxBound = xLen;
    }
    if (yLen > maxBound) {
        maxBound = yLen;
    }
    if (zLen > maxBound) {
        maxBound = zLen;
    }

    double cellSizeRange[2];
    double avgSize;
    _dataSet->getCellSizeRange(cellSizeRange, &avgSize);
    _dataScale = avgSize / 8.;

    vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetVectors() == NULL) {
        TRACE("No vector point data found in DataSet %s", _dataSet->getName().c_str());
        if (ds->GetCellData() != NULL &&
            ds->GetCellData()->GetVectors() != NULL) {
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
            USER_ERROR("No vector field was found in the data set.");
            return;
        }
    }

    if (_streamTracer == NULL) {
        _streamTracer = vtkSmartPointer<vtkStreamTracer>::New();
    }

    if (_dataSet->isCloud()) {
        // DataSet is a point cloud
        PrincipalPlane plane;
        double offset;
        vtkDataSet *mesherOutput = NULL;
        if (_dataSet->is2D(&plane, &offset)) {
            // Generate a 2D PolyData
            _mesher = vtkSmartPointer<vtkDelaunay2D>::New();
            if (plane == PLANE_ZY) {
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateWXYZ(90, 0, 1, 0);
                if (offset != 0.0) {
                    trans->Translate(-offset, 0, 0);
                }
                vtkDelaunay2D::SafeDownCast(_mesher)->SetTransform(trans);
            } else if (plane == PLANE_XZ) {
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateWXYZ(-90, 1, 0, 0);
                if (offset != 0.0) {
                    trans->Translate(0, -offset, 0);
                }
                vtkDelaunay2D::SafeDownCast(_mesher)->SetTransform(trans);
            } else if (offset != 0.0) {
                // XY with Z offset
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->Translate(0, 0, -offset);
                vtkDelaunay2D::SafeDownCast(_mesher)->SetTransform(trans);
            }
#ifdef USE_VTK6
            vtkDelaunay2D::SafeDownCast(_mesher)->SetInputData(ds);
#else
            _mesher->SetInput(ds);
#endif
            _streamTracer->SetInputConnection(_mesher->GetOutputPort());
            _mesher->Update();
            mesherOutput = vtkDelaunay2D::SafeDownCast(_mesher)->GetOutput();
        } else {
            // Generate a 3D unstructured grid
            _mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
            vtkDelaunay3D::SafeDownCast(_mesher)->SetInputData(ds);
#else
            _mesher->SetInput(ds);
#endif
            _streamTracer->SetInputConnection(_mesher->GetOutputPort());
            _mesher->Update();
            mesherOutput = vtkDelaunay3D::SafeDownCast(_mesher)->GetOutput();
        }
        DataSet::getCellSizeRange(mesherOutput, cellSizeRange, &avgSize);
        _dataScale = avgSize / 8.;
     } else {
        // DataSet is NOT a cloud (has cells)
#ifdef USE_VTK6
        _streamTracer->SetInputData(ds);
#else
        _streamTracer->SetInput(ds);
#endif
    }

    _streamTracer->SetMaximumPropagation(xLen + yLen + zLen);
    _streamTracer->SetIntegratorTypeToRungeKutta45();

    TRACE("Setting streamlines max length to %g", 
          _streamTracer->GetMaximumPropagation());

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOn();
    }
    if (_seedMapper == NULL) {
        _seedMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _seedMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _seedMapper->ScalarVisibilityOff();
    }

    // Set up seed source object
    setSeedToFilledMesh(200);

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

#if 0 && defined(WANT_TRACE)
    _streamTracer->Update();
    vtkPolyData *pd = _streamTracer->GetOutput();
    DataSet::print(pd);
    TRACE("Points: %d Verts: %d Lines: %d Polys: %d Strips: %d",
          pd->GetNumberOfPoints(),
          pd->GetNumberOfVerts(),
          pd->GetNumberOfLines(),
          pd->GetNumberOfPolys(),
          pd->GetNumberOfStrips());
#if 0
    vtkCellArray *arr = pd->GetLines();
    arr->InitTraversal();
    vtkIdType npts, *pts;
    arr->GetNextCell(npts, pts);
    for (int i = 0; i < npts; i++) {
        TRACE("Pt: %d", pts[i]);
    }
#endif
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
#ifdef WANT_TRACE
    double *b = getBounds();
    TRACE("bounds: %g %g %g %g %g %g", b[0], b[1], b[2], b[3], b[4], b[5]);
#endif
}

void Streamlines::setNumberOfSeedPoints(int numPoints)
{
    switch(_seedType) {
    case DATASET_FILLED_MESH:
        setSeedToFilledMesh(numPoints);
        break;
    case FILLED_MESH:
        if (_seedMesh != NULL) {
            setSeedToFilledMesh(_seedMesh, numPoints);
        } else {
            ERROR("NULL _seedMesh");
        }
        break;
    case DATASET_MESH_POINTS:
        setSeedToMeshPoints(numPoints);
        break;
    case MESH_POINTS:
        setSeedToMeshPoints(_seedMesh, numPoints);
        break;
    default:
        ERROR("Can't set number of points for seed type %d", _seedType);
        break;
    }
    _pdMapper->Update();
    _seedMapper->Update();
}

/**
 * \brief Use points of the DataSet associated with this 
 * Streamlines as seeds
 */
void Streamlines::setSeedToMeshPoints(int maxPoints)
{
    _seedType = DATASET_MESH_POINTS;
    setSeedToMeshPoints(_dataSet->getVtkDataSet(), maxPoints);
}

/**
 * \brief Use seed points randomly distributed within the cells 
 * of the DataSet associated with this Streamlines
 *
 * Note: The current implementation doesn't give a uniform 
 * distribution of points, and points outside the mesh bounds
 * may be generated
 *
 * \param[in] numPoints Number of random seed points to generate
 */
void Streamlines::setSeedToFilledMesh(int numPoints)
{
    _seedType = DATASET_FILLED_MESH;
    setSeedToFilledMesh(_dataSet->getVtkDataSet(), numPoints);
}

/**
 * \brief Use points of a supplied vtkDataSet as seeds
 *
 * \param[in] seed vtkDataSet with points to use as seeds
 */
void Streamlines::setSeedToMeshPoints(vtkDataSet *seed, int maxPoints)
{
    if (seed != _dataSet->getVtkDataSet()) {
        _seedType = MESH_POINTS;
        _seedMesh = seed;
    } else {
        _seedType = DATASET_MESH_POINTS;
        _seedMesh = NULL;
    }

    if (_streamTracer == NULL)
        return;

#ifndef USE_VTK6
    vtkSmartPointer<vtkDataSet> oldSeed;
    if (_streamTracer->GetSource() != NULL) {
        oldSeed = _streamTracer->GetSource();
    }
#endif

    if (maxPoints > 0 && seed->GetNumberOfPoints() > maxPoints) {
        TRACE("Seed points: %d", maxPoints);
        vtkSmartPointer<vtkMaskPoints> mask = vtkSmartPointer<vtkMaskPoints>::New();
#ifdef USE_VTK6
        mask->SetInputData(seed);
#else
        mask->SetInput(seed);
#endif
        mask->SetMaximumNumberOfPoints(maxPoints);
        mask->RandomModeOn();
        mask->GenerateVerticesOff();
        _streamTracer->SetSourceConnection(mask->GetOutputPort());

        vtkSmartPointer<vtkVertexGlyphFilter> vertFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
        vertFilter->SetInputConnection(mask->GetOutputPort());
        _seedMapper->SetInputConnection(vertFilter->GetOutputPort());
    } else {
        TRACE("Seed points: %d", seed->GetNumberOfPoints());

#ifdef USE_VTK6
        _streamTracer->SetSourceData(seed);
#else
        _streamTracer->SetSource(seed);
#endif
        if (vtkPolyData::SafeDownCast(seed) != NULL) {
#ifdef USE_VTK6
            _seedMapper->SetInputData(vtkPolyData::SafeDownCast(seed));
#else
            _seedMapper->SetInput(vtkPolyData::SafeDownCast(seed));
#endif
        } else {
            vtkSmartPointer<vtkVertexGlyphFilter> vertFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
#ifdef USE_VTK6
            vertFilter->SetInputData(seed);
#else
            vertFilter->SetInput(seed);
#endif
            _seedMapper->SetInputConnection(vertFilter->GetOutputPort());
        }
    }

#ifndef USE_VTK6
    if (oldSeed != NULL) {
        oldSeed->SetPipelineInformation(NULL);
    }
#endif
}

/**
 * \brief Use seed points randomly distributed within the cells 
 * of a supplied vtkDataSet
 *
 * Note: The current implementation doesn't give a uniform 
 * distribution of points, and points outside the mesh bounds
 * may be generated
 *
 * \param[in] ds vtkDataSet containing cells
 * \param[in] numPoints Number of random seed points to generate
 */
void Streamlines::setSeedToFilledMesh(vtkDataSet *ds, int numPoints)
{
    if (ds != _dataSet->getVtkDataSet()) {
        _seedMesh = ds;
        _seedType = FILLED_MESH;
#ifdef DEBUG
        DataSet::print(ds);
#endif
    } else {
        _seedMesh = NULL;
        if (_mesher != NULL) {
            if (vtkDelaunay2D::SafeDownCast(_mesher) != NULL) {
                ds = vtkDelaunay2D::SafeDownCast(_mesher)->GetOutput();
            } else {
                ds = vtkDelaunay3D::SafeDownCast(_mesher)->GetOutput();
            }
        }
    }
    if (_streamTracer != NULL) {
        // Set up seed source object
        vtkSmartPointer<vtkPolyData> seed = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

        if (ds->GetNumberOfCells() < 1) {
            ERROR("No cells in mesh");
        }

        for (int i = 0; i < numPoints; i++) {
            double pt[3];
            getRandomCellPt(pt, ds);
            //TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
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

#ifdef USE_VTK6
        _streamTracer->SetSourceData(seed);
        _seedMapper->SetInputData(seed);
#else
        _streamTracer->SetSource(seed);

        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }
        _seedMapper->SetInput(seed);
#endif
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
    _seedType = RAKE;
    _seedMesh = NULL;
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
            //TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
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
#ifdef USE_VTK6
        _streamTracer->SetSourceData(seed);
        _seedMapper->SetInputData(seed);
#else
        _streamTracer->SetSource(seed);
        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }

        _seedMapper->SetInput(seed);
#endif
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
    _seedType = DISK;
    _seedMesh = NULL;
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
#ifdef USE_VTK6
            double angle = getRandomNum(0, 2.0 * vtkMath::Pi());
#else
            double angle = getRandomNum(0, 2.0 * vtkMath::DoublePi());
#endif
            // Need sqrt to get uniform distribution
            double r = sqrt(getRandomNum(minSquared, 1)) * radius;
            double pt[3];
            for (int i = 0; i < 3; i++) {
                pt[i] = center[i] + r * (px[i] * cos(angle) + py[i] * sin(angle));
            }
            //TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
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
#ifdef USE_VTK6
        _streamTracer->SetSourceData(seed);
        _seedMapper->SetInputData(seed);
#else
        _streamTracer->SetSource(seed);
        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }

        _seedMapper->SetInput(seed);
#endif
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
    _seedType = POLYGON;
    _seedMesh = NULL;
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

#ifndef USE_VTK6
        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }
#endif
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
    _seedType = FILLED_POLYGON;
    _seedMesh = NULL;
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
#ifdef USE_VTK6
        double sliceTheta = 2.0 * vtkMath::Pi() / (double)numSides;
#else
        double sliceTheta = 2.0 * vtkMath::DoublePi() / (double)numSides;
#endif
        angle = vtkMath::RadiansFromDegrees(angle);
        for (int j = 0; j < numSides; j++) {
            for (int i = 0; i < 3; i++) {
                double theta = sliceTheta * (double)j - angle;
                verts[j][i] = center[i] + radius * (px[i] * cos(theta) + 
                                                    py[i] * sin(theta));
            }
            //TRACE("Vert %d: %g %g %g", j, verts[j][0], verts[j][1], verts[j][2]);
        }

        // Note: this gives a uniform distribution because the polygon is regular and
        // the triangular sections have equal area
        if (numSides == 3) {
            for (int j = 0; j < numPoints; j++) {
                double pt[3];
                getRandomPointInTriangle(pt, verts[0], verts[1], verts[2]);
                //TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
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
                //TRACE("Seed pt: %g %g %g", pt[0], pt[1], pt[2]);
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

#ifdef USE_VTK6
        _streamTracer->SetSourceData(seed);
        _seedMapper->SetInputData(seed);
#else
        _streamTracer->SetSource(seed);
        if (oldSeed != NULL) {
            oldSeed->SetPipelineInformation(NULL);
        }

        _seedMapper->SetInput(seed);
#endif
    }
}

/**
 * \brief Set the integration method used
 */
void Streamlines::setIntegrator(IntegratorType integrator)
{
    if (_streamTracer != NULL) {
        switch (integrator) {
        case RUNGE_KUTTA2:
            _streamTracer->SetIntegratorTypeToRungeKutta2();
            break;
        case RUNGE_KUTTA4:
            _streamTracer->SetIntegratorTypeToRungeKutta4();
            break;
        case RUNGE_KUTTA45:
            _streamTracer->SetIntegratorTypeToRungeKutta45();
            break;
        default:
            ;
        }
    }
}

/**
 * \brief Set the direction of integration
 */
void Streamlines::setIntegrationDirection(IntegrationDirection dir)
{
    if (_streamTracer != NULL) {
        switch (dir) {
        case FORWARD:
            _streamTracer->SetIntegrationDirectionToForward();
            break;
        case BACKWARD:
            _streamTracer->SetIntegrationDirectionToBackward();
            break;
        case BOTH:
            _streamTracer->SetIntegrationDirectionToBoth();
            break;
        default:
            ;
        }
    }
}

/**
 * \brief Set the step size units.  Length units are world 
 * coordinates, and cell units means steps are from cell to 
 * cell.  Default is cell units.
 *
 * Note: calling this function will not convert existing
 * initial, minimum or maximum step value settings to the
 * new units, so this function should be called before 
 * setting step values.
 */
void Streamlines::setIntegrationStepUnit(StepUnit unit)
{
    if (_streamTracer != NULL) {
        switch (unit) {
        case LENGTH_UNIT:
            _streamTracer->SetIntegrationStepUnit(vtkStreamTracer::LENGTH_UNIT);
            break;
        case CELL_LENGTH_UNIT:
            _streamTracer->SetIntegrationStepUnit(vtkStreamTracer::CELL_LENGTH_UNIT);
            break;
        default:
            ;
        }
    }
}

/**
 * \brief Set initial step size for adaptive step integrator in 
 * step units (see setIntegrationStepUnit).  For non-adaptive
 * integrators, this is the fixed step size.
 */
void Streamlines::setInitialIntegrationStep(double step)
{
    if (_streamTracer != NULL) {
        _streamTracer->SetInitialIntegrationStep(step);
    }
}

/**
 * \brief Set minimum step for adaptive step integrator in 
 * step units (see setIntegrationStepUnit)
 */
void Streamlines::setMinimumIntegrationStep(double step)
{
    if (_streamTracer != NULL) {
        _streamTracer->SetMinimumIntegrationStep(step);
    }
}

/**
 * \brief Set maximum step for adaptive step integrator in 
 * step units (see setIntegrationStepUnit)
 */
void Streamlines::setMaximumIntegrationStep(double step)
{
    if (_streamTracer != NULL) {
        _streamTracer->SetMaximumIntegrationStep(step);
    }
}

/**
 * \brief Set maximum error tolerance
 */
void Streamlines::setMaximumError(double error)
{
    if (_streamTracer != NULL) {
        _streamTracer->SetMaximumError(error);
    }
}

/**
 * \brief Set maximum length of stream lines in world
 * coordinates
 */
void Streamlines::setMaxPropagation(double length)
{
    if (_streamTracer != NULL) {
        _streamTracer->SetMaximumPropagation(length);
    }
}

/**
 * \brief Set maximum number of integration steps
 */
void Streamlines::setMaxNumberOfSteps(int steps)
{
    if (_streamTracer != NULL) {
        _streamTracer->SetMaximumNumberOfSteps(steps);
    }
}

/**
 * \brief Set the minimum speed before integration stops
 */
void Streamlines::setTerminalSpeed(double speed)
{
    if (_streamTracer != NULL) {
        _streamTracer->SetTerminalSpeed(speed);
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

void Streamlines::updateRanges(Renderer *renderer)
{
    if (_dataSet == NULL) {
        ERROR("called before setDataSet");
        return;
    }

    if (renderer->getUseCumulativeRange()) {
        renderer->getCumulativeDataRange(_dataRange,
                                         _dataSet->getActiveScalarsName(),
                                         1);
        renderer->getCumulativeDataRange(_vectorMagnitudeRange,
                                         _dataSet->getActiveVectorsName(),
                                         3);
        for (int i = 0; i < 3; i++) {
            renderer->getCumulativeDataRange(_vectorComponentRange[i],
                                             _dataSet->getActiveVectorsName(),
                                             3, i);
        }
    } else {
        _dataSet->getScalarRange(_dataRange);
        _dataSet->getVectorRange(_vectorMagnitudeRange);
        for (int i = 0; i < 3; i++) {
            _dataSet->getVectorRange(_vectorComponentRange[i], i);
        }
    }

    // Need to update color map ranges and/or active vector field
    double *rangePtr = _colorFieldRange;
    if (_colorFieldRange[0] > _colorFieldRange[1]) {
        rangePtr = NULL;
    }
    setColorMode(_colorMode, _colorFieldType, _colorFieldName.c_str(), rangePtr);
}

void Streamlines::setColorMode(ColorMode mode)
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

void Streamlines::setColorMode(ColorMode mode,
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

void Streamlines::setColorMode(ColorMode mode, DataSet::DataAttributeType type,
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

    if (_dataSet == NULL || _pdMapper == NULL)
        return;

    switch (type) {
    case DataSet::POINT_DATA:
        _pdMapper->SetScalarModeToUsePointFieldData();
        break;
    case DataSet::CELL_DATA:
        _pdMapper->SetScalarModeToUseCellFieldData();
        break;
    default:
        ERROR("Unsupported DataAttributeType: %d", type);
        return;
    }

    if (name != NULL && strlen(name) > 0) {
        _pdMapper->SelectColorArray(name);
    } else {
        _pdMapper->SetScalarModeToDefault();
    }

    if (_lut != NULL) {
        if (range != NULL) {
            _lut->SetRange(range);
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
        }
    }

    switch (mode) {
    case COLOR_BY_SCALAR:
        _pdMapper->ScalarVisibilityOn();
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        _pdMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToMagnitude();
        }
        break;
    case COLOR_BY_VECTOR_X:
        _pdMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        _pdMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        _pdMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
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
    if (_seedActor != NULL) {
        _seedActor->GetProperty()->SetColor(_seedColor[0], _seedColor[1], _seedColor[2]);
        _seedActor->GetProperty()->SetEdgeColor(_seedColor[0], _seedColor[1], _seedColor[2]);
    }
}

/**
 * \brief Set point size of seed geometry (may be a no-op)
 */
void Streamlines::setSeedPointSize(float size)
{
    if (_seedActor != NULL) {
        _seedActor->GetProperty()->SetPointSize(size);
    }
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
