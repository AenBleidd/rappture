/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */
#ifndef ORIENTATION_INDICATOR_H
#define ORIENTATION_INDICATOR_H

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

private:
    Representation _rep;
    bool _visible;
    float _lineWidth;
    void *_quadric;
};
}

#endif
