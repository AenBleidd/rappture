/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Event.h: user event class for statistics purpose
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdio.h>
#include <fstream>

enum EventType {EVENT_ROTATE, EVENT_MOVE, EVENT_OTHER};

class Event{

public:
   int type;
   float parameter[3];	//event parameters: rotate: x,y,z  
			//                  move: x,y,z
  			//                  other: nan 
   float msec;		//millisecond since the session started
	
   Event();
   Event(int _type, float _param[3], float _time);
   ~Event();
   void write(FILE* fd);
};


#endif
