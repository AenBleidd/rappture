/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageActor.h>
#include <vtkImageSlice.h>
#include <vtkImageProperty.h>
#include <vtkImageMapper3D.h>
#include <vtkImageResliceMapper.h>
#include <vtkLookupTable.h>

#include "Image.h"
#include "Trace.h"

using namespace VtkVis;

#define USE_RESLICE_MAPPER

Image::Image() :
    GraphicsObject(),
    _colorMap(NULL)
{
}

Image::~Image()
{
    TRACE("Deleting Image");
}

void Image::initProp()
{
    if (_prop == NULL) {
#ifdef USE_RESLICE_MAPPER
        _prop = vtkSmartPointer<vtkImageSlice>::New();
#else
        _prop = vtkSmartPointer<vtkImageActor>::New();
#endif
        vtkImageProperty *property = getImageProperty();
        property->SetInterpolationTypeToLinear();
        property->SetBackingColor(_color[0], _color[1], _color[2]);
        property->BackingOff();
        if (_dataSet != NULL)
            _opacity = _dataSet->getOpacity();
        property->SetOpacity(_opacity);

        if (_dataSet != NULL)
            setVisibility(_dataSet->getVisibility());
    }
}

void Image::update()
{
    if (_dataSet == NULL)
        return;

    TRACE("DataSet: %s", _dataSet->getName().c_str());

    vtkDataSet *ds = _dataSet->getVtkDataSet();
    vtkImageData *imageData = vtkImageData::SafeDownCast(ds);

    if (imageData == NULL) {
        ERROR("DataSet is not an image.");
        return;
    }

    initProp();

    vtkImageActor *actor = getImageActor();
    vtkImageMapper3D *mapper = getImageMapper();
    if (mapper == NULL) {
        TRACE("Creating mapper");
        vtkSmartPointer<vtkImageResliceMapper> newMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
        newMapper->AutoAdjustImageQualityOff();
        getImageSlice()->SetMapper(newMapper);
        mapper = getImageMapper();
        assert(mapper != NULL);
    }
    if (actor != NULL) {
        TRACE("Have actor");
        actor->SetInputData(imageData);
        actor->InterpolateOn();
    } else {
        TRACE("No actor");
        mapper->SetInputData(imageData);
    }

    mapper->SliceAtFocalPointOn();
    mapper->SliceFacesCameraOn();

    vtkImageResliceMapper *resliceMapper = getImageResliceMapper();
    if (resliceMapper) {
        TRACE("Mapper is a vtkImageResliceMapper");
        resliceMapper->AutoAdjustImageQualityOff();
        resliceMapper->ResampleToScreenPixelsOff();
    } else {
        TRACE("Mapper is a %s", mapper->GetClassName());
    }

    mapper->Update();

    TRACE("Window: %g Level: %g", getWindow(), getLevel());
}

void Image::setColor(float color[3])
{
    GraphicsObject::setColor(color);

    if (getImageProperty() != NULL) {
        getImageProperty()->SetBackingColor(color[0], color[1], color[2]);
    }
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Image::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL) {
        _colorMap = NULL;
        _lut = NULL;
        if (getImageProperty() != NULL) {
            getImageProperty()->SetLookupTable(NULL);
        }
        return;
    }

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        if (getImageProperty() != NULL) {
            getImageProperty()->UseLookupTableScalarRangeOn();
            getImageProperty()->SetLookupTable(_lut);
        }
        _lut->DeepCopy(cmap->getLookupTable());
        _lut->SetRange(_dataRange);
    } else {
        double range[2];
        _lut->GetTableRange(range);
        _lut->DeepCopy(cmap->getLookupTable());
        _lut->SetRange(range);
        _lut->Modified();
    }
}

void Image::setAspect(double aspect)
{
    double bounds[6];
    vtkDataSet *ds = _dataSet->getVtkDataSet();
    ds->GetBounds(bounds);
    double size[3];
    size[0] = bounds[1] - bounds[0];
    size[1] = bounds[3] - bounds[2];
    size[2] = bounds[5] - bounds[4];
    double scale[3];
    scale[0] = scale[1] = scale[2] = 1.;

    vtkImageMapper3D *mapper = getImageMapper();
    if (mapper == NULL)
        return;

    mapper->Update();

    double normal[3];
    mapper->GetSlicePlane()->GetNormal(normal);

    Axis sliceAxis = Z_AXIS;
    if (fabs(normal[0]) == 1.0 &&
        normal[1] == 0.0 &&
        normal[2] == 0.0) {
        sliceAxis = X_AXIS;
    } else if (normal[0] == 0.0 &&
               fabs(normal[1]) == 1.0 &&
               normal[2] == 0.0) {
        sliceAxis = Y_AXIS;
    } else if (normal[0] == 0.0 &&
               normal[1] == 0.0 &&
               fabs(normal[2]) == 1.0) {
        sliceAxis = Z_AXIS;
    } else {
        TRACE("Non orthogonal slice plane, setting native aspect");
        aspect = 0.0;
    }

    if (aspect == 1.0) {
        // Square
        switch (sliceAxis) {
        case X_AXIS: {
            if (size[1] > size[2] && size[2] > 0.0) {
                scale[2] = size[1] / size[2];
            } else if (size[2] > size[1] && size[1] > 0.0) {
                scale[1] = size[2] / size[1];
            }
        }
            break;
        case Y_AXIS: {
            if (size[0] > size[2] && size[2] > 0.0) {
                scale[2] = size[0] / size[2];
            } else if (size[2] > size[0] && size[0] > 0.0) {
                scale[0] = size[2] / size[0];
            }
        }
            break;
        case Z_AXIS: {
            if (size[0] > size[1] && size[1] > 0.0) {
                scale[1] = size[0] / size[1];
            } else if (size[1] > size[0] && size[0] > 0.0) {
                scale[0] = size[1] / size[0];
            }
        }
            break;
        }
    } else if (aspect != 0.0) {
        switch (sliceAxis) {
        case X_AXIS: {
            if (aspect > 1.0) {
                if (size[2] > size[1] && size[1] > 0.0) {
                    scale[1] = (size[2] / aspect) / size[1];
                } else if (size[2] > 0.0) {
                    scale[2] = (size[1] * aspect) / size[2];
                }
            } else {
                if (size[1] > size[2] && size[2] > 0.0) {
                    scale[2] = (size[1] * aspect) / size[2];
                } else if (size[1] > 0.0) {
                    scale[1] = (size[2] / aspect) / size[1];
                }
            }
        }
            break;
        case Y_AXIS: {
            if (aspect > 1.0) {
                if (size[0] > size[2] && size[2] > 0.0) {
                    scale[2] = (size[0] / aspect) / size[2];
                } else if (size[0] > 0.0) {
                    scale[0] = (size[2] * aspect) / size[0];
                }
            } else {
                if (size[2] > size[0] && size[0] > 0.0) {
                    scale[0] = (size[2] * aspect) / size[0];
                } else if (size[2] > 0.0) {
                    scale[2] = (size[0] / aspect) / size[2];
                }
            }
        }
            break;
        case Z_AXIS: {
            if (aspect > 1.0) {
                if (size[0] > size[1] && size[1] > 0.0) {
                    scale[1] = (size[0] / aspect) / size[1];
                } else if (size[0] > 0.0) {
                    scale[0] = (size[1] * aspect) / size[0];
                }
            } else {
                if (size[1] > size[0] && size[0] > 0.0) {
                    scale[0] = (size[1] * aspect) / size[0];
                } else if (size[1] > 0.0) {
                    scale[1] = (size[0] / aspect) / size[1];
                }
            }
        }
            break;
        }
    }

    TRACE("%s dims %g,%g,%g", _dataSet->getName().c_str(),
          size[0], size[1], size[2]);
    TRACE("Setting scale to %g,%g,%g", scale[0], scale[1], scale[2]);
    setScale(scale);
    mapper->Modified();
    mapper->Update();
}

void Image::setClippingPlanes(vtkPlaneCollection *planes)
{
    vtkImageMapper3D *mapper = getImageMapper();
    if (mapper != NULL) {
        mapper->SetClippingPlanes(planes);
    }
}
