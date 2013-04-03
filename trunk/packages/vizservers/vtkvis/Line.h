/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_LINE_H
#define VTKVIS_LINE_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkLineSource.h>

#include "Shape.h"
#include "VtkDataSet.h"

namespace VtkVis {

/**
 * \brief VTK PolyData Line
 *
 * This class creates a line
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

    void setEndPoints(double pt1[3], double pt2[3])
    {
        if (_line != NULL) {
            _line->SetPoint1(pt1);
	    _line->SetPoint2(pt2);
	}
    }

private:
    virtual void update();

    vtkSmartPointer<vtkLineSource> _line;
};

}

#endif
