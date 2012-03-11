/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRTHREAD_H
#define VRTHREAD_H

#include <pthread.h>

#include <vrutil/vrUtil.h>

class vrThread
{
public:
    vrThread();

    virtual ~vrThread();

    void initialize();

    virtual void run();

private:
    static void *threadMain(void *data);

    pthread_t _thread;
    pthread_attr_t _attr;
    int _threadID;
};

#endif

