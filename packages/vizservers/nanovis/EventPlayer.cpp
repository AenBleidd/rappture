/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 *  EventPlayer.cpp: client that plays back events from file
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>

#include "clientlib.h"
#include "Event.h"

Event* event[300];
int cur_event = 0;
double interval_sum = 0;

int width = 512;
int height = 512;
char screen_buffer[512*512*3];
int screen_size = width * height;

int socket_fd; 	//socekt file descriptor

/*
void display()
{
    //paste to screen 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, screen_buffer);	
    glFlush();
    glutSwapBuffers();
    return;
}
*/

void key(unsigned char key, int x, int y)
{
    switch (key) {
    case 'q':
        exit(0);
        break;
    default:
        return;
    }
}

/*
 *Client communicates with the server
 */
void idle(void)
{
    //send requests

    if (cur_event >= (int)(sizeof(event)/sizeof(event[0]))) {
        float ave = interval_sum / (cur_event-1);
        float fps = 1/ave;
        TRACE("Average frame time = %.6f\n", ave);
        TRACE("Frames per second  = %f\n", fps);
        exit(0);
    }

    Event *cur = event[cur_event];
    std::stringstream msgstream;
    std::string msg;

    switch (cur->type){
    case 0: //rotate
        msgstream << "camera " << cur->parameter[0] << " " 
                  << cur->parameter[1] << " " 
                  << cur->parameter[2] << " " << endl;
        break;
    case 1: //move
        msgstream << "move " << cur->parameter[0] << " " 
                  << cur->parameter[1] << " " 
                  << cur->parameter[2] << " " << endl;
        break;
    case 2: //other
        msgstream << "refresh " << cur->parameter[0] << " " 
                  << cur->parameter[1] << " " 
                  << cur->parameter[2] << " " << endl;
        break;
    default:
        return;
    }

    msg = msgstream.str();

    //sleep a little
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = cur->msec*1000000000;
    nanosleep(&ts, 0);
  
    //start timer
    struct timeval clock;
    gettimeofday(&clock, NULL);
    double start_time = clock.tv_sec + clock.tv_usec/1000000.0;

    //send msg
    //TRACE("Writing message %04d to server: '%s'\n", cur_event, msg.c_str());
    int status = write(socket_fd, msg.c_str(), strlen(msg.c_str()));
    if (status <= 0) {
        perror("socket write");
        return;
    }

#if DO_RLE
    int sizes[2];
    status = read(socket_fd, &sizes, sizeof(sizes));
    TRACE("Reading %d,%d bytes\n", sizes[0], sizes[1]);
    int len = sizes[0] + sizes[1];
#else
    int len = width * height * 3;
#endif

    //receive screen update
    int remain = len;
    char* ptr = screen_buffer;
    while (remain > 0) {
        status = read(socket_fd, ptr, remain);
        if (status <= 0) {
            perror("socket read");
            return;
        } else {
            remain -= status;
            ptr += status;
        }
    }

    //TRACE("Read message to server.\n");

    //end timer
    gettimeofday(&clock, NULL);
    double end_time = clock.tv_sec + clock.tv_usec/1000000.0;

    double time_interval = end_time - start_time;

    if (cur_event != 0) {
        interval_sum += time_interval;
    }
    cur_event++;

    TRACE("% 6d %.6f\n", len, time_interval);
}

int init_client(char* host, char* port, char* file){

    //load the event file
    FILE* fd = fopen(file, "r");

    //load events
    for (int i = 0; i < (int)(sizeof(event)/sizeof(event[0])); i++) {
        int type;
        float param[3];
        double interval;
        fscanf(fd, "%d %f %f %f %lf\n", &type, param, param+1, param+2, &interval);
        event[i] = new Event(type, param, interval);
        TRACE("%d %f %f %f\n", type, param[0], param[1], param[2]);
    }
    fclose(fd);

    socket_fd = connect_renderer(host, atoi(port), 100);
    if (socket_fd == -1) {
        ERROR("Could not connect to a server.\n");
        return 1;
    }

    return 0;
}

void 
help(const char *argv0)
{
    TRACE("Syntax: %s addr port eventfile\n", argv0);
    exit(1);
}

/* Program entry point */
int 
main(int argc, char *argv[])
{
    //parameters: host and port and event file
    if (argc != 4)
        help(argv[0]);

    assert(init_client(argv[1], argv[2], argv[3])==0);

    /*
      glutInit(&argc, argv);
      glutInitWindowSize(width,height);
      glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

      glutCreateWindow("Client");

      glutDisplayFunc(display);
      glutKeyboardFunc(key);
      glutIdleFunc(idle);

      glClearColor(0.,0.,0.,0.);
      glutMainLoop();
    */
    while (1)
        idle();

    return 0;
}
