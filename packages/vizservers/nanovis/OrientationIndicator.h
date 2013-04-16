/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */
#ifndef NV_ORIENTATION_INDICATOR_H
#define NV_ORIENTATION_INDICATOR_H

#include <vrmath/Vector3f.h>

namespace nv {

class OrientationIndicator
{
public:
    enum Representation {
        LINES,
        ARROWS
    };

    OrientationIndicator();
    virtual ~OrientationIndicator();

    void setRepresentation(Representation rep);

    void render();

    bool isVisible() const
    {
        return _visible;
    }

    void setVisible(bool state)
    {
        _visible = state;
    }

    void setPosition(const vrmath::Vector3f& pos)
    {
        _position = pos;
    }

    void setScale(const vrmath::Vector3f& scale)
    {
        _scale = scale;
    }

private:
    Representation _rep;
    bool _visible;
    float _lineWidth;
    void *_quadric;

    vrmath::Vector3f _position;
    vrmath::Vector3f _scale;
};

}

#endif
