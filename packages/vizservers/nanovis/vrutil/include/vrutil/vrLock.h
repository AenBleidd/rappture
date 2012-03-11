/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRLOCK_H
#define VRLOCK_H

#include <pthread.h>

#include <vrutil/vrUtil.h>

class vrLock
{
public:
    vrLock();
    ~vrLock();

    void lock();
    void unlock();

private:
    pthread_mutex_t _mutex; 
    pthread_mutexattr_t _attr;
};

inline void vrLock::lock()
{
    pthread_mutex_lock(&_mutex);
}

inline void vrLock::unlock()
{
    pthread_mutex_unlock(&_mutex);
}

#endif

