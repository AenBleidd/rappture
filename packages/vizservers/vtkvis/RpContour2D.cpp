/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

#include "RpContour2D.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Contour2D::Contour2D() :
    _dataSet(NULL),
    _numContours(0),
    _edgeWidth(1.0f),
    _opacity(1.0)
{
    _dataRange[0] = 0;
    _dataRange[1] = 1;
    _edgeColor[0] = 0;
    _edgeColor[1] = 0;
    _edgeColor[2] = 0;
}

Contour2D::~Contour2D()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Contour2D for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Contour2D with NULL DataSet");
#endif
}

/**
 * \brief Specify input DataSet used to extract contours
 */
void Contour2D::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        if (_dataSet != NULL) {
            double dataRange[2];
            _dataSet->getDataRange(dataRange);
            _dataRange[0] = dataRange[0];
            _dataRange[1] = dataRange[1];
        }

        update();
    }
}

/**
 * \brief Returns the DataSet this Contour2D renders
 */
DataSet *Contour2D::getDataSet()
{
    return _dataSet;
}

/**
 * \brief Get the VTK Actor for the contour lines
 */
vtkActor *Contour2D::getActor()
{
    return _contourActor;
}

/**
 * \brief Create and initialize a VTK actor to render isolines
 */
void Contour2D::initActor()
{
    if (_contourActor == NULL) {
        _contourActor = vtkSmartPointer<vtkActor>::New();
        _contourActor->GetProperty()->EdgeVisibilityOn();
        _contourActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _contourActor->GetProperty()->SetLineWidth(_edgeWidth);
        _contourActor->GetProperty()->SetOpacity(_opacity);
        _contourActor->GetProperty()->LightingOff();
    }
}

/**
 * \brief Internal method to re-compute contours after a state change
 */
void Contour2D::update()
{
    if (_dataSet == NULL) {
        return;
    }

    initActor();

    // Contour filter to generate isolines
    if (_contourFilter == NULL) {
        _contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    }

    _contourFilter->SetInput(_dataSet->getVtkDataSet());

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
        _contourActor->SetMapper(_contourMapper);
        _contourMapper->SetInput(_contourFilter->GetOutput());
    }
}

/**
 * \brief Specify number of evenly spaced contour lines to render
 *
 * Will override any existing contours
 */
void Contour2D::setContours(int numContours)
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
void Contour2D::setContours(int numContours, double range[2])
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
void Contour2D::setContourList(const std::vector<double>& contours)
{
    _contours = contours;
    _numContours = (int)_contours.size();

    update();
}

/**
 * \brief Get the number of contours
 */
int Contour2D::getNumContours() const
{
    return _numContours;
}

/**
 * \brief Get the contour list (may be empty if number of contours
 * was specified in place of a list)
 */
const std::vector<double>& Contour2D::getContourList() const
{
    return _contours;
}

/**
 * \brief Turn on/off rendering of this contour set
 */
void Contour2D::setVisibility(bool state)
{
    if (_contourActor != NULL) {
        _contourActor->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Get visibility state of the contour set
 * 
 * \return Is contour set visible?
 */
bool Contour2D::getVisibility() const
{
    if (_contourActor == NULL) {
        return false;
    } else {
        return (_contourActor->GetVisibility() != 0);
    }
}

/**
 * \brief Set opacity used to render contour lines
 */
void Contour2D::setOpacity(double opacity)
{
    _opacity = opacity;
    if (_contourActor != NULL)
        _contourActor->GetProperty()->SetOpacity(opacity);
}

/**
 * \brief Set RGB color of contour lines
 */
void Contour2D::setEdgeColor(float color[3])
{
    _edgeColor[0] = color[0];
    _edgeColor[1] = color[1];
    _edgeColor[2] = color[2];
    if (_contourActor != NULL)
        _contourActor->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
}

/**
 * \brief Set pixel width of contour lines (may be a no-op)
 */
void Contour2D::setEdgeWidth(float edgeWidth)
{
    _edgeWidth = edgeWidth;
    if (_contourActor != NULL)
        _contourActor->GetProperty()->SetLineWidth(_edgeWidth);
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Contour2D::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_contourMapper != NULL) {
        _contourMapper->SetClippingPlanes(planes);
    }
}

/**
 * \brief Turn on/off lighting of this object
 */
void Contour2D::setLighting(bool state)
{
    if (_contourActor != NULL)
        _contourActor->GetProperty()->SetLighting((state ? 1 : 0));
}
