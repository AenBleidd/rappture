/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#ifndef IPHONE

#ifdef WIN32
#include <pthread/pthread.h>
#else
#include <pthread.h>
#endif

#include <vrutil/vrUtil.h>

class VrUtilExport vrThread {
	pthread_t _thread;
	pthread_attr_t _attr;
	int _threadID;

public :
	vrThread();
	virtual ~vrThread();

private :
	static void* threadMain(void* data);
public :
	void initialize();

public :
	virtual void run();
};

#endif

