/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <vrutil/vrUtil.h>

#ifndef IPHONE

#include <pthread.h>

class VrUtilExport vrLock {
	pthread_mutex_t _mutex; 
	pthread_mutexattr_t _attr;
public :
	vrLock();
	~vrLock();

public :
	void lock();
	void unlock();
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

