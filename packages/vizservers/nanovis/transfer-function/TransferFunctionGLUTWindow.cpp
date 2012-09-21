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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "TransferFunctionGLUTWindow.h"
#include "ColorGradientGLUTWindow.h"
#include "ColorPaletteWindow.h"
#include "ControlPoint.h"
#include "TransferFunctionMain.h"

/****************Result Array******************/
int numOfOutput=NUM_OF_OUTPUT;
float* output=(float*)malloc(sizeof(float)*numOfOutput); //result array
/**********************************************/

//Histograms
Histogram Hist[4];

int tf_winx, tf_winy;
int tf_unitWidth;
extern double distanceThreshold;
ControlPoint *tf_gvSelectedPoint;
bool tf_gvIsDragging=false;
ControlPoint* tf_pointList=0;
int tf_numOfPoints;

float tfMaximum;
int tf_pointEditState;

namespace TransferFunctionWindow
{
    int winx = 600;
    int winy = 150;

    int unitHeight;

    double gv_X, gv_Y;
    double gv_lastY;

    bool gvScaleDragging=false;

    double gv_DX, gv_DY;

    float lv_tf_height_scale = 2.0;
    float last_lv_tf_height_scale= 2.0;

    float lv_tf_width_scale = 1.0;
    float last_lv_tf_width_scale= 1.0;
};

using namespace TransferFunctionWindow;

TransferFunctionGLUTWindow::TransferFunctionGLUTWindow()
{
}

TransferFunctionGLUTWindow::~TransferFunctionGLUTWindow()
{
}

void TransferFunctionGLUTWindow::tfInit(int main_window_x, int main_window_y)
{
    //initialize the global list of contorlPoint "tf_pointList"
    tf_winx=main_window_x;
    tf_winy=main_window_y*2/3;

    unitHeight = (int)(0.8*tf_winy);
    tf_unitWidth = (int)(0.95*tf_winx);

    //printf("tf_unitWidth=%d\n", tf_unitWidth);

    tf_pointList = new ControlPoint(15,15);
    tf_pointList->next = new ControlPoint(15+tf_unitWidth,15);

    int i=0;
    for (i=0;i<numOfOutput;i++) {
        //output[i]=0;
        color_table[i][3]=0;
    }
    tf_numOfPoints=2;
    tf_pointEditState=0;

    tf_gvSelectedPoint = new ControlPoint(0,0);

    fprintf(stderr, "Transferfunction Init...\n");
}

void TransferFunctionGLUTWindow::tfDisplay()
{
    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glColor3d(0, 0, 0);

    //draw x and y axis
    glBegin(GL_LINES);
    glVertex2d(15, 15);
    glVertex2d(15, winy);

    glVertex2d(15, 15);
    glVertex2d(winx, 15);
    glEnd();

    //printf("tf_unitWidth=%d\n", tf_unitWidth);

    //draw label for X
    glRasterPos2f(2, 2);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, '0');

    glRasterPos2f(18+tf_unitWidth, 2);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, '1');

    //draw marker on axis
    glLineWidth(2.0);
    glBegin(GL_LINES);
    glVertex2d(15+tf_unitWidth, 20);
    glVertex2d(15+tf_unitWidth, 15);

    int k;
    k = 1;
    while (15+k*unitHeight/lv_tf_height_scale < winy) {
        glVertex2d(20, 15+k*unitHeight/lv_tf_height_scale);
        glVertex2d(15, 15+k*unitHeight/lv_tf_height_scale);
        k++;
    }
    glEnd();
    glLineWidth(1.0);

    //draw label for Y
    k = 1;

#ifdef notdef
      char label[3];
      while (15+k*unitHeight/lv_tf_height_scale < winy) {
	  glRasterPos2f(2,15+k*unitHeight/lv_tf_height_scale);
	  label[0]=0;
	  label[1]=0;
	  label[2]=0;
	  itoa(k, label, 10);
	  int i=0;
	  while (label[i]!=0) {
	      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, label[i]);
	      i++;
	  }
	  k++;
      }
      printf("tf_unitWidth=%d\n", tf_unitWidth);
#endif

    //draw all points
    ControlPoint* cur=tf_pointList;
    while (cur!=0) {
        cur->glDraw();
        cur=cur->next;
    }

    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2.0);

    //connect all points
    cur=tf_pointList;
    glBegin(GL_LINES);
    glVertex2d(cur->x, cur->y);

    while (cur!=0) {
        if (cur->next!=0) {
            glVertex2d(cur->x, cur->y);
            glVertex2d(cur->x, cur->y);
        } else {
            glVertex2d(cur->x, cur->y);
        }
        cur=cur->next;
    }
    glVertex2d(winx, 15);
    glEnd();

    glLineWidth(1.0);
    glDisable(GL_LINE_SMOOTH);
    plotHist();

    glutSwapBuffers();
}

bool TransferFunctionGLUTWindow::SelectPoint(double x, double y)
{
    //printf("select point\n");

    if (tf_numOfPoints==0)
        return false;

    //Reset selected/dragged flags
    ControlPoint* cur=tf_pointList;
    while (cur!=0) {
        cur->dragged=false;
        cur->selected=false;
        cur=cur->next;
    }

    tf_gvSelectedPoint = NULL;

    //Traverse the points and find the one that seems close
    cur=tf_pointList;
    while (cur!=0) {
        double distanceSq;
        distanceSq = (cur->x - x)*(cur->x - x) + (cur->y - y)*(cur->y - y);

        if (distanceSq < distanceThreshold) {
            cur->selected = true;
            tf_gvSelectedPoint = cur;
            return true;
        }
        cur=cur->next;
    }

    return false;
}

void TransferFunctionGLUTWindow::tfMouse(int button, int state, int x, int y)
{
    //printf("tf mouse\n");
    //printf("(%d,%d)\n", x, y);
    GLint viewportVector[4];
    double scX, scY;

    glGetIntegerv(GL_VIEWPORT, viewportVector);

    gv_X = scX = x;
    gv_Y = scY = viewportVector[3] - (GLint) y - 1;

    gv_lastY = scY;

    //Selecting point, begin dragging
    bool gvIsSelected = SelectPoint(scX, scY);  

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (gvIsSelected && tf_pointEditState==0) {
            assert(tf_gvSelectedPoint!=NULL);
            gv_DX = scX - tf_gvSelectedPoint->x; //Offset from current point to selected point, for dragging corrections
            gv_DY = scY - tf_gvSelectedPoint->y;

            tf_gvSelectedPoint->dragged = true;
            tf_gvIsDragging = true;
        }
        /*else if ((!gvIsSelected) && tf_pointEditState==1) {
            GLint viewportVector[4];
            double scX, scY;

            glGetIntegerv(GL_VIEWPORT, viewportVector);

            gv_X = scX = x;
            gv_Y = scY = viewportVector[3] - (GLint) y - 1;
            tf_gvSelectedPoint=addPoint(scX - gv_DX,scY - gv_DY);
            tf_pointEditState=0;
            WriteControlPoints();
        }*/
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        // End dragging code.
        tf_gvIsDragging = false;
        if (tf_gvSelectedPoint)
            tf_gvSelectedPoint->dragged = false;
    }

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        GLint viewportVector[4];
        double scX, scY;

        glGetIntegerv(GL_VIEWPORT, viewportVector);

        gv_X = scX = x;
        gv_Y = scY = viewportVector[3] - (GLint) y - 1;
        tf_gvSelectedPoint=addPoint(scX - gv_DX,scY - gv_DY);
        tf_pointEditState=0;
        WriteControlPoints();
    }
        
    if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
        gvScaleDragging = true;

    if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP)
        gvScaleDragging = false;

    glutPostRedisplay();
}

void TransferFunctionGLUTWindow::tfMotion(int x, int y)
{
    GLint viewportVector[4];
    double scX, scY;

    glGetIntegerv(GL_VIEWPORT, viewportVector);

    gv_X = scX = x;
    gv_Y = scY = viewportVector[3] - (GLint) y - 1;

    if (tf_gvIsDragging == true && tf_gvSelectedPoint!=NULL) {
        //Dragging handling code.
        tf_gvSelectedPoint->dragged = true;

        ControlPoint* cur=tf_pointList;
        while (cur->next!=0) {
            cur=cur->next;
        }

        //update x, y values for selected point
        if (tf_gvSelectedPoint!=tf_pointList && tf_gvSelectedPoint!=cur) {
            //not the first one, not the last one
            tf_gvSelectedPoint->x = scX - gv_DX;
            if (tf_gvSelectedPoint->x<=15) {
                tf_gvSelectedPoint->x=16;
            } else if (tf_gvSelectedPoint->x>=15+tf_unitWidth) {
                tf_gvSelectedPoint->x=15+tf_unitWidth-1;
            }
        }
        tf_gvSelectedPoint->y = scY - gv_DY;

        //Boundary conditions, so we can't drag point away from the screen.
        if (scY - gv_DY > viewportVector[3])
            tf_gvSelectedPoint->y = viewportVector[3] - 1;
        //points can't be dragged below x axis
        if (scY - gv_DY < 15)
            tf_gvSelectedPoint->y = 15;

        boundaryChecking();
    }

    if (gvScaleDragging == true) {
        last_lv_tf_height_scale=lv_tf_height_scale;
        lv_tf_height_scale += (scY - gv_lastY)*0.007;
        gv_lastY = scY;
        scalePointsY((int)(scY - gv_lastY));
    }

    WriteControlPoints();

    glutPostRedisplay();
}

void TransferFunctionGLUTWindow::tfIdle()
{
    /*
      glutSetWindow(tfWinId);
      glutPostRedisplay();

      glutSetWindow(cmWinId);
      glutPostRedisplay();

      glutSetWindow(cpWinId);
      glutPostRedisplay();
    */
}

void TransferFunctionGLUTWindow::tfKeyboard(unsigned char key, int x, int y)
{
    switch (key) {    
    case 'q':
        exit(0);
        break;    
    case 'a':
        tf_pointEditState=1;
        break;
    case 'd':
        tf_pointEditState=2;
        if (tf_gvSelectedPoint!=NULL) {
            removePoint(tf_gvSelectedPoint);
            tf_gvSelectedPoint=0;
            WriteControlPoints();
            glutPostRedisplay();
        }
        tf_pointEditState=0;
        break;   
    case 'p':
        printPoints();
        break;
    case 'w':
        printInterpolation();
        break; 
    case 'r':
        readHist();
        break; 
    case 'u':
        dumpHist();
        break; 
    case 'i':
        printHist();
        break;

    default:
        break;
    }    
}

void  TransferFunctionGLUTWindow::tfReshape(int x, int y)
{
    //do not accept 0 value!!!
    if (x==0 || y==0)
        return;

    last_lv_tf_height_scale=lv_tf_height_scale;
    lv_tf_height_scale=lv_tf_height_scale*y/winy;

    last_lv_tf_width_scale= lv_tf_width_scale;
    lv_tf_width_scale=lv_tf_width_scale*x/winx;
    scalePointsX();

    float xy_aspect;

    //lastWinx= winx;
    //lastWiny= winy;

    winx = x;
    winy = y;

    unitHeight = (int)(0.8*y);
    tf_unitWidth = (int)(0.95*x);

    xy_aspect = (float)x / (float)y;
    glViewport( 0, 0, x, y );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    //Set up most convenient projection
    gluOrtho2D(0, x, 0, y);
    glutPostRedisplay();
}

void  TransferFunctionGLUTWindow::ReadControlPoints()
{
    //read the live variables and update the points.
}

void update_tf_texture();

void TransferFunctionGLUTWindow::WriteControlPoints()
{
    //special case: all points have the same y coordinates
    double rangeY=0;
    float unitY=0;
    float unitX=0;
    double maxY=tf_pointList->y;
    double maxX=tf_pointList->x;

    //special case: all points have the same y coordinates,generate an error
    ControlPoint* cur=tf_pointList;
    int allSameY=1;
    int y=(int)tf_pointList->y;
    while (cur!=0) {
        if (cur->y!=y) {
            allSameY=0;
            break;
        }
        cur=cur->next;
    }

    if (allSameY==1) {
        int i=0;
        for (i=0; i<numOfOutput; i++){
            //output[i]=0;
            color_table[i][3]=0;
        }               
        return;
    }

    //get max Y
    cur=tf_pointList;
    while (cur!=0) {
        if (cur->y>maxY) {
            maxY=cur->y;
        }
        if (cur->x>maxX) {
            maxX=cur->x;
        }
        cur=cur->next;
    }

    //get min Y
    cur=tf_pointList;
    double minY=maxY;
    while (cur!=0) {
        if (cur->y<minY) {
            minY=cur->y;
        }
        cur=cur->next;
    }

    //range of Y
    rangeY=maxY-minY;

    tfMaximum = ((float) rangeY) / (unitHeight/lv_tf_height_scale);

    //unit to map [0,1] to [0, rangeY] 
    unitY=1/(float)rangeY;
    unitX=(float)(maxX-15)/(float)numOfOutput;

    cur=tf_pointList;
    ControlPoint* next=cur->next;
    double x1=cur->x;
    double y1=(cur->y)-minY;
    double x2=next->x;
    double y2=(next->y)-minY;
    float slope=((float)(y2-y1))/((float)(x2-x1));
    float t;  //t=(x-a)/(b-a)

    int i=0;
    for (i=0; i<numOfOutput; i++) {
        if (unitX*i+15>x2) {
            //go to next line segment
            cur=next;
            next=next->next;
            if (next==0) {
                return;
            }
            x1=cur->x;
            y1=cur->y-minY;
            x2=next->x;
            y2=next->y-minY;

            slope=((float)(y2-y1))/((float)(x2-x1));
        }
        t=((float)(unitX*i+15-x1))/((float)(x2-x1));
        //output[i] = (y1+t*(y2-y1))/rangeY;
        color_table[i][3]=control_point_scale*(y1+t*(y2-y1))/rangeY;
    }

    update_tf_texture();
    glutSetWindow(transferFunctionWindow);
}

void TransferFunctionGLUTWindow::printInterpolation()
{
    WriteControlPoints();
    int i=0;
    for (i=0; i<numOfOutput; i++) {
        //printf("%f, ",output[i]);
        fprintf(stderr, "%f, ",color_table[i][3]);
    }
    fprintf(stderr, "\n");
}

//sort all controlpoints by bubble sort
void 
TransferFunctionGLUTWindow::sortPoints()
{
    if (tf_numOfPoints==0)
        return;

    //bubble-sort list in ascending order by X
    ControlPoint* cur;
    ControlPoint* pre;
    ControlPoint* a;
    ControlPoint* b;
    cur=tf_pointList;

    int i;
    pre = NULL;			/* Suppress compiler warning */
    for (i=0; i<tf_numOfPoints-1; i++) {
        while (cur->next!=NULL) {
            if (cur->x > cur->next->x) {
                if (cur==tf_pointList) {
                    //first node
                    a=tf_pointList;
                    b=tf_pointList->next;
                    a->next=b->next;
                    b->next=a;
                    tf_pointList=b;
                    cur=tf_pointList;
                } else {
                    a=cur;
                    b=cur->next;
                    a->next=b->next;
                    b->next=a;
                    pre->next=b;
                    cur=b;
                }
            }
            pre=cur;
            cur=cur->next;
        }
        cur=tf_pointList;
    }
    WriteControlPoints();
}

ControlPoint* 
TransferFunctionGLUTWindow::addPoint(double x, double y)
{
    //check if x is between first and last point
    //if not return without adding point
    ControlPoint* last=tf_pointList;
    while (last->next!=0){
        last=last->next;
    }
    if (x<15 || x>last->x || y<15) {
        return tf_pointList;
    }

    ControlPoint* cur=tf_pointList;
    ControlPoint* pre=cur;

    //if x is the first or last. Don't add, but update.
    if (x==cur->x) {
        cur->y=y;
        return tf_pointList;
    } else if(x==last->x) {
        last->y=y;
        return last;
    }

    while (cur->next!=0) {
        if (x<=cur->x){
            ControlPoint* newPoint=new ControlPoint(x,y);
            if (cur==tf_pointList) {
                newPoint->next=tf_pointList;
                tf_pointList=newPoint;
            } else {
                newPoint->next=pre->next;
                pre->next=newPoint;
            }
            tf_numOfPoints++;
            //map->addKey(x, new Color(0,0,0));
            return newPoint;
        }
        pre=cur;
        cur=cur->next;  
    }

    if (x<=cur->x) {
        ControlPoint* newPoint=new ControlPoint(x,y);
        newPoint->next=pre->next;
        pre->next=newPoint;
        tf_numOfPoints++;
        //map->addKey(x, new Color(0,0,0));
        return newPoint;
    } else {
        cur->next=new ControlPoint(x,y);
        tf_numOfPoints++;
        //map->addKey(x, new Color(0,0,0));
        return cur->next;
    }
}

void 
TransferFunctionGLUTWindow::removePoint(void* ptr)
{
    if (tf_numOfPoints==0)
        return;

    ControlPoint* last=tf_pointList;
    while (last->next!=0) {
        last=last->next;
    }

    if (ptr==tf_pointList || ptr==last) {
        /*
          if (tf_numOfPoints=1) {
              delete(ptr);
              tf_pointList=0;
          } else {
              tf_pointList=tf_pointList->next;
              delete(ptr);
          }
        */
        return;
    } else {
        //find the previous point
        ControlPoint* cur=tf_pointList;
        ControlPoint* pre=cur;
        while (cur->next!=0) {
            if (cur==ptr) {
                break;
            }
            pre=cur;
            cur=cur->next;      
        }
        pre->next=cur->next;
        map->removeKey(cur->x);
        delete(cur);
    }
    tf_numOfPoints--;
}

//remove all points but the two initial points
void TransferFunctionGLUTWindow::cleanUpPoints()
{
    if (tf_numOfPoints<2)
        return;

    ControlPoint* cur=tf_pointList;

    if (tf_numOfPoints==2) {
        cur->x=15;
        cur->y=15;
                
        cur->next->x=15+tf_unitWidth;
        cur->next->y=15;
        return;
    }

    cur=tf_pointList->next;
    ControlPoint* pre=tf_pointList;

    while (cur->next!=0){
        if(pre!=tf_pointList)
            delete(pre);
        pre=cur;
        cur=cur->next;
    }

    delete(pre);
    tf_pointList->next=cur;

    tf_pointList->x=15;
    tf_pointList->y=15;

    tf_pointList->next->x=15+tf_unitWidth;
    tf_pointList->next->y=15;

    tf_numOfPoints=2;
    TransferFunctionGLUTWindow::WriteControlPoints();
}

//debugging: print out all points
void 
TransferFunctionGLUTWindow::printPoints()
{
    ControlPoint* cur=tf_pointList;
    fprintf(stderr, "********************\n");
    fprintf(stderr, "Total points %d \n", tf_numOfPoints);

    if (tf_numOfPoints==0)
        return;

    while (cur->next!=0) {
        fprintf(stderr, "(%g,%g)\n", cur->x, cur->y);
        cur=cur->next;
    }
    fprintf(stderr, "(%g,%g)\n", cur->x, cur->y);
    fprintf(stderr, "********************\n");
}

ControlPoint* 
TransferFunctionGLUTWindow::boundaryChecking()
{
    assert(tf_gvSelectedPoint!=0);
    assert(tf_pointList!=0);
    //find left and right neighbor in the list
    //If point is dragged passing the neighbor, the points are reordered.

    ControlPoint* left=tf_pointList;
    ControlPoint* right=tf_pointList;

    while (right!=tf_gvSelectedPoint) {
        left=right;
        right=right->next;
    }

    //selected point is not the right-end point
    if (right->next!=0) {
        right=right->next;
        if (tf_gvSelectedPoint->x>right->x) {
            sortPoints();
        }
    }

    //selected point is not the left-end point
    if (left!=tf_gvSelectedPoint) {
        if (tf_gvSelectedPoint->x<left->x){
            sortPoints();
        }
    }

    return tf_gvSelectedPoint;
}

void 
TransferFunctionGLUTWindow::scalePointsY(int offset)
{
    if (tf_numOfPoints<2)
        return;

    ControlPoint* cur=tf_pointList;
    while (cur!=0) {
        cur->y=((cur->y)-offset-15)*((float)last_lv_tf_height_scale/(float)lv_tf_height_scale)+15;
        cur=cur->next;
    }
    WriteControlPoints();
}

void 
TransferFunctionGLUTWindow::scalePointsX()
{
    ControlPoint* cur=tf_pointList;
    while (cur!=0) {
        cur->x=(cur->x-15)*((float)lv_tf_width_scale/(float)last_lv_tf_width_scale)+15;
        cur=cur->next;
    }
    WriteControlPoints();
}

void 
TransferFunctionGLUTWindow::dumpHist()
{
    /*
      FILE* fp = fopen("HistFile", "wt");

      if(!fp)
      return; //error

      int i=0; 
      for (i=0; i<Hist.range; i++){
      fprintf(fp, "%d\n", Hist.count[i]);       
      }
      printf("Histogram dumped.\n");
      fclose(fp);
    */
}

void 
TransferFunctionGLUTWindow::readHist()
{
    /*
      FILE* fp = fopen("HistFile", "rt");

      if(!fp)
      return; //error

      char buf[100];             //buffer to store one line from file
      char* token;               //pointer to token in the buffer
      char seps[]="\n";

      unsigned long value;

      int counter=0;

      while(true){
      //read file
      if (fgets(buf, 100, fp)==NULL)
      break;
      token=strtok(buf, seps);
      value=atof(token);
      Hist.count[counter]=value;
      counter++;
      }
      printf("Histgram loaded. Total number of values:%d\n", counter);
      tfDisplay();
      fclose(fp);
    */
}

//output histogram to console
void TransferFunctionGLUTWindow::printHist()
{
    int i=0; 
        
    for(int j=0; j<4; j++){
        for (i=0; i<Hist[j].range; i++){
            fprintf(stderr, "%ld\t", Hist[j].count[i]); 
        }
        fprintf(stderr, "\n\n");
    }

    /*
      for (i=0; i<Hist_p.range; i++){
      printf("%ld\t", Hist_p.count[i]); 
      }
      printf("\n\n");

      for (i=0; i<Hist_d.range; i++){
      printf("%ld\t", Hist_d.count[i]); 
      }
      printf("\n\n");

      for (i=0; i<Hist_ss.range; i++){
      printf("%ld\t", Hist_ss.count[i]);        
      }
      printf("\n\n");
    */
}

void 
TransferFunctionGLUTWindow::plotHist()
{
    for (int j=0; j<4; j++) {
        Histogram cur = Hist[j];

        float unitX=(float)tf_unitWidth/(float)(cur.range-1);
        float unitY;
        unsigned long maxY;
        unsigned long minY;
        double rangeY;

        float result[256];
 
        //get max Y
        maxY=0;
        int i=0;
        for (i=0; i<cur.range; i++) {
            if (cur.count[i]>maxY)
                maxY=cur.count[i];
        }
        //printf("maxY=%d\n", maxY);

        //get min Y
        minY=maxY;
        for (i=0; i<cur.range; i++) {
            if (cur.count[i]<minY)
                minY=cur.count[i];
        }
        //printf("minY=%d\n", minY);

        //rangeY=(double)log(maxY-minY);
        //rangeY=(double)(maxY-minY);
        rangeY=(float)log(float(maxY - minY));
        unitY=(float)(winy-20)/(float)rangeY;

        //reduce all values with log then by minY
        for (i=0; i<cur.range; i++) {
            if (cur.count[i]==0) {
                result[i]=0;
            } else {
                //result[i]=log((double)(cur.count[i]));
                //result[i]=double(cur.count[i]);
                //result[i]=log((float)(cur.count[i]))-log((float)minY);
                result[i]=log((float)(cur.count[i])-(float)minY);
            }   
        }

        //draw points
        glColor3d(0, 0, 1);
        glBegin(GL_LINES);
        for (i=0; i<cur.range-1; i++) {
            glVertex2f(15+i*unitX, 15+result[i]*unitY);
            glVertex2f(15+(i+1)*unitX, 15+result[i+1]*unitY);
        }
        glEnd();
        glColor3d(0, 0, 0);     
    }
}
