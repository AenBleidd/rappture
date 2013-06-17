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

#include "Shape.h"
#include "DataSet.h"

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

    void setStartPoint(double pt[3])
    {
        if (_arc != NULL) {
            double polarVec[3];
            for (int i = 0; i < 3; i++) {
                polarVec[i] = pt[i] - _arc->GetCenter()[i];
            }
            setPolarVector(polarVec);
 	}
    }

    void setNormal(double norm[3])
    {
        if (_arc != NULL) {
            _arc->SetNormal(norm);
	}
    }

    void setPolarVector(double vec[3])
    {
        if (_arc != NULL) {
            _arc->SetPolarVector(vec);
	}
    }

    void setAngle(double angle)
    {
        if (_arc != NULL) {
            _arc->SetAngle(angle);
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
