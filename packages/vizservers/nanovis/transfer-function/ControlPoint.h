/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef CONTROL_POINT_H
#define CONTROL_POINT_H

class ControlPoint
{
public:
    ControlPoint(double _x, double _y);
    virtual ~ControlPoint();

    void Set(double x, double y);

    void glDraw();    //draw a cross

    void glDraw_2();  //draw a filled squre

    void glDraw_3();  //draw a circle

    double x;
    double y;

    bool selected;
    bool dragged;

    ControlPoint * next;
};

#endif
