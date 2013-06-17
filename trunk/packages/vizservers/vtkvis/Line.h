/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_LINE_H
#define VTKVIS_LINE_H

#include <vector>

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>

#include "Shape.h"
#include "DataSet.h"

namespace VtkVis {

/**
 * \brief VTK PolyData Line
 *
 * This class creates a polyline
 */
class Line : public Shape
{
public:
    Line();
    virtual ~Line();

    virtual const char *getClassName() const
    {
        return "Line";
    }

    /**
     * \brief This interface creates a single line
     */
    void setEndPoints(double pt1[3], double pt2[3])
    {
        if (_line != NULL) {
            _line->SetPoint1(pt1);
            _line->SetPoint2(pt2);
            _line->SetPoints(NULL);
        }
    }

    /**
     * \brief This interface creates a polyline
     */
    void setPoints(std::vector<double> pts)
    {
        if (_line == NULL)
            return;
        vtkPoints *points = vtkPoints::New();
        for (size_t i = 0; i < pts.size(); i+=3) {
            points->InsertNextPoint(pts[i], pts[i+1], pts[i+2]);
        }
        _line->SetPoints(points);
    }

private:
    virtual void update();

    vtkSmartPointer<vtkLineSource> _line;
};

}

#endif
