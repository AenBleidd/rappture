/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCubeSource.h>
#include <vtkReverseSense.h>

#include "Parallelpiped.h"
#include "Trace.h"

using namespace VtkVis;

Parallelpiped::Parallelpiped() :
    Shape()
{
}

Parallelpiped::~Parallelpiped()
{
    TRACE("Deleting Parallelpiped");
}

void Parallelpiped::update()
{
    if (_box == NULL) {
	_box = vtkSmartPointer<vtkCubeSource>::New();
        _box->SetCenter(0.5, 0.5, 0.5);
        _box->SetXLength(1);
        _box->SetYLength(1);
        _box->SetZLength(1);
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOff();
    }

    if (_transform == NULL) {
        _transform = vtkSmartPointer<vtkTransform>::New();
    }

    vtkSmartPointer<vtkTransformPolyDataFilter> filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    filter->SetInputConnection(_box->GetOutputPort());
    filter->SetTransform(_transform);
    _pdMapper->SetInputConnection(filter->GetOutputPort());

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

/**
 * \brief Create parallelpiped from 3 basis vectors
 * 
 * This method expects the order of vectors to define a right-handed coordinate
 * system, so ((vec1 cross vec2) dot vec3) should be positive.
 */
void Parallelpiped::setVectors(double vec1[3], double vec2[3], double vec3[3])
{
    double matrix[16];
    memset(matrix, 0, sizeof(double)*16);
    matrix[0] = vec1[0];
    matrix[4] = vec1[1];
    matrix[8] = vec1[2];
    matrix[1] = vec2[0];
    matrix[5] = vec2[1];
    matrix[9] = vec2[2];
    matrix[2] = vec3[0];
    matrix[6] = vec3[1];
    matrix[10] = vec3[2];
    matrix[15] = 1;
    _transform->SetMatrix(matrix);
}

void Parallelpiped::flipNormals(bool state)
{
    if (_box == NULL || _pdMapper == NULL)
        return;

    if (state) {
        vtkSmartPointer<vtkReverseSense> filter = vtkSmartPointer<vtkReverseSense>::New();
        filter->ReverseCellsOn();
        filter->ReverseNormalsOn();
        filter->SetInputConnection(_box->GetOutputPort());

        _pdMapper->SetInputConnection(filter->GetOutputPort());
    } else {
        _pdMapper->SetInputConnection(_box->GetOutputPort());
    }
}
