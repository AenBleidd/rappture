/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
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

using namespace GeoVis;

ResponseQueue::ResponseQueue()
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
    TRACE("Before # of elements is %d", _list.size());
    for (std::list<Response *>::iterator itr = _list.begin();
         itr != _list.end();) {
        /* Remove any duplicate image or legend responses. There should be no 
         * more than one.  Note that if the client starts using multiple legends 
         * for different fields, this optimization should be disabled for legends.
         * We may also need to differentiate between screen images and "hardcopy"
         * images.
         */
        if ((response->type() == Response::IMAGE ||
             response->type() == Response::LEGEND) &&
            (*itr)->type() == response->type()) {
            TRACE("Removing duplicate response of type: %d", (*itr)->type());
            delete *itr;
            itr = _list.erase(itr);
        } else {
            TRACE("Found queued response of type %d", (*itr)->type());
            ++itr;
        }
    }
    /* Add the new response to the end of the list. */
    _list.push_back(response);
    TRACE("After # of elements is %d", _list.size());
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

    int ret;
    while ((ret = sem_wait(&_ready)) < 0 && errno == EINTR)
        continue;
    if (ret < 0) {
        ERROR("can't wait on semaphore: %s", strerror(errno));
    }
    if (pthread_mutex_lock(&_idle) != 0) {
        ERROR("can't lock mutex: %s", strerror(errno));
    }
    /* List may be empty if semaphore value was incremented beyond list
     * size in the case of an enqueue operation deleting from the queue
     * This is not an error, so just unlock and return to wait loop
     */
    if (_list.empty()) {
        TRACE("Empty queue");
    } else {
        response = _list.front();
        _list.pop_front();
        TRACE("Dequeued response of type %d", response->type());
    }
    if (pthread_mutex_unlock(&_idle) != 0) {
        ERROR("can't unlock mutex: %s", strerror(errno));
    }	
    return response;
}
