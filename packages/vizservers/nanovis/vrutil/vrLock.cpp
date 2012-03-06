/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrutil/vrLock.h>

#ifndef IPHONE
vrLock::vrLock()
{
	pthread_mutex_init(&_mutex, &_attr);
}

vrLock::~vrLock()
{
	pthread_mutex_destroy(&_mutex);
}
#endif

