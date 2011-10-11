/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_CONTOUR2D_H__
#define __RAPPTURE_VTKVIS_CONTOUR2D_H__

#include <vtkSmartPointer.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>

#include <vector>

#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief 2D Contour lines (isolines)
 */
class Contour2D : public VtkGraphicsObject {
public:
    Contour2D(int numContours);

    Contour2D(const std::vector<double>& contours);

    virtual ~Contour2D();

    virtual const char *getClassName() const
    {
        return "Contour2D";
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setContours(int numContours);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    virtual void updateRanges(Renderer *renderer);

    virtual void setColor(float color[3])
    {
        VtkGraphicsObject::setColor(color);
        VtkGraphicsObject::setEdgeColor(color);
    }

    virtual void setEdgeColor(float color[3])
    {
        setColor(color);
    }

private:
    Contour2D();

    virtual void update();

    int _numContours;
    std::vector<double> _contours;

    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
};

}
}

#endif
