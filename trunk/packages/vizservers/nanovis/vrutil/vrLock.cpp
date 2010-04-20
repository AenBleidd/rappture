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

