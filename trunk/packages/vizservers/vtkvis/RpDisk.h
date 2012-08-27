/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_DISK_H__
#define __RAPPTURE_VTKVIS_DISK_H__

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkDiskSource.h>

#include "RpShape.h"
#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK PolyData disk
 *
 * This class creates a mesh disk
 */
class Disk : public Shape
{
public:
    Disk();
    virtual ~Disk();

    virtual const char *getClassName() const
    {
        return "Disk";
    }

    void setRadii(double inner, double outer)
    {
        if (_disk != NULL) {
            _disk->SetInnerRadius(inner);
	    _disk->SetOuterRadius(outer);
	}
    }

    void setResolution(int radialRes, int circumRes)
    {
        if (_disk != NULL) {
            _disk->SetRadialResolution(radialRes);
	    _disk->SetCircumferentialResolution(circumRes);
	}
    }

private:
    virtual void update();

    vtkSmartPointer<vtkDiskSource> _disk;
};

}
}

#endif
