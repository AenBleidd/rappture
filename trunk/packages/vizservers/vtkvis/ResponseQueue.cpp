/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#include <pthread.h>
#include <semaphore.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <list>

#include "Trace.h"
#include "ResponseQueue.h"

using namespace Rappture::VtkVis;

ResponseQueue::ResponseQueue(void *clientData)  :
    _clientData(clientData)
{
    pthread_mutex_init(&_idle, NULL);
    if (sem_init(&_ready, 0, 0) < 0) {
        ERROR("can't initialize semaphore: %s", strerror(errno));
    }
}

ResponseQueue::~ResponseQueue() 
{
    TRACE("Deleting ResponseQueue");
    pthread_mutex_destroy(&_idle);
    if (sem_destroy(&_ready) < 0) {
        ERROR("can't destroy semaphore: %s", strerror(errno));
    }
    for (std::list<Response *>::iterator itr = _list.begin();
         itr != _list.end(); ++itr) {
        delete *itr;
    }
    _list.clear();
}

void
ResponseQueue::enqueue(Response *response)
{
    if (pthread_mutex_lock(&_idle) != 0) {
        ERROR("can't lock mutex: %s", strerror(errno));
    }	
    /* Examine the list and remove any queued responses of the same type. */
    TRACE("before # of elements is %d\n", _list.size());
    for (std::list<Response *>::iterator itr = _list.begin();
         itr != _list.end(); ++itr) {
        /* Remove any responses of the same type. There should be no more than
         * one. */
        if ((*itr)->type() == response->type()) {
            _list.erase(itr);
        }
    }
    /* Add the new response to the end of the list. */
    _list.push_back(response);
    if (sem_post(&_ready) < 0) {
        ERROR("can't post semaphore: %s", strerror(errno));
    }
    if (pthread_mutex_unlock(&_idle) != 0) {
        ERROR("can't unlock mutex: %s", strerror(errno));
    }	
}

Response *
ResponseQueue::dequeue()
{
    Response *response = NULL;

    if (sem_wait(&_ready) < 0) {
        ERROR("can't wait on semaphore: %s", strerror(errno));
    }
    if (pthread_mutex_lock(&_idle) != 0) {
        ERROR("can't lock mutex: %s", strerror(errno));
    }
    if (_list.empty()) {
        ERROR("Empty queue");
    } else {
        response = _list.front();
        _list.pop_front();
    }
    if (pthread_mutex_unlock(&_idle) != 0) {
        ERROR("can't unlock mutex: %s", strerror(errno));
    }	
    return response;
}
