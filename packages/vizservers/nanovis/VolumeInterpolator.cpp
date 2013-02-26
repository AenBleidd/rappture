/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <string.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>

#include "VolumeInterpolator.h"
#include "Volume.h"
#include "Vector3.h"
#include "Trace.h"

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
        TRACE("\tVolume Interpolation Started\n");
        _started = true;
    } else {
        TRACE("\tVolume Interpolation did not get started\n");
        _started = false;
    }
    struct timeval clock;
    gettimeofday(&clock, NULL);
    _startTime = clock.tv_sec + clock.tv_usec/1000000.0;
    TRACE("End Start - VolumeInterpolator\n");
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

        Vector3 normal1, normal2, normal;
        for (unsigned int i = 0; i < _dataCount; ++i) {
            *result = interp * (*data2 - *data1) + *data1;
            normal1 = (*(Vector3*)(data1 + 1) - 0.5) * 2;
            normal2 = (*(Vector3*)(data2 + 1) - 0.5) * 2;
            normal = (normal2 - normal2) * interp + normal1;
            normal = normal.normalize();
            normal = normal * 0.5 + 0.5;
            *((Vector3*)(result + 1)) = normal;

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

        TRACE("n = %d count = %d\n", n, count);
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
VolumeInterpolator::addVolume(Volume *refPtr)
{
    if (_volumes.size() != 0) {
        if (_volumes[0]->width() != refPtr->width() || 
            _volumes[0]->height() != refPtr->height() ||   
            _volumes[0]->depth() != refPtr->depth() || 
            _volumes[0]->numComponents() != refPtr->numComponents()) {
            TRACE("The volume should be the same width, height, number of components\n");
            return;
        }
    } else {
        _dataCount = refPtr->width() * refPtr->height() * refPtr->depth();
        _numComponents = refPtr->numComponents();
        _numBytes = _dataCount * _numComponents * sizeof(float);
        Vector3 loc = refPtr->location();
        _volume = new Volume(loc.x, loc.y, loc.z,
                             refPtr->width(),
                             refPtr->height(),
                             refPtr->depth(), 
                             refPtr->numComponents(), 
                             refPtr->data(), 
                             refPtr->wAxis.min(), 
                             refPtr->wAxis.max(), 
                             refPtr->nonZeroMin());

        _volume->numSlices(256-1);
        _volume->disableCutplane(0);
        _volume->disableCutplane(1);
        _volume->disableCutplane(2);
        _volume->visible(true);
        _volume->dataEnabled(true);
        _volume->ambient(refPtr->ambient());
        _volume->diffuse(refPtr->diffuse());
        _volume->specularLevel(refPtr->specularLevel());
        _volume->specularExponent(refPtr->specularExponent());
        _volume->opacityScale(refPtr->opacityScale());
        _volume->isosurface(0);
        TRACE("VOL : location %f %f %f\n\tid : %s\n", loc.x, loc.y, loc.z, 
               refPtr->name());
    }
    _volumes.push_back(_volume);
    TRACE("a Volume[%s] is added to VolumeInterpolator\n", refPtr->name());
}

Volume *VolumeInterpolator::getVolume()
{
    return _volume;
    //return _volumes[0];
}
