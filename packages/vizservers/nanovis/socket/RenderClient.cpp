/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ======================================================================
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
#include <sstream>
#include <string.h>
#include <time.h>
#include <iostream>
#include <assert.h>

#include <GL/glut.h>

#include "RenderClient.h"
#include "Event.h"

using namespace std;

Event* event[5000];
int cur_event = 0;

int width, height;

RenderClient::RenderClient()
{
}

RenderClient::RenderClient(std::string& remote_host, int port_num)
{
    //screen_size = sizeof(float)*4*512*512;	//float
    screen_size = 3*width*height;	//unsigned byte
    screen_buffer = new char[screen_size];

    socket_num = port_num;
    host = remote_host;

    //init socket server
    std::cout << "client running....\n";

    try {
        // Create the socket
        client_socket = new ClientSocket(host, socket_num);
        //client_socket->set_non_blocking(true);
        fprintf(stderr, "client socket initialized\n");
    } catch (SocketException& e) {
        std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
    }
}

void RenderClient::send(std::string& msg)
{
    try {
        //printf("client: send()\n");
        *client_socket << msg;
    } catch (SocketException&) {
    }
}

void RenderClient::receive(std::string& msg)
{
    std::string buf="";
    try	{
        fprintf(stderr, "client: receive()\n");
        fflush(stdout);
        *client_socket >> buf;
        msg = buf;
    } catch (SocketException&){
    }
}

bool RenderClient::receive(char* s, int size)
{
    try	{
        //printf("client: receive()\n");
        bool ret = client_socket->recv(s, size);
        return true;
    } catch (SocketException&) {
        return false;
    }
}

bool RenderClient::send(char* s, int size)
{
    try	{
        //printf("client: receive()\n");
        bool ret = client_socket->send(s, size);
        return true;
    } catch (SocketException&) {
        return false;
    }
}

RenderClient* client;

bool loaded = false;

float live_rot_x = 0.;
float live_rot_y = 0.;
float live_rot_z = 0.;

float live_obj_x = 0.;
float live_obj_y = 0.;
float live_obj_z = -3.;

int left_last_x, left_last_y, right_last_x, right_last_y;
bool left_down, right_down;
bool render_done = true;

/* GLUT callback Handlers */
void resize(int _width, int _height)
{
    delete[] client->screen_buffer;
    client->screen_size = 3*_width*_height;
    client->screen_buffer = new char[client->screen_size];

    width = _width;
    height = _height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, float(width)/height, 0.1, 50.0);
    //glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;
    glTranslatef(live_obj_x, live_obj_y, live_obj_z);
    glRotated(live_rot_x, 1., 0., 0.);
    glRotated(live_rot_y, 0., 1., 0.);
    glRotated(live_rot_z, 0., 0., 1.);
}

void draw_buttons()
{
    glPushMatrix();

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -0.09, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_BLEND);
    glEnable(GL_BLEND);

    //loaddata button
    glColor4f(0.5, 0.5, 0.5, 0.5);
    glBegin(GL_QUADS);
    glVertex2d(10, 10);
    glVertex2d(60, 10);
    glVertex2d(60, 40);
    glVertex2d(10, 40);
    glEnd();

    glDisable(GL_BLEND);
    glPopMatrix();
}

void display()
{

    if (loaded) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glDrawPixels(width, height, GL_RGBA, GL_FLOAT, client->screen_buffer);	
        //bzero(client->screen_buffer, client->screen_size);
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, client->screen_buffer);	
        glFlush();
        glutSwapBuffers();
        return;
    }
  
    render_done = false;

    //const double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, float(width)/height, 0.1, 50.0);
    //glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;
    glTranslatef(live_obj_x, live_obj_y, live_obj_z);
    glRotated(live_rot_x, 1., 0., 0.);
    glRotated(live_rot_y, 0., 1., 0.);
    glRotated(live_rot_z, 0., 0., 1.);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    //glutSolidCone(1,1,slices,stacks);
    glutSolidCube(1.);
    glDisable(GL_LIGHTING);

    //draw buttons
    //draw_buttons();

    glFlush();
    glutSwapBuffers();

    render_done = true;
}

void key(unsigned char key, int x, int y)
{
    cerr << "client: key()" << endl;
    std::stringstream msgstream;
    std::string msg;
    std::string msg2;

    switch (key) {
    case 27 :
    case 'q':
        exit(0);
        break;

    case 'l':
        cerr << "client: load" << endl;
        //msgstream << "command=0" << endl;
        msgstream << "load" << endl;
        msg = msgstream.str();
        client->send(msg);
        loaded = true;
        break;

    case 'o':
        msgstream << "command=" << "1" << endl;
        msg = msgstream.str();
        client->send(msg);
        break;

    case '+':
        msgstream << "command=" << "4" << endl;
        msg = msgstream.str();
        client->send(msg);
        break;

    case '-':
        msgstream << "command=" << "5" << endl;
        msg = msgstream.str();
        client->send(msg);
        break;

    case 'x':
        //msgstream << "and=" << "5" << endl;
        msgstream << "cut x " << "off" << endl;
        msg = msgstream.str();
        cerr << "client: " << msg << endl;
        client->send(msg);
        break;

    case 'X':
        msgstream << "cut x " << " on" << endl;
        msg = msgstream.str();
        cerr << "client: " << msg << endl;
        client->send(msg);
        break;

    case 'y':
        msgstream << "cut y " << "off" << endl;
        msg = msgstream.str();
        cerr << "client: " << msg << endl;
        client->send(msg);
        break;

    case 'Y':
        msgstream << "cut y " << "on" << endl;
        msg = msgstream.str();
        cerr << "client: " << msg << endl;
        client->send(msg);
        break;

    case 'z':
        msgstream << "cut z " << "off" << endl;
        msg = msgstream.str();
        cerr << "client: " << msg << endl;
        client->send(msg);
        break;

    case 'Z':
        msgstream << "cut z " << "on" << endl;
        msg = msgstream.str();
        cerr << "client: " << msg << endl;
        client->send(msg);
        break;

    default:
        return;
    }
    
    /*
      for(int j=0; j<512; j=j+1){
      cerr << "client read: " << j << endl;
      client->receive(client->screen_buffer + j*4*512, 4*512);	//unsigned byte
      }
    */

    client->receive(client->screen_buffer, client->screen_size);	//unsigned byte

    //cin >> msg;
    //strncpy(client->screen_buffer, msg.c_str(), 512*512*4);

    display();
    //glutPostRedisplay();

    cerr << "client: key() done" << endl;
}

void update_rot(int delta_x, int delta_y)
{
    live_rot_x += delta_x;
    live_rot_y += delta_y;

    if (live_rot_x > 360.0)
        live_rot_x -= 360.0;	
    else if (live_rot_x < -360.0)
        live_rot_x += 360.0;

    if (live_rot_y > 360.0)
        live_rot_y -= 360.0;	
    else if (live_rot_y < -360.0)
        live_rot_y += 360.0;
}

void update_trans(int delta_x, int delta_y, int delta_z)
{
    live_obj_x += delta_x*0.01;
    live_obj_y += delta_y*0.01;
    live_obj_z += delta_z*0.01;
}

bool test_in_box(int box_x0, int box_y0, int box_x1, int box_y1, int x, int y)
{
    return (box_x0<=x && box_x1>=x && box_y0<=y && box_y1>=y);
}

void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        left_last_x = x;
        left_last_y = y;

        if (state == GLUT_DOWN) {
            left_down = true;
            right_down = false;
        } else {
            left_down = false;
            right_down = true;
        }
    } else {
        right_last_x = x;
        right_last_y = y;

        if (state == GLUT_DOWN) {
            left_down = false;
            right_down = true;
        } else {
            left_down = true;
            right_down = false;
        }
    }
}

void motion(int x, int y)
{
    cerr << "client: motion() " << endl;

    if (render_done) {
        int old_x, old_y;	

        if (left_down) {
            old_x = left_last_x;
            old_y = left_last_y;   
        } else {
            old_x = right_last_x;
            old_y = right_last_y;   
        }

        int delta_x = x - old_x;
        int delta_y = y - old_y;

        //more coarse event handling
        if (abs(delta_x)<5 && abs(delta_y)<5)
            return;

        if (left_down) {
            left_last_x = x;
            left_last_y = y;

            update_rot(delta_y, delta_x);
            std::stringstream msgstream;
            //msgstream << "command=" << "2" << "&delta_x=" << delta_y << "&delta_y=" << delta_x << endl;
            msgstream << "camera " << live_rot_x << " " << live_rot_y << " " << live_rot_z << " " << live_obj_z << endl;
            std::string msg;
            msg = msgstream.str();
            std::cout << "client msg: " << msg <<"\n";
            //client->send(msg);
            client->send((char*) msg.c_str(), (int) strlen(msg.c_str()));
        } else {
            right_last_x = x;
            right_last_y = y;

            update_trans(0, 0, delta_y);
            std::stringstream msgstream;
            //msgstream << "command=" << "3" << "&delta_x=" << delta_y << "&delta_y=" << delta_x << endl;
            msgstream << "camera " << live_rot_x << " " << live_rot_y << " " << live_rot_z << " " << live_obj_z << endl;
            std::string msg;
            msg = msgstream.str();
            std::cout << "client msg: " << msg <<"\n";
            //client->send(msg);
            client->send((char*) msg.c_str(), (int) strlen(msg.c_str()));
        }

        client->receive(client->screen_buffer, client->screen_size);

        /*
          for (int j=0; j<512; j=j+4) {
              unsigned int r,g,b,a;
              r = client->screen_buffer[j];
              g = client->screen_buffer[j+1];
              b = client->screen_buffer[j+2];
              a = client->screen_buffer[j+3];
              //fprintf(stderr, "(%d %d %d %d) ", r,g,b,a);	//unsigned byte
              fprintf(stderr, "(%X %X %X %X) ", r,g,b,a);	//unsigned byte
          }
        */

        display();
        cerr << "client: motion() done " << endl;
    }
}

void idle()
{
    /*
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 100000000;

      nanosleep(&ts, 0);
    */

    //send requests
    Event* cur = event[cur_event];
    std::stringstream msgstream;
    std::string msg;

    switch (cur->type) {
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
    //std::cout << "client msg: " << msg <<"\n";

    //start timer
    client->send((char*) msg.c_str(), (int) strlen(msg.c_str()));
    client->receive(client->screen_buffer, client->screen_size);
    //end timer
}

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void init_client(char* host, char* port, char* file)
{
    //load the event file
    FILE* fd = fopen(file, "r");
    //load 5000 events
    for (int i=0; i<5000; i++) {
        int type;
        float param[3];
        fscanf(fd, "%d %f %f %f\n", &type, param, param+1, param+2);
        event[i] = new Event(type, param, 0);
        fprintf(stderr, "%d %f %f %f\n", type, param[0], param[1], param[2]);
    }
    fclose(fd);

    //std::string host = "localhost";
    //hostname -i
    //std::string host = "128.46.137.192";
  
    std::string hostname = host;
    client = new RenderClient(hostname, atoi(port));

    //point stdin stdout to socket
    /*
      close(0);
      close(1);
      dup2(client->Socket::m_sock, 0);
      dup2(client->Socket::m_sock, 1);
    */
  
    std::string msg1 = "hello";
    std::string msg2 = " ";

    //cout << msg1;
    //cin >> msg2;

    client->send(msg1);
    client->receive(msg2);
  
    cerr << "client: msg received - " << msg2 << endl;
    cerr << "connection to server established" << endl;
}

void print_gl_info()
{
    fprintf(stderr, "OpenGL vendor: %s %s\n", glGetString(GL_VENDOR), glGetString(GL_VERSION));
    fprintf(stderr, "OpenGL renderer: %s\n", glGetString(GL_RENDERER));
}

void menu_cb(int entry)
{
    std::stringstream msgstream;
    std::string msg;
    std::string msg2;
    switch (entry) { 
    case 0:
        msgstream << "command=" << "0";
        msgstream >> msg;
        client->send(msg);

        client->receive(msg2);
	
        loaded = true;
        break;

    case 1:
        msgstream << "command=" << "1";
        msgstream >> msg;
        client->send(msg);

        client->receive(msg2);
        break;

    case 2:
        msgstream << "command=" << "2";
        msgstream >> msg;
        client->send(msg);

        client->receive(msg2);
        break;
    }
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
    //parameters:  hostip and port and event file
    if (argc!=4)
        help(argv[0]);

    width =512; height=512;

    init_client(argv[1], argv[2], argv[3]);

    glutInit(&argc, argv);
    glutInitWindowSize(width,height);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("Client");

    /*
      glutCreateMenu(menu_cb);
      glutAddMenuEntry("load data ", 0);
      glutAddMenuEntry("toggle mode (on/off screen)", 1);
      glutAttachMenu(GLUT_RIGHT_BUTTON);
    */

    glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    glutIdleFunc(idle);

    glClearColor(0.,0.,0.,0.);

    glDepthFunc(GL_LESS);
    glDepthRange(0,1);
    glDisable(GL_DEPTH_TEST);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    glDisable(GL_ALPHA_TEST);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

    print_gl_info();

    left_down = false;
    right_down = false;

    glutMainLoop();

    return 0;
}

