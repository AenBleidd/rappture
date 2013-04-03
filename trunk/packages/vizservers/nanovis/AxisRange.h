/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors: George A. Howlett <gah@purdue.edu>
 */
#ifndef AXIS_RANGE_H
#define AXIS_RANGE_H

#include <string.h>

namespace nv {

class AxisRange
{
public:
    AxisRange() :
        _min(0.0),
        _max(1.0),
        _units(NULL)
    {
    }

    ~AxisRange()
    {
        if (_units != NULL) {
            delete [] _units;
        }
    }

    void setRange(double min, double max)
    {
        _min = min;
        _max = max;
    }

    double min() const
    {
        return _min;
    }

    void min(double min)
    {
        _min = min;
    }

    double max() const
    {
        return _max;
    }

    void max(double max)
    {
        _max = max;
    }

    double length() const
    {
        return (_max - _min);
    }

    const char *units() const
    {
        return _units;
    }

    void units(const char *units)
    {
        if (_units != NULL) {
            delete [] _units;
        }
        if (units == NULL) {
            _units = NULL;
        } else {
            _units = new char[strlen(units) + 1];
            strcpy(_units, units);
        }
    }

private:
    double _min, _max;
    char *_units;
};

}

#endif
