/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkSmartPointer.h>
#include <vtkTextActor3D.h>
#include <vtkTextProperty.h>
#include <vtkProp3DFollower.h>
#include <vtkRenderer.h>

#include "Text3D.h"
#include "Trace.h"

using namespace VtkVis;

Text3D::Text3D() :
    GraphicsObject()
{
}

Text3D::~Text3D()
{
    TRACE("Deleting Text3D");
}

void Text3D::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkTextActor3D>::New();
        vtkTextProperty *property = getTextProperty();
        property->SetColor(_color[0], _color[1], _color[2]);
        if (_dataSet != NULL)
            _opacity = _dataSet->getOpacity();
        property->SetOpacity(_opacity);

        if (_dataSet != NULL)
            setVisibility(_dataSet->getVisibility());
    }
}

void Text3D::update()
{
    initProp();
}

void Text3D::setOpacity(double opacity)
{
    GraphicsObject::setOpacity(opacity);

    vtkTextProperty *property = getTextProperty();
    if (property == NULL)
        return;

    property->SetOpacity(_opacity);
}

void Text3D::setColor(float color[3])
{
    GraphicsObject::setColor(color);

    vtkTextProperty *property = getTextProperty();
    if (property == NULL)
        return;

    property->SetColor(_color[0], _color[1], _color[2]);
}

void Text3D::setFont(const char *fontName)
{
    vtkTextProperty *property = getTextProperty();
    if (property == NULL)
        return;

    property->SetFontFamilyAsString(fontName);
}

void Text3D::setFontSize(int size)
{
    vtkTextProperty *property = getTextProperty();
    if (property == NULL)
        return;

    property->SetFontSize(size);
}

void Text3D::setBold(bool state)
{
    vtkTextProperty *property = getTextProperty();
    if (property == NULL)
        return;

    property->SetBold((state ? 1 : 0));
}

void Text3D::setItalic(bool state)
{
    vtkTextProperty *property = getTextProperty();
    if (property == NULL)
        return;

    property->SetItalic((state ? 1 : 0));
}

void Text3D::setShadow(bool state)
{
    vtkTextProperty *property = getTextProperty();
    if (property == NULL)
        return;

    property->SetShadow((state ? 1 : 0));
}

void Text3D::setFollowCamera(bool state, vtkRenderer *renderer)
{
    if (state && vtkTextActor3D::SafeDownCast(_prop) != NULL) {
        vtkSmartPointer<vtkTextActor3D> textActor = vtkTextActor3D::SafeDownCast(_prop);
        vtkSmartPointer<vtkProp3DFollower> follower = vtkSmartPointer<vtkProp3DFollower>::New();
        follower->SetCamera(renderer->GetActiveCamera());
        follower->SetProp3D(textActor);
        renderer->RemoveViewProp(textActor);
        _prop = follower;
        renderer->AddViewProp(_prop);
    } else if (!state && vtkProp3DFollower::SafeDownCast(_prop) != NULL) {
        vtkSmartPointer<vtkProp3DFollower> follower = vtkProp3DFollower::SafeDownCast(_prop);
        vtkSmartPointer<vtkTextActor3D> textActor = vtkTextActor3D::SafeDownCast(follower->GetProp3D());
        renderer->RemoveViewProp(follower);
        _prop = textActor;
        textActor->SetUserMatrix(NULL);
        textActor->SetUserTransform(NULL);
        renderer->AddViewProp(_prop);
    }
}

void Text3D::setClippingPlanes(vtkPlaneCollection *planes)
{
    // FIXME: How to get Mapper?
}
