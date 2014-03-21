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
    enum InterpType {
        INTERP_NEAREST,
        INTERP_LINEAR,
        INTERP_CUBIC
    };

    Image();
    virtual ~Image();

    virtual const char *getClassName() const
    {
        return "Image";
    }

    virtual void initProp();

    virtual void setColor(float color[3]);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    virtual void setAspect(double aspect);

    void updateColorMap()
    {
        setColorMap(_colorMap);
    }

    void setColorMap(ColorMap *cmap);

    ColorMap *getColorMap()
    {
        return _colorMap;
    }

    void setSlicePlane(double normal[3], double origin[3])
    {
        setSliceFollowsCamera(false);

        vtkImageMapper3D *mapper = getImageMapper();
        vtkImageResliceMapper *resliceMapper = vtkImageResliceMapper::SafeDownCast(mapper);
        if (resliceMapper != NULL) {
            vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
            plane->SetNormal(normal);
            plane->SetOrigin(origin);
            resliceMapper->SetSlicePlane(plane);
        }
    }

    void setSliceFollowsCamera(bool state)
    {
        vtkImageMapper3D *mapper = getImageMapper();
        if (mapper != NULL) {
            mapper->SetSliceFacesCamera(state ? 1 : 0);
            mapper->SetSliceAtFocalPoint(state ? 1 : 0);
        }
    }

    void setJumpToNearestSlice(bool state)
    {
        vtkImageMapper3D *mapper = getImageMapper();
        vtkImageResliceMapper *resliceMapper = vtkImageResliceMapper::SafeDownCast(mapper);
        if (resliceMapper != NULL) {
            resliceMapper->SetJumpToNearestSlice(state ? 1 : 0);
        }
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

    void setUseWindowLevel(bool state)
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return;

        property->SetUseLookupTableScalarRange((state ? 0 : 1));
    }

    double getWindow()
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return 0.0;

        return property->GetColorWindow();
    }

    void setWindow(double window)
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return;

        property->SetColorWindow(window);
        property->UseLookupTableScalarRangeOff();
    }

    double getLevel()
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return 0.0;

        return property->GetColorLevel();
    }

    void setLevel(double level)
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return;

        property->SetColorLevel(level);
        property->UseLookupTableScalarRangeOff();
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

    void setInterpolationType(InterpType type)
    {
        vtkImageProperty *property = getImageProperty();
        if (property == NULL)
            return;

        switch (type) {
        case INTERP_NEAREST:
            property->SetInterpolationTypeToNearest();
            break;
        case INTERP_LINEAR:
            property->SetInterpolationTypeToLinear();
            break;
        case INTERP_CUBIC:
            property->SetInterpolationTypeToCubic();
            break;
        }
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
