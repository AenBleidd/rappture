/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <string.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>

#include <vrmath/Vector3f.h>

#include "VolumeInterpolator.h"
#include "Volume.h"
#include "Trace.h"

using namespace vrmath;

VolumeInterpolator::VolumeInterpolator() : 
    _volume(0), 
    _interval(8.0), 
    _started(false), 
    _numBytes(0), 
    _dataCount(0), 
    _numComponents(0)
{
}

void VolumeInterpolator::start()
{
    if (_volumes.size() > 0) {
        TRACE("Volume Interpolation Started");
        _started = true;
    } else {
        TRACE("Volume Interpolation NOT Started");
        _started = false;
    }
    struct timeval clock;
    gettimeofday(&clock, NULL);
    _startTime = clock.tv_sec + clock.tv_usec/1000000.0;
    TRACE("Leave");
}

void VolumeInterpolator::stop()
{
    _started = false;
}

Volume *VolumeInterpolator::update(float fraction)
{
    int key1, key2;
    float interp;

    computeKeys(fraction, _volumes.size(), &interp, &key1, &key2);

    if (interp == 0.0f) {
        memcpy(_volume->data(), _volumes[key1]->data(), _numBytes);
        _volume->tex()->update(_volume->data());
    } else {
        float *data1 = _volumes[key1]->data();
        float *data2 = _volumes[key2]->data();
        float *result = _volume->data();

        Vector3f normal1, normal2, normal;
        for (unsigned int i = 0; i < _dataCount; ++i) {
            *result = interp * (*data2 - *data1) + *data1;
            normal1 = (*(Vector3f*)(data1 + 1) - 0.5) * 2;
            normal2 = (*(Vector3f*)(data2 + 1) - 0.5) * 2;
            normal = (normal2 - normal2) * interp + normal1;
            normal = normal.normalize();
            normal = normal * 0.5 + 0.5;
            *((Vector3f*)(result + 1)) = normal;

            result += _numComponents;
            data1 += _numComponents;
            data2 += _numComponents;
        }

        _volume->tex()->update(_volume->data());
    }

    return _volume;
}

void
VolumeInterpolator::computeKeys(float fraction, int count, float *interp, 
                                int *key1, int *key2)
{
    int limit = (int) count - 1;
    if (fraction <= 0) {
        *key1 = *key2 = 0;
        *interp = 0.0f;
    } else if (fraction >= 1.0f) {
        *key1 = *key2 = limit;
        *interp = 0.0f;
    } else {
        int n;
        for (n = 0; n < limit; n++){
            if (fraction >= (n / (count - 1.0f)) && 
                fraction < ((n+1)/(count-1.0f))) {
                break;
            }
        }

        TRACE("n = %d count = %d", n, count);
        if (n >= limit) {
            *key1 = *key2 = limit;
            *interp = 0.0f;
        } else {
            *key1 = n;
            *key2 = n+1;
            *interp = (fraction - (n / (count -1.0f))) / ((n + 1) / (count - 1.0f) - n / (count - 1.0f));
            //*ret = inter * (keyValue[n + 1] - keyValue[n]) + keyValue[n];
        }
    }
}

void
VolumeInterpolator::clearAll()
{
    _volumes.clear();
}

void
VolumeInterpolator::addVolume(Volume *volume)
{
    if (_volumes.size() != 0) {
        if (_volumes[0]->width() != volume->width() || 
            _volumes[0]->height() != volume->height() ||   
            _volumes[0]->depth() != volume->depth() || 
            _volumes[0]->numComponents() != volume->numComponents()) {
            TRACE("The volume should be the same width, height, number of components");
            return;
        }
    } else {
        _dataCount = volume->width() * volume->height() * volume->depth();
        _numComponents = volume->numComponents();
        _numBytes = _dataCount * _numComponents * sizeof(float);
        Vector3f loc = volume->location();
        _volume = new Volume(loc.x, loc.y, loc.z,
                             volume->width(),
                             volume->height(),
                             volume->depth(), 
                             volume->numComponents(), 
                             volume->data(), 
                             volume->wAxis.min(), 
                             volume->wAxis.max(), 
                             volume->nonZeroMin());

        _volume->numSlices(256-1);
        _volume->disableCutplane(0);
        _volume->disableCutplane(1);
        _volume->disableCutplane(2);
        _volume->visible(true);
        _volume->dataEnabled(true);
        _volume->ambient(volume->ambient());
        _volume->diffuse(volume->diffuse());
        _volume->specularLevel(volume->specularLevel());
        _volume->specularExponent(volume->specularExponent());
        _volume->opacityScale(volume->opacityScale());
        _volume->isosurface(0);
        TRACE("VOL : location %f %f %f\n\tid : %s", loc.x, loc.y, loc.z, 
               volume->name());
    }
    _volumes.push_back(_volume);
    TRACE("a Volume[%s] is added to VolumeInterpolator", volume->name());
}

Volume *VolumeInterpolator::getVolume()
{
    return _volume;
    //return _volumes[0];
}
