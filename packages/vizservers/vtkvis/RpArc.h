/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_ARC_H
#define VTKVIS_ARC_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkArcSource.h>

#include "RpShape.h"
#include "RpVtkDataSet.h"

namespace VtkVis {

/**
 * \brief VTK PolyData Arc
 *
 * This class creates a arc
 */
class Arc : public Shape
{
public:
    Arc();
    virtual ~Arc();

    virtual const char *getClassName() const
    {
        return "Arc";
    }

    void setCenter(double center[3])
    {
        if (_arc != NULL) {
            _arc->SetCenter(center);
	}
    }

    void setEndPoints(double pt1[3], double pt2[3])
    {
        if (_arc != NULL) {
            _arc->SetPoint1(pt1);
            _arc->SetPoint2(pt2);
	}
    }

    void setResolution(int res)
    {
        if (_arc != NULL) {
            _arc->SetResolution(res);
	}
    }

private:
    virtual void update();

    vtkSmartPointer<vtkArcSource> _arc;
};

}

#endif
