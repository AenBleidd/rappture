/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "TransferFunctionMain.h"
#include "ColorGradientGLUTWindow.h"
#include "ColorPaletteWindow.h"
#include "TransferFunctionGLUTWindow.h"

int mapNumOfOutput=MAP_NUM_OF_OUTPUT;
float* mapOutput;

int cm_winx, cm_winy;
int cm_unitWidth;

ColorGradient* map;
GLUI *cm_glui;
GLUI_Panel* pan;
ControlPoint* cm_gvSelectedPoint;

int cm_editState;

namespace ColorGradientWindow {
    int cm_numOfPoints;

    bool cm_gvIsDragging=false;
    bool cm_gvScaleDragging=false;

    double distanceThreshold;
    double gv_X, gv_Y;
    double gv_DX, gv_DY;
    double gv_lastX, gv_lastY;
};

using namespace ColorGradientWindow;

//extern UnstructuredVolumeRenderer *uvol;

ColorGradientGLUTWindow::ColorGradientGLUTWindow()
{
    map = new ColorGradient();
}

ColorGradientGLUTWindow::~ColorGradientGLUTWindow()
{ 
}

void 
ColorGradientGLUTWindow::cmInit(int main_win_x, int main_win_y)
{
    mapOutput=(float*)malloc(3*sizeof(float)*mapNumOfOutput);
    cm_winx=main_win_x;
    //cm_winy=main_win_y/3;
    cm_winy=100;
    cm_unitWidth=(int)((double)tf_winx*0.95);
    map=new ColorGradient();
    cm_editState=0;
    cm_gvIsDragging=false;
    cm_gvScaleDragging=false;
    cm_gvSelectedPoint=0;
    cm_numOfPoints=2;
    gv_X=0;
    gv_Y=0;
    gv_DX=0;
    gv_DY=0;
    gv_lastX=0;
    gv_lastY=0;
    distanceThreshold=64;

    if (mapOutput==0)
        fprintf(stderr, "NULL");

    //initialize mapOutput
    int i=0;
    for (i=0; i<mapNumOfOutput; i++){
        color_table[i][0]=0;
    }
}

void 
ColorGradientGLUTWindow::createGLUIWidgets()
{
    cm_glui = GLUI_Master.create_glui_subwindow(colorMapWindow, 
                                                GLUI_SUBWINDOW_BOTTOM);
    pan=cm_glui->add_panel("buttons", GLUI_PANEL_NONE);
    cm_glui->set_main_gfx_window(colorMapWindow);
    cm_glui->add_column_to_panel(pan, false );
    cm_glui->add_statictext_to_panel(pan,"Color Controls:");
    cm_glui->add_column_to_panel(pan,false );
    cm_glui->add_button_to_panel(pan,"Add", 1, changeState);
    cm_glui->add_column_to_panel(pan, false );
    cm_glui->add_button_to_panel(pan, "Edit", 2, changeState);
    cm_glui->add_column_to_panel(pan, false );
    cm_glui->add_button_to_panel(pan, "Remove", 3, changeState);
}

void update_tf_texture();

void 
ColorGradientGLUTWindow::changeState(int arg)
{
    switch (arg){
    case 1: //add color 
        cm_editState=1;
        break;
    case 2:
        //cm_editState=2;
        ColorPalette::cpInit();
        update_tf_texture();
        break;
    case 3:
        cm_editState=3;
        if (cm_gvSelectedPoint!=0)
            map->removeKey(cm_gvSelectedPoint->x);
        cm_editState=0;
        update_tf_texture();
        break;
    case 4:
        cm_editState=4;
        break;

    default:
        break;
    }
}

void 
ColorGradientGLUTWindow::cmDisplay()
{
    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    int i;

    if (map == NULL) {
        fprintf(stderr, "ColorGradient Window : forgot to bind the colorMap\n");
        return;
    }

    Color* leftColor=map->keyColors;
    Color* rightColor;
    ControlPoint* leftPoint=map->keyList;
    ControlPoint* rightPoint;
    double left_x, right_x;
    GLfloat left_color[3];
    GLfloat right_color[3];


    //draw "rainbow"
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);

    glBegin(GL_QUADS);
    for (i=0; i<map->numOfKeys-1; i++) { 
        left_x=leftPoint->x;
        leftColor->GetRGB(left_color);

        rightColor=leftColor->next;
        rightPoint=leftPoint->next;
        right_x=rightPoint->x;
        rightColor->GetRGB(right_color);

        //printf("right=(%g, %g, %g)\n", right_color[0], right_color[1], right_color[2]);
        glColor3f(left_color[0], left_color[1], left_color[2]); 
        glVertex2d(left_x, 0);
        glVertex2d(left_x, cm_winy-10);

        //printf("left=(%g, %g, %g)\n", left_color[0], left_color[1], left_color[2]);
        glColor3f(right_color[0], right_color[1], right_color[2]);      
        glVertex2d(right_x, cm_winy-10);
        glVertex2d(right_x, 0);

        leftColor=leftColor->next;
        leftPoint=leftPoint->next;
    }
    glEnd();
    glColor3f(0, 0, 0);

    //draw bar
    glLineWidth(1.0);
    glBegin(GL_LINES);
    glVertex2d(15, cm_winy-1);
    glVertex2d(15+tf_unitWidth, cm_winy-1);
    glEnd();

    i=0;
    Color* curColor=map->keyColors;
    ControlPoint* curKey=map->keyList;
    for (i=0; i<map->numOfKeys ; i++) {
        //glColor3d(curColor->R, curColor->G, curColor->B);
        glColor3d(0.0, 0.0, 0.0);
        curKey->glDraw_2();
        curColor=curColor->next;
        curKey=curKey->next;
    }

    glColor3d(0.0, 0.0, 0.0);
    glutSwapBuffers();
}

void 
ColorGradientGLUTWindow::cmReshape(int x, int y)
{
    //printf("cmReshape");
    cm_winx=x;
    cm_winy=100;

    double scale=((double)tf_winx*0.95)/((double)cm_unitWidth);
    cm_unitWidth=(int)((double)tf_winx*0.95);
    map->scaleKeys(scale, cm_unitWidth);

    glViewport( 0, 0, x, y );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Set up most convenient projection
    gluOrtho2D(0, x, 0, y);
    glutPostRedisplay();
}

void 
ColorGradientGLUTWindow::cmDestroy()
{
}

void 
ColorGradientGLUTWindow::cmKeyboard(unsigned char key, int x, int y)
{
    switch(key) {    
    case 'p':
        //printPoints();
        break;
    case 'c':
        printInterpolation();
        break; 
    default:
        break;
    }
}

void 
ColorGradientGLUTWindow::cmIdle()
{ 
}

void 
ColorGradientGLUTWindow::cmMouse(int button, int state, int x, int y)
{       
    //printf("cm mouse\n");

    GLint viewportVector[4];
    double scX, scY;

    glGetIntegerv(GL_VIEWPORT, viewportVector);
        
    gv_X = scX = x;
    gv_Y = scY = viewportVector[3] - (GLint) y - 1;

    gv_lastY = scY;

    //Selecting point, begin dragging
    bool gvIsSelected = SelectPoint(scX, scY);

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (gvIsSelected && cm_editState==0) {
            //assert(cm_gvSelectedPoint!=NULL);
            gv_DX = scX - cm_gvSelectedPoint->x; //Offset from current point to selected point, for dragging corrections
        
            cm_gvSelectedPoint->dragged = true;
            cm_gvIsDragging = true;
        }
        /*else if ((!gvIsSelected) && cm_editState==1) {
            GLint viewportVector[4];
            double scX, scY;

            glGetIntegerv(GL_VIEWPORT, viewportVector);

            gv_X = scX = x;
            gv_Y = scY = viewportVector[3] - (GLint) y - 1;
            cm_gvSelectedPoint=map->addKey(scX - gv_DX, new Color(1,1,1));
            cm_editState=0;
            WriteControlPoints();
        }*/
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        // End dragging code.
        cm_gvIsDragging = false;
        if (cm_gvSelectedPoint)
            cm_gvSelectedPoint->dragged = false;
    }
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        GLint viewportVector[4];
        double scX, scY;
        
        glGetIntegerv(GL_VIEWPORT, viewportVector);
        
        gv_X = scX = x;
        gv_Y = scY = viewportVector[3] - (GLint) y - 1;
        cm_gvSelectedPoint=map->addKey(scX - gv_DX, new Color(1,1,1));
        cm_editState=0;
        WriteControlPoints();
    }

    if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
        cm_gvScaleDragging = true;

    if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP)
        cm_gvScaleDragging = false;

    glutSetWindow(colorMapWindow);
    glutPostRedisplay();
}

void 
ColorGradientGLUTWindow::cmMotion(int x, int y)
{
    GLint viewportVector[4];
    double scX, scY;

    glGetIntegerv(GL_VIEWPORT, viewportVector);

    gv_X = scX = x;
    gv_Y = scY = viewportVector[3] - (GLint) y - 1;

    if (cm_gvIsDragging == true && cm_gvSelectedPoint!=NULL) {
        //Dragging handling code.
        cm_gvSelectedPoint->dragged = true;

        ControlPoint* cur=map->keyList;
        while (cur->next!=0){
            cur=cur->next;
        }

        //update x, y values for selected point
        if (cm_gvSelectedPoint!=map->keyList && cm_gvSelectedPoint!=cur){

            cm_gvSelectedPoint->x = scX - gv_DX;
            //not the first one, not the last one
            cm_gvSelectedPoint->x = scX - gv_DX;
            if (cm_gvSelectedPoint->x<=15){
                cm_gvSelectedPoint->x=16;
            }
            else if(cm_gvSelectedPoint->x>=15+cm_unitWidth){
                cm_gvSelectedPoint->x=15+cm_unitWidth-1;
            }
                        
            boundaryChecking();
            WriteControlPoints();
        }
    }

    glutPostRedisplay(); 
}

bool 
ColorGradientGLUTWindow::SelectPoint(double x, double y)
{
    //Reset selected/dragged flags
    ControlPoint* cur=map->keyList;
    while(cur!=0){
        cur->dragged=false;
        cur->selected=false;
        cur=cur->next;
    }

    cm_gvSelectedPoint = NULL;

    //Traverse the points and find the one that seems close
    cur=map->keyList;
    while (cur!=0) {
        double distanceSq;
        distanceSq = (cur->x - x)*(cur->x - x) + (cur->y - y)*(cur->y - y);     
        if (distanceSq < distanceThreshold){
            cur->selected = true;
            cm_gvSelectedPoint = cur;
            return true;
        }
        cur=cur->next;
    }

    return false;
}

void update_tf_texture();

void 
ColorGradientGLUTWindow::WriteControlPoints()
{
    //special case: all points have the same color
    Color* curColor=map->keyColors;
    bool allSameY=true;
    double R = map->keyColors->R;
    double G = map->keyColors->G;
    double B = map->keyColors->B;
    while (curColor!=0) {
        if (curColor->R!=R || curColor->G!=G || curColor->B!=B){
            allSameY=false;
            break;
        }
        curColor=curColor->next;
    }

    if (allSameY) {
        int i=0;

        for (i=0;i<mapNumOfOutput;i++) {
            color_table[i][0]=R;
            color_table[i][1]=G;
            color_table[i][2]=B;
        }
        return;
    }

    float unit=(float)cm_unitWidth/(float)mapNumOfOutput;

    curColor=map->keyColors;
    Color* nextColor=curColor->next;

    double r1=curColor->R*255;
    double g1=curColor->G*255;
    double b1=curColor->B*255;

    double r2=nextColor->R*255;
    double g2=nextColor->G*255;
    double b2=nextColor->B*255;

    ControlPoint* curPoint=map->keyList;
    ControlPoint* nextPoint=curPoint->next;

    double x1=curPoint->x;
    double x2=nextPoint->x;

    float t=0;

    int i=0;
    for (i=0; i<numOfOutput; i++) {
        if (unit*i+15>x2) {
            //go to next line segment
            curColor=nextColor;
            nextColor=nextColor->next;

            curPoint=nextPoint;
            nextPoint=nextPoint->next;

            if (nextColor==0 || nextPoint==0){
                return;
            }

            x1=curPoint->x;
            x2=nextPoint->x;

            r1=curColor->R*255;
            g1=curColor->G*255;
            b1=curColor->B*255;

            r2=nextColor->R*255;
            g2=nextColor->G*255;
            b2=nextColor->B*255;
        }
        t=((float)(unit*i+15-x1))/((float)(x2-x1));

        color_table[i][0]=(float)(r1+t*(r2-r1))/(float)255;
        color_table[i][1]=(float)(g1+t*(g2-g1))/(float)255;
        color_table[i][2]=(float)(b1+t*(b2-b1))/(float)255;
    }   

    update_tf_texture();
    glutSetWindow(colorMapWindow);
}

ControlPoint* 
ColorGradientGLUTWindow::boundaryChecking()
{
    //find left and right neighbor in the list
    //If point is dragged passing the neighbor, the points are reordered.

    ControlPoint* left=map->keyList;
    ControlPoint* right=map->keyList;

    while (right!=cm_gvSelectedPoint) {
        left=right;
        right=right->next;
    }

    //selected point is not the right-end point
    if (right->next!=0){
        right=right->next;
        if (cm_gvSelectedPoint->x>right->x) {
            sortPoints();
        }
    }

    //selected point is not the left-end point
    if (left!=cm_gvSelectedPoint) {
        if (cm_gvSelectedPoint->x<left->x) {
            sortPoints();
        }
    }

    return cm_gvSelectedPoint;
}

//sort all controlpoints by bubble sort
void 
ColorGradientGLUTWindow::sortPoints()
{
    if (map->numOfKeys==0)
        return;

    //bubble-sort list in ascending order by X
    ControlPoint* cur;
    ControlPoint* pre;
    ControlPoint* a;
    ControlPoint* b;
    cur=map->keyList;

    Color* curColor;
    Color* preColor;
    Color* c;
    Color* d;
    curColor=map->keyColors;

    preColor = NULL;
    pre = NULL;

    int i;
    for (i=0;i<map->numOfKeys-1;i++) {
        while (cur->next!=NULL){
            if (cur->x > cur->next->x) {
                if (cur==map->keyList) {
                    //first node
                    a=map->keyList;
                    b=map->keyList->next;
                    a->next=b->next;
                    b->next=a;
                    map->keyList=b;
                    cur=map->keyList;

                    c=map->keyColors;
                    d=map->keyColors->next;
                    c->next=d->next;
                    d->next=c;
                    map->keyColors=d;
                    curColor=map->keyColors;
                } else {
                    a=cur;
                    b=cur->next;
                    a->next=b->next;
                    b->next=a;
                    pre->next=b;
                    cur=b;

                    c=curColor;
                    d=curColor->next;
                    c->next=d->next;
                    d->next=c;
                    preColor->next=d;
                    curColor=d;
                }
            }
            pre=cur;
            cur=cur->next;

            preColor=curColor;
            curColor=curColor->next;
        }
        cur=map->keyList;
        curColor=map->keyColors;
    }
    WriteControlPoints();
}

void 
ColorGradientGLUTWindow::printInterpolation()
{
    int i=0;
    for (i=0;i<numOfOutput;i++) {
        //printf("(%g,%g,%g) ", mapOutput[3*i], mapOutput[3*i+1], mapOutput[3*i+2]);
        fprintf(stderr, "(%g,%g,%g) ", color_table[i][0], color_table[i][1], color_table[i][2]);
    }
}
