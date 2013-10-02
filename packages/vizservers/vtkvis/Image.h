/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_IMAGE_H
#define VTKVIS_IMAGE_H

#include <vtkSmartPointer.h>
#include <vtkImageSlice.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSliceMapper.h>
#include <vtkLookupTable.h>
#include <vtkPlaneCollection.h>

#include "ColorMap.h"
#include "GraphicsObject.h"
#include "DataSet.h"
#include "Trace.h"

namespace VtkVis {

/**
 * \brief Image Slicer
 *
 */
class Image : public GraphicsObject
{
public:
    Image();
    virtual ~Image();

    virtual const char *getClassName() const
    {
        return "Image";
    }

    virtual void initProp();

    virtual void setColor(float color[3]);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void updateColorMap()
    {
        setColorMap(_colorMap);
    }

    void setColorMap(ColorMap *cmap);

    ColorMap *getColorMap()
    {
        return _colorMap;
    }

    void setExtents(int extent[6])
    {
        vtkImageActor *actor = getImageActor();
        if (actor == NULL)
            return;

        actor->SetDisplayExtent(extent);
    }

    void setZSlice(int z)
    {
        vtkImageActor *actor = getImageActor();
        if (actor == NULL)
            return;

        TRACE("before slice # %d, (%d,%d), whole z: (%d,%d)",
              actor->GetSliceNumber(), actor->GetSliceNumberMin(), actor->GetSliceNumberMax(),
              actor->GetZSlice(), actor->GetWholeZMin(), actor->GetWholeZMax());

        actor->SetZSlice(z);

        TRACE("after slice # %d, (%d,%d), whole z: (%d,%d)",
              actor->GetSliceNumber(), actor->GetSliceNumberMin(), actor->GetSliceNumberMax(),
              actor->GetZSlice(), actor->GetWholeZMin(), actor->GetWholeZMax());
    }

    void setWindow(double window)
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return;

        property->SetColorWindow(window);
    }

    void setLevel(double level)
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return;

        property->SetColorLevel(level);
    }

    void setWindowAndLevel(double window, double level)
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return;

        property->SetColorWindow(window);
        property->SetColorLevel(level);
    }

    void setBacking(bool state)
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return;

        property->SetBacking((state ? 1 : 0));
    }

    void setBorder(bool state)
    {
        vtkImageMapper3D *mapper = getImageMapper();
        if (mapper == NULL)
            return;

        mapper->SetBorder((state ? 1 : 0));
    }

    void setBackground(bool state)
    {
        vtkImageMapper3D *mapper = getImageMapper();
        if (mapper == NULL)
            return;

        mapper->SetBackground((state ? 1 : 0));
    }

private:
    virtual void update();

    vtkImageProperty *getImageProperty()
    {
        if (getImageSlice() != NULL) {
            return getImageSlice()->GetProperty();
        } else {
            return NULL;
        }
    }

    vtkImageMapper3D *getImageMapper()
    {
        if (getImageSlice() != NULL) {
            return getImageSlice()->GetMapper();
        } else {
            return NULL;
        }
    }

    vtkImageResliceMapper *getImageResliceMapper()
    {
        if (getImageSlice() != NULL) {
            return vtkImageResliceMapper::SafeDownCast(getImageSlice()->GetMapper());
        } else {
            return NULL;
        }
    }

    vtkImageSliceMapper *getImageSliceMapper()
    {
        if (getImageSlice() != NULL) {
            return vtkImageSliceMapper::SafeDownCast(getImageSlice()->GetMapper());
        } else {
            return NULL;
        }
    }

    ColorMap *_colorMap;

    vtkSmartPointer<vtkLookupTable> _lut;
};

}

#endif
