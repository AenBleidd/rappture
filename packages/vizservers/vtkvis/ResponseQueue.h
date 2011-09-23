/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#include <pthread.h>
#include <semaphore.h>
#include <cstdlib>
#include <list>

#ifndef _RESPONSE_QUEUE_H
#define _RESPONSE_QUEUE_H

namespace Rappture {
namespace VtkVis {

class Response
{
public:
    enum AllocationType {
        STATIC,
        DYNAMIC,
        VOLATILE
    };
    enum ResponseType {
        IMAGE,          /**< Image to be displayed. */
        LEGEND,         /**< Legend to be displayed. */
        DATA            /**< Any other type of message. */
    };

    Response(ResponseType type) :
        _mesg(NULL),
        _length(0),
        _type(type)
    {
    }

    virtual ~Response()
    {
	if (_length > 0) {
	    if (_allocType == DYNAMIC) {
		free(_mesg);
	    }
	}
    }

    ResponseType type()
    {
	return _type;
    }

    unsigned char *message()
    {
	return _mesg;
    }

    size_t length()
    {
	return _length;
    }

    void setMessage(unsigned char *mesg, size_t length, AllocationType type)
    {
	if (type == VOLATILE) {
	    _length = length;
	    _mesg = (unsigned char *)malloc(length);
	    memcpy(_mesg, mesg, length);
	    _allocType = DYNAMIC;
	} else {
	    _length = length;
	    _mesg = mesg;
	    _allocType = type;
	}
    }

private:
    /** 
     * (Malloc-ed by caller, freed by destructor)
     * Contains the message/bytes to be sent to the client. */
    unsigned char *_mesg;
    size_t _length;       /**< # of bytes in the above message. */
    ResponseType _type;
    AllocationType _allocType;
};

class ResponseQueue
{
public:
    ResponseQueue(void *clientData);

    virtual ~ResponseQueue();

    void *clientData()
    {
	return _clientData;
    }

    void enqueue(Response *response);

    Response *dequeue();

private:
    pthread_mutex_t _idle;
    sem_t _ready; /**< Semaphore indicating that a response has been queued. */
    std::list<Response *> _list;
    void *_clientData;
};

}
}

#endif
