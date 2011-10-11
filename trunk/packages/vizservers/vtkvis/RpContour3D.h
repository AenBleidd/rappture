/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_CONTOUR3D_H__
#define __RAPPTURE_VTKVIS_CONTOUR3D_H__

#include <vtkSmartPointer.h>
#include <vtkContourFilter.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPlaneCollection.h>

#include <vector>

#include "ColorMap.h"
#include "RpVtkGraphicsObject.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief 3D Contour isosurfaces (geometry)
 */
class Contour3D : public VtkGraphicsObject {
public:
    Contour3D(int numContours);

    Contour3D(const std::vector<double>& contours);

    virtual ~Contour3D();

    virtual const char *getClassName() const
    {
        return "Contour3D";
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setContours(int numContours);

    void setContourList(const std::vector<double>& contours);

    int getNumContours() const;

    const std::vector<double>& getContourList() const;

    void setColorMap(ColorMap *colorMap);

    /**
     * \brief Return the ColorMap in use
     */
    ColorMap *getColorMap()
    {
        return _colorMap;
    }

    void updateColorMap();

    virtual void updateRanges(Renderer *renderer);

private:
    Contour3D();

    virtual void update();

    int _numContours;
    std::vector<double> _contours;
    ColorMap *_colorMap;

    vtkSmartPointer<vtkContourFilter> _contourFilter;
    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkPolyDataMapper> _contourMapper;
};

}
}

#endif
