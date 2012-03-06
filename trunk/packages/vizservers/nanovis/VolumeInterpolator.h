/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __VOLUME_INTERPOLATOR_H__
#define __VOLUME_INTERPOLATOR_H__

#include "Volume.h"

#include <vector>

class VolumeInterpolator {
    Volume* _volume;

    std::vector<Volume*> _volumes;

    double _interval;
    bool _started;
    unsigned int _numBytes;
    unsigned int _dataCount;
    unsigned int _n_components;
    unsigned int _referenceOfVolume;
    double _start_time;

public :
    VolumeInterpolator();
    void addVolume(Volume* vol);
    void clearAll();
    
    void start();
    Volume* update(float fraction);
    void stop();
    void computeKeys(float fraction, int count, float* interp, int* key1, int* key2);
    bool is_started() const;
    double getInterval() const;
    double getStartTime() const;
    unsigned int getReferenceVolumeID() const;
    Volume* getVolume();
};

inline bool VolumeInterpolator::is_started() const
{
    return _started;
}

inline double VolumeInterpolator::getStartTime() const
{
    return _start_time;
}

inline double VolumeInterpolator::getInterval() const
{
    return _interval;
}

inline unsigned int VolumeInterpolator::getReferenceVolumeID() const
{
    return _referenceOfVolume;
}

//inline Volume* VolumeInterpolator::getVolume()
//{
//    return _volume;
//    //return _volumes[0];
//}
#endif

