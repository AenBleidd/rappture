/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_ARROW_H
#define VTKVIS_ARROW_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkArrowSource.h>

#include "Shape.h"
#include "DataSet.h"

namespace VtkVis {

/**
 * \brief VTK PolyData 3D Arrow
 *
 * This class creates a mesh arrow
 */
class Arrow : public Shape
{
public:
    Arrow();
    virtual ~Arrow();

    virtual const char *getClassName() const
    {
        return "Arrow";
    }

    void setTipLength(double len)
    {
        if (_arrow != NULL) {
            _arrow->SetTipLength(len);
	}
    }

    void setRadii(double tipRadius, double shaftRadius)
    {
        if (_arrow != NULL) {
            _arrow->SetTipRadius(tipRadius);
            _arrow->SetShaftRadius(shaftRadius);
	}
    }

    void setResolution(int tipRes, int shaftRes)
    {
        if (_arrow != NULL) {
            _arrow->SetTipResolution(tipRes);
            _arrow->SetShaftResolution(shaftRes);
	}
    }

    void setInvert(bool state)
    {
        if (_arrow != NULL) {
            _arrow->SetInvert(state);
	}
    }

private:
    virtual void update();

    vtkSmartPointer<vtkArrowSource> _arrow;
};

}

#endif
