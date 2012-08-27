/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_ARROW_H__
#define __RAPPTURE_VTKVIS_ARROW_H__

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkArrowSource.h>

#include "RpShape.h"
#include "RpVtkDataSet.h"

namespace Rappture {
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
}

#endif
