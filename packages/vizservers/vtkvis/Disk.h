/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_DISK_H
#define VTKVIS_DISK_H

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkDiskSource.h>

#include "Shape.h"
#include "DataSet.h"

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

    void flipNormals(bool state);

private:
    virtual void update();

    bool _flipNormals;
    vtkSmartPointer<vtkDiskSource> _disk;
};

}

#endif
