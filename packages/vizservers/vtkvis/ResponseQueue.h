/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
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

/**
 * \brief Holds data for a response to be sent to client
 */
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
	if ((_length > 0) && (_mesg != NULL) && (_allocType == DYNAMIC)) {
            free(_mesg);
	}
    }

    /// Get the ResponseType
    ResponseType type()
    {
	return _type;
    }

    /// Get the Response data
    unsigned char *message()
    {
	return _mesg;
    }

    /// Get the number of bytes in the Response data
    size_t length()
    {
	return _length;
    }

    /// Set the message/data making up the Response
    /**
     * If the AllocationType is DYNAMIC, the message data will be free()d
     * by the destructor.  If the AllocationType is VOLATILE, a copy of 
     * the message data will be made.
     *
     * \param[in] mesg The Response data, can be a command and/or binary data
     * \param[in] length The number of bytes in mesg
     * \param[in] type Specify how the memory was allocated for mesg
     */ 
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

/**
 * \brief Queue to hold pending Responses to be sent to the client
 *
 * A semaphore and mutex are used to control access to the 
 * queue by a reader and writer thread
 */
class ResponseQueue
{
public:
    ResponseQueue(void *clientData);

    virtual ~ResponseQueue();

    /// A place to store a data pointer.  Not used internally.
    /* XXX: This probably doesn't belong here */
    void *clientData()
    {
	return _clientData;
    }

    /// Add a response to the end of the queue
    void enqueue(Response *response);

    /// Remove a response from the front of the queue
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
