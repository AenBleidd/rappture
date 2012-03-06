/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrutil/vrThread.h>

vrThread::vrThread()
{
}

vrThread::~vrThread()
{
}

void* vrThread::threadMain(void* data)
{
	((vrThread*) data)->run();
	return 0;
}

void vrThread::initialize()
{
	pthread_attr_init(&_attr);

	_threadID = pthread_create(&_thread, &_attr, threadMain, (void *)this); 
}


void vrThread::run()
{
}
