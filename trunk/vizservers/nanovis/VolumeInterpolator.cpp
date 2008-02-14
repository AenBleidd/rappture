#include "VolumeInterpolator.h"
#include "Volume.h"
#include <string.h>
#include <memory.h>
#include "Vector3.h"
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include "Trace.h"

VolumeInterpolator::VolumeInterpolator()
: _started(false), _volume(0), _numBytes(0), _interval(8.0), _dataCount(0), _n_components(0), _referenceOfVolume(0)
{
}

void VolumeInterpolator::start()
{
    Trace("Begin Start - VolumeInterpolator\n");
    if (_volumes.size() != 0)
    {
        Trace("\tStarted\n");
	    _started = true;
    }
    else
    {
        Trace("\tnot started\n");
        _started = false;
    }
  
    struct timeval clock;
    gettimeofday(&clock, NULL);
    _start_time = clock.tv_sec + clock.tv_usec/1000000.0;

    Trace("End Start - VolumeInterpolator\n");
}

void VolumeInterpolator::stop()
{
	_started = false;
}

Volume* VolumeInterpolator::update(float fraction)
{
    int key1, key2;
    float interp;

/*
    computeKeys(fraction, _volumes.size(), &interp, &key1, &key2);

    if (interp == 0.0f)
    {
        memcpy(_volume->_data, _volumes[key1]->_data, _numBytes);
        _volume->tex->initialize(_volume->_data);
    }
    else
    {
        float* data1 = _volumes[key1]->_data;
        float* data2 = _volumes[key2]->_data;
        float* result = _volume->_data;

        Vector3 normal;
        for (int i = 0; i < _dataCount; ++i)
        {
            *result = interp * (*data2 - *data1) + *data1;
            normal = (*(Vector3*)(data2 + 1) - *(Vector3*)(data1 + 1)) * interp + *(Vector3*) (data1 + 1);
            *((Vector3*)(result + 1)) = normal.normalize();

            result += _n_components;
            data1 += _n_components;
            data2 += _n_components;
        }

        _volume->tex->initialize(_volume->_data);
    }
    //Trace("End of Update - VolumeInterpolator");
    return _volume;

*/
    //memcpy(_volume->_data, _volumes[0]->_data, _numBytes);
    //_volume->id = _volume->tex->initialize(_volumes[0]->_data);
    //_volume->tex->update(_volumes[0]->_data);
    return _volume;
}

void VolumeInterpolator::computeKeys(float fraction, int count, float* interp, int* key1, int* key2)
{
	int limit = (int) count - 1;
	if (fraction <= 0)
    {
	    *key1 = *key2 = 0;
        *interp = 0.0f;
    }
    else if (fraction >= 1.0f)
    {
	    *key1 = *key2 = limit;
        *interp = 0.0f;
	}
    else
    {
        int n;
        for (n = 0;n < limit; n++){
            if (fraction >= (n / (count - 1.0f)) && fraction < ((n+1)/(count-1.0f))) break;
        }

        if (n >= limit){
	        *key1 = *key2 = limit;
            *interp = 0.0f;

        }
        else {
	        *key1 = n;
            *key2 = n+1;
	        *interp = (fraction - (n / (count -1.0f))) / ((n + 1) / (count - 1.0f) - n / (count - 1.0f));
            //*ret = inter * (keyValue[n + 1] - keyValue[n]) + keyValue[n];
        }
    }
}

void VolumeInterpolator::clearAll()
{
	_volumes.clear();

    if (_volume) delete _volume;
}

void VolumeInterpolator::addVolume(Volume* volume, unsigned int volumeId)
{
    if (_volumes.size() != 0)
    {
        if (_volumes[0]->width != volume->width || _volumes[0]->height != volume->height ||   
            _volumes[0]->depth != volume->depth || _volumes[0]->n_components != volume->n_components)
        {
            printf("The volume should be the same width, height, number of components\n");
            return;
        }

    }
    else
    {
        _dataCount = volume->width * volume->height * volume->depth * volume->n_components;
        _n_components = volume->n_components;
        _numBytes = _dataCount * sizeof(float);
        _volume = new Volume(volume->location.x, volume->location.y, volume->location.z,
                        volume->width, volume->height, volume->depth, volume->size,
                        volume->n_components, volume->_data, volume->vmin, volume->vmax, volume->nonzero_min);
        _referenceOfVolume = volumeId;
        _volume->set_n_slice(256-volumeId);
        _volume->disable_cutplane(0);
        _volume->disable_cutplane(1);
        _volume->disable_cutplane(2);
        _volume->enable();
        _volume->enable_data();
        _volume->set_specular(volume->get_specular());
        _volume->set_diffuse(volume->get_diffuse());
        _volume->set_opacity_scale(volume->get_opacity_scale());
        _volume->set_isosurface(1);

        Trace("VOL : location %f %f %f\n\tid : %d\n", _volume->location.x, _volume->location.y, _volume->location.z,
                                        _volume->id);
    }

	_volumes.push_back(volume);

    Trace("a Volume[%d] is added to VolumeInterpolator\n");
}

