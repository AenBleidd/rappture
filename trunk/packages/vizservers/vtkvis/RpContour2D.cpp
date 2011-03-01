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

using namespace Rappture::VtkVis;

Contour2D::Contour2D() :
    _dataSet(NULL),
    _numContours(0),
    _edgeWidth(1.0f),
    _opacity(1.0)
{
    _edgeColor[0] = 0;
    _edgeColor[1] = 0;
    _edgeColor[2] = 0;
}

Contour2D::~Contour2D()
{
}

/**
 * \brief Specify input DataSet used to extract contours
 */
void Contour2D::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;
        update();
    }
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
    vtkSmartPointer<vtkContourFilter> contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    contourFilter->SetInput(_dataSet->getVtkDataSet());

    // Speed up multiple contour computation at cost of extra memory use
    if (_numContours > 1) {
        contourFilter->UseScalarTreeOn();
    } else {
        contourFilter->UseScalarTreeOff();
    }
    if (_contours.empty()) {
        // Evenly spaced isovalues
        double dataRange[2];
        _dataSet->getDataRange(dataRange);
        contourFilter->GenerateValues(_numContours, dataRange[0], dataRange[1]);
    } else {
        // User-supplied isovalues
        for (int i = 0; i < (int)_contours.size(); i++) {
            contourFilter->SetValue(i, _contours[i]);
        }
    }
    if (_contourMapper == NULL) {
        _contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _contourMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _contourActor->SetMapper(_contourMapper);
    }

    _contourMapper->SetInput(contourFilter->GetOutput());
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
    _numContours = _contours.size();

    update();
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
bool Contour2D::getVisibility()
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
        if (planes == NULL)
            _contourMapper->RemoveAllClippingPlanes();
        else
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
