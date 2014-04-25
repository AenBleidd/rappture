/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2014  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_MOUSE_COORDS_TOOL_H
#define GEOVIS_MOUSE_COORDS_TOOL_H

#include <osgEarth/MapNode>

#include <osgEarthUtil/Common>
#include <osgEarthUtil/Controls>
#include <osgEarthUtil/Formatter>

#include <osgGA/GUIEventHandler>

#include "Trace.h"

namespace GeoVis {

class MouseCoordsCallback : public osgEarth::Util::MouseCoordsTool::Callback
{
public:
    MouseCoordsCallback(osgEarth::Util::Controls::LabelControl *label,
                        osgEarth::Util::Formatter *formatter) :
        osgEarth::Util::MouseCoordsTool::Callback(),
        _label(label),
        _formatter(formatter),
        _havePoint(false)
    {
        if (dynamic_cast<osgEarth::Util::MGRSFormatter *>(formatter) != NULL) {
            _prefix = "MGRS: ";
        } else {
            _prefix = "Lat/Long: ";
        }
    }

    void set(const osgEarth::GeoPoint& mapCoords, osg::View *view, osgEarth::MapNode *mapNode)
    {
        TRACE("%g %g %g", mapCoords.x(), mapCoords.y(), mapCoords.z());
        if (_label.valid()) {
            _label->setText(osgEarth::Stringify()
                            << _prefix
                            <<  _formatter->format(mapCoords));
                            //<< ", " << mapCoords.z());
        }
        _pt = mapCoords;
        _havePoint = true;
    }

    void reset(osg::View *view, osgEarth::MapNode *mapNode)
    {
        TRACE("Out of range");
        // Out of range of map extents
        if (_label.valid()) {
            _label->setText("");
        }
        _havePoint = false;
    }

    bool report(double *x, double *y, double *z)
    {
        if (_havePoint) {
            *x = _pt.x();
            *y = _pt.y();
            *z = _pt.z();
            _havePoint = false;
            return true;
        }
        return false;
    }

    osgEarth::Util::Controls::LabelControl *getLabel()
    { return _label.get(); }

    osgEarth::Util::Formatter *getFormatter()
    { return _formatter.get(); }

private:
    osg::observer_ptr<osgEarth::Util::Controls::LabelControl> _label;
    osg::ref_ptr<osgEarth::Util::Formatter> _formatter;
    std::string _prefix;
    bool _havePoint;
    osgEarth::GeoPoint _pt;
};

/**
 * Tool that prints the map coordinates under the mouse into a 
 * LabelControl.
 */
class MouseCoordsTool : public osgEarth::Util::MouseCoordsTool
{
public:
    MouseCoordsTool(osgEarth::MapNode* mapNode) :
        osgEarth::Util::MouseCoordsTool(mapNode)
    {}

    virtual ~MouseCoordsTool()
    {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == ea.PUSH ||
            ea.getEventType() == ea.RELEASE ||
            ea.getEventType() == ea.MOVE ||
            ea.getEventType() == ea.DRAG) {
            // mouse Y from OSG is 1 at bottom of window to +height at top of window
            // mouse Y from TK  is 0 at top of window to +(height-1) at bottom of window
            //TRACE("coords: %g,%g", ea.getX(), ea.getY());
            osg::Vec3d world;
            if (_mapNode->getTerrain()->getWorldCoordsUnderMouse(aa.asView(), ea.getX(), ea.getY(), world)) {
                osgEarth::GeoPoint map;
                map.fromWorld(_mapNode->getMapSRS(), world);

                for (osgEarth::Util::MouseCoordsTool::Callbacks::iterator i = _callbacks.begin();
                     i != _callbacks.end(); ++i) {
                    i->get()->set(map, aa.asView(), _mapNode);
                }
            } else {
                for (osgEarth::Util::MouseCoordsTool::Callbacks::iterator i = _callbacks.begin();
                     i != _callbacks.end(); ++i) {
                    i->get()->reset(aa.asView(), _mapNode);
                }
            }
        }
        return false;
    }
};

}

#endif
