/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_TEXT3D_H
#define VTKVIS_TEXT3D_H

#include <vtkSmartPointer.h>
#include <vtkTextActor3D.h>
#include <vtkTextProperty.h>
#include <vtkProp3DFollower.h>
#include <vtkRenderer.h>

#include "GraphicsObject.h"
#include "DataSet.h"

namespace VtkVis {

/**
 * \brief Text Label with 3D transform
 *
 * This class creates a text label that can be positioned in 3D space
 */
class Text3D : public GraphicsObject
{
public:
    Text3D();
    virtual ~Text3D();

    virtual const char *getClassName() const
    {
        return "Text3D";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer)
    {
        assert(dataSet == NULL);
        update();
    }

    virtual void initProp();

    virtual void setOpacity(double opacity);

    virtual void setColor(float color[3]);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setText(const char *string)
    {
        vtkTextActor3D *textActor = getTextActor();
        if (textActor != NULL) {
            textActor->SetInput(string);
	}
    }

    void setFont(const char *fontName);

    void setFontSize(int size);

    void setBold(bool state);

    void setItalic(bool state);

    void setShadow(bool state);

    void setFollowCamera(bool state, vtkRenderer *renderer);

private:
    virtual void update();

    vtkTextActor3D *getTextActor()
    {
        if (vtkTextActor3D::SafeDownCast(_prop) != NULL) {
            return vtkTextActor3D::SafeDownCast(_prop);
        } else if (vtkProp3DFollower::SafeDownCast(_prop) != NULL) {
            return vtkTextActor3D::SafeDownCast(vtkProp3DFollower::SafeDownCast(_prop)->GetProp3D());
        }
        return NULL;
    }

    vtkTextProperty *getTextProperty()
    {
        vtkTextActor3D *textActor = getTextActor();
        if (textActor == NULL)
            return NULL;
        return textActor->GetTextProperty();
    }
};

}

#endif
