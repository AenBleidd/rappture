/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageActor.h>
#include <vtkImageProperty.h>
#include <vtkImageMapper3D.h>
#include <vtkLookupTable.h>

#include "Image.h"
#include "Trace.h"

using namespace VtkVis;

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
        _prop = vtkSmartPointer<vtkImageActor>::New();
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
    actor->SetInputData(imageData);
    actor->InterpolateOn();

    vtkImageMapper3D *mapper = getImageMapper();
    mapper->Update();
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

void Image::setClippingPlanes(vtkPlaneCollection *planes)
{
    vtkImageMapper3D *mapper = getImageMapper();
    if (mapper != NULL) {
        mapper->SetClippingPlanes(planes);
    }
}
