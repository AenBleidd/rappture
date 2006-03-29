/*
 * ----------------------------------------------------------------------
 *  EventPlayer.cpp: client that plays back events from file
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

#include <GL/freeglut.h>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <time.h>
#include <iostream>
#include <assert.h>

#include "clientlib.h"
#include "../Event.h"

using namespace std;

Event* event[5000];
int cur_event = 0;

int width = 512;
int height = 512;
char* screen_buffer = new char[width*height*3];
int screen_size = width * height;

int socket_fd; 	//socekt file descriptor


void display()
{
   //paste to screen 
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, screen_buffer);	
   glFlush();
   glutSwapBuffers();
   return;
}


void key(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'q':
          exit(0);
          break;
	default:
	  return;
    }
}

void idle(void)
{
  //send requests
  Event* cur = event[cur_event];
  std::stringstream msgstream;
  std::string msg;

  switch(cur->type){
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

  //start timer
  struct timeval clock;
  time((long*) &clock);
  long start_time = clock.tv_sec*1000 + clock.tv_usec/1000;

  //send msg
  int status = write(socket_fd, msg.c_str(), strlen(msg.c_str()));
  if (status <= 0) {
     perror("socket write");
     return;
  }
  
  //receive screen update
  int len = width * height;
  int remain = len;
  char* ptr = screen_buffer;
  while(remain>0){
    status = read(socket_fd, ptr, remain);
    if(status <= 0) {
      perror("socket read");
      return;
    }
    else {
       remain -= status;
       ptr += status;
    }
  }


  //end timer
  time((long*) &clock);
  long end_time = clock.tv_sec*1000 + clock.tv_usec/1000;

  long time_interval = end_time - start_time;

  fprintf(stderr, "%d ", time_interval);
}

int init_client(char* host, char* port, char* file){

  //load the event file
  FILE* fd = fopen(file, "r");

  //load 5000 events
  for(int i=0; i<5000; i++){
    int type;
    float param[3];
    fscanf(fd, "%d %f %f %f\n", &type, param, param+1, param+2);
    event[i] = new Event(type, param, 0);
    fprintf(stderr, "%d %f %f %f\n", type, param[0], param[1], param[2]);
  }
  fclose(fd);

  socket_fd = connect_renderer(host, atoi(port), 100);
  if (socket_fd == -1) {
     fprintf(stderr, "Could not connect to a server.\n");
     return 1;
  }

  return 0;
}

void help(const char *argv0)
{
  fprintf(stderr,
          "Syntax: %s addr port eventfile\n",
          argv0);
  exit(1);
}

/* Program entry point */
int main(int argc, char *argv[])
{
    //parameters: host and port and event file
    if(argc!=4) help(argv[0]);

    assert(init_client(argv[1], argv[2], argv[3])==0);

    glutInit(&argc, argv);
    glutInitWindowSize(width,height);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

    glutCreateWindow("Client");

    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutIdleFunc(idle);

    glClearColor(0.,0.,0.,0.);
    glutMainLoop();

    return 0;
}

