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

#include "MainWindow.h"
#include "TransferFunctionGLUTWindow.h"
#include "ColorPaletteWindow.h"
#include "ColorGradientGLUTWindow.h"
#include "TransferFunctionMain.h"

GLUI* main_glui;
GLUI_Panel* main_pan;

int main_winx, main_winy;
int mainWindow;
int colorMapWindow;
int transferFunctionWindow=-1;
int colorPaletteWindow = -1;
int unitWidth, unitHeight;
double distanceThreshold;

MainTransferFunctionWindow::MainTransferFunctionWindow(){}
MainTransferFunctionWindow::~MainTransferFunctionWindow(){}

//initialize 
void MainTransferFunctionWindow::mainInit()
{
    distanceThreshold=36;

    main_winx=600;
    main_winy=300;

    glutInitWindowPosition(100, 100);
    glutInitWindowSize(main_winx, main_winy );

    mainWindow=glutCreateWindow("Advanced Transfer Function");

    glutDisplayFunc(mainDisplay);
    glutReshapeFunc(mainReshape);
    glutKeyboardFunc (mainKeyboard);
    //glutIdleFunc (mainIdle);


    //glutMouseFunc (mainMouse);
    //glutMotionFunc (mainMotion);

    TransferFunctionGLUTWindow::tfInit(main_winx, main_winy);
    transferFunctionWindow=glutCreateSubWindow(mainWindow, 0, 0, tf_winx, tf_winy);
    glutDisplayFunc(TransferFunctionGLUTWindow::tfDisplay);
    glutReshapeFunc(TransferFunctionGLUTWindow::tfReshape);
    glutMouseFunc(TransferFunctionGLUTWindow::tfMouse);
    glutKeyboardFunc(TransferFunctionGLUTWindow::tfKeyboard);
    glutMotionFunc (TransferFunctionGLUTWindow::tfMotion);

    ColorGradientGLUTWindow::cmInit(main_winx, main_winy);
    colorMapWindow=glutCreateSubWindow(mainWindow, 0, main_winy-cm_winy, cm_winx, cm_winy);
    //ColorGradientGLUTWindow::createGLUIWidgets();        
    glutDisplayFunc(ColorGradientGLUTWindow::cmDisplay);
    glutReshapeFunc(ColorGradientGLUTWindow::cmReshape);
    glutMouseFunc(ColorGradientGLUTWindow::cmMouse);
    glutMotionFunc(ColorGradientGLUTWindow::cmMotion);
    glutKeyboardFunc(ColorGradientGLUTWindow::cmKeyboard);

    GLUI_Master.set_glutIdleFunc(mainIdle);

    ColorPalette::cpInit();

    //int tmp= glutGetWindow();

    //load a initial trasfer function
    //for bluntfin dataset
    //MainTransferFunctionWindow::loadFile("bluntfin_tf_1");
    //glutSetWindow(tmp);
}

void MainTransferFunctionWindow::mainDisplay()
{
    glutSetWindow(transferFunctionWindow);
    glutPostRedisplay();
    glutSetWindow(colorMapWindow);
    glutPostRedisplay();
    glutSetWindow(colorPaletteWindow);
    glutPostRedisplay();
}

void MainTransferFunctionWindow::mainMouse(int button, int state, int x, int y)
{
    fprintf(stderr, "main mouse\n");
    //find out what area received the event
    if (y<=main_winy-tf_winy){
        //transfer function received the event
        glutSetWindow(transferFunctionWindow);
        TransferFunctionGLUTWindow::tfMouse(button, state, x, y);
    }
}

void MainTransferFunctionWindow::mainMotion(int x, int y)
{
    ////////////////////////////////////////
    //glutSetWindow(transferFunctionWindow);
    //TransferFunctionGLUTWindow::tfMotion(x, y);
    /////////////////////////////////////
}

void MainTransferFunctionWindow::mainIdle()
{
    /*
      glutSetWindow(mainWindow);
      glutPostRedisplay();
      glutSetWindow(transferFunctionWindow);
      glutPostRedisplay();
      glutSetWindow(colorMapWindow);
      glutPostRedisplay();
      glutSetWindow(colorPaletteWindow);
      glutPostRedisplay();
    */
}

void MainTransferFunctionWindow::mainKeyboard(unsigned char key, int x, int y)
{
    switch(key) {    
    case 'q':
        exit(0);
        break;    
    case 'a':
        glutSetWindow(transferFunctionWindow);
        tf_pointEditState=1;
        break;
    case 'd':
        glutSetWindow(transferFunctionWindow);
        tf_pointEditState=2;
        if (tf_gvSelectedPoint!=NULL) {
            TransferFunctionGLUTWindow::removePoint(tf_gvSelectedPoint);
            tf_gvSelectedPoint=0;
            TransferFunctionGLUTWindow::WriteControlPoints();
            glutPostRedisplay();
        }
        tf_pointEditState=0;
        break;   
    case 'p':
        TransferFunctionGLUTWindow::printPoints();
        break;
    case 'w':
        TransferFunctionGLUTWindow::printInterpolation();
        break; 
    case 'r':
        TransferFunctionGLUTWindow::readHist();
        break; 
    case 'u':
        TransferFunctionGLUTWindow::dumpHist();
        break; 
    case 'i':
        TransferFunctionGLUTWindow::printHist();
        break;
    default:
        break;
    }  
}

void MainTransferFunctionWindow::mainDestroy()
{
}

void MainTransferFunctionWindow::mainReshape(int x, int y)
{
    if (x==0 || y==0)
        return;

    main_winx=x;
    main_winy=y;

    //change size and location of the subwindow
    cm_winx=main_winx;
    //cm_winy=main_winy/3;
    cm_winy=100;

    glutSetWindow(colorMapWindow);
    glutPositionWindow(0, main_winy-cm_winy);
    glutReshapeWindow(cm_winx, cm_winy);
    glutPostRedisplay();

    //change size and location of the subwindow
    tf_winx=main_winx;
    //tf_winy=main_winy*2/3;
    tf_winy=main_winy-100;

    glutSetWindow(transferFunctionWindow);
    glutPositionWindow(0,0);
    glutReshapeWindow(tf_winx, tf_winy);
    glutPostRedisplay();
}

//////////////////////////////////////////////////////////
//FILE STRUCTURE:
//
//<first section>
//first line: number_of_opacity_control_points
//following: each line is a control point
//control point coordinate is in range [0,1], inclusive
//empty line signals the end of first section 
//
//<second section>
//first line: number_of_color_control_points
//following: each line is a control point
//only x is recorded, control point coordinate is in range [0,1], inclusive
//empty line signals the end of second section 
//
//<third section>
//each line is a color in RGB
//each color value is in range [0,1], inclusive
//"END" signals the end of file
//////////////////////////////////////////////////////////// 

void MainTransferFunctionWindow::loadFile(char* fileName)
{
    if (fileName==0 || strlen(fileName)<1) {
        fprintf(stderr, "Error: file name not supplied.\n");
        return;
    }

    FILE* fp=fopen(fileName, "r");

    if (!fp) {
        fprintf(stderr, "Error: open file.\n");
        return;
    }

    fprintf(stderr, "File \"%s\" opened.\n", fileName);

    char buf[300];                        //buffer to store one line from file
    char* token;                        //pointer to token in the buffer
    char seps[]=" \n";                //seperators

    int num_of_control_point;
    int num_of_color_point;
    double control_points[200];
    double color_points[100];
    double colors[300];

    //clean up old control points
    TransferFunctionGLUTWindow::cleanUpPoints();

    fgets(buf, 100, fp);
    token=strtok(buf, seps);
    num_of_control_point=(int)atof(token);

    //read control points
    for(int i=0; i<num_of_control_point; i++){
        fgets(buf, 100, fp);
        token=strtok(buf, seps);
        control_points[2*i]=atof(token)*tf_unitWidth+15;
                
        token=strtok(NULL, seps);
        control_points[2*i+1]=(atof(token)*(tf_winy-15))+15;
                
        //add points
        TransferFunctionGLUTWindow::addPoint(control_points[2*i], control_points[2*i+1]);
    }
    //rewrite the interpolatoin
    TransferFunctionGLUTWindow::WriteControlPoints();
        
    fgets(buf, 100, fp); //empty line

    fgets(buf, 100, fp);
    token=strtok(buf, seps);
    num_of_color_point=(int)atof(token);

    //read color points
    for(int i=0; i<num_of_color_point; i++){
        fgets(buf, 100, fp);
        token=strtok(buf, seps);
        color_points[i]=atof(token)*cm_unitWidth+15;
    }

    //clean map
    map->cleanUp();

    fgets(buf, 100, fp); //empty line

    //read colors
    for(int i=0; i<num_of_color_point; i++){
        fgets(buf, 100, fp);
        token=strtok(buf, seps);
        colors[3*i]=atof(token);
                
        token=strtok(NULL, seps);
        colors[3*i+1]=atof(token);

        token=strtok(NULL, seps);
        colors[3*i+2]=atof(token);

        map->addKey(color_points[i], new Color(colors[3*i], colors[3*i+1], colors[3*i+2]));

    }
    //rewrite the interpolatoin
    ColorGradientGLUTWindow::WriteControlPoints();

    glutSetWindow(transferFunctionWindow);
    TransferFunctionGLUTWindow::tfDisplay();

    //read modelview settings
    fgets(buf, 100, fp); //empty line
        
    //camero location
    fgets(buf, 100, fp);
    token=strtok(buf, seps);
    //eye_shift_x=atof(token);
        
    token=strtok(NULL, seps);
    //eye_shift_y=atof(token);

    token=strtok(NULL, seps);
    //eyedistance=atof(token);
        
    fgets(buf, 100, fp); //empty line
        
    //object center location
    fgets(buf, 100, fp);
    token=strtok(buf, seps);
    //obj_center[0]=atof(token);
        
    token=strtok(NULL, seps);
    //obj_center[1]=atof(token);

    token=strtok(NULL, seps);
    //obj_center[2]=atof(token);

    fgets(buf, 100, fp); //empty line
        
    //scale and field_of_view
    fgets(buf, 100, fp);
    token=strtok(buf, seps);
    control_point_scale=atof(token);

    token=strtok(NULL, seps);
    //fovy=atof(token);

    if (colorPaletteWindow!=-1) {
        glutSetWindow(colorPaletteWindow);
        ColorPalette::cpSyncLive();
    }

    fclose(fp);
        
    //update transfer function values
    if (transferFunctionWindow!=-1) {
        TransferFunctionGLUTWindow::WriteControlPoints();
    }

    //render the volume again.
    //glutSetWindow(render_window);
    glutPostRedisplay();
}

void MainTransferFunctionWindow::saveFile(char* fileName)
{
    // Check for bad param
    if (fileName == NULL)
        return;
    if (strlen(fileName) < 1)
        return;

    FILE* fp = fopen(fileName, "w");

    if (!fp)
        return; //error

    fprintf(fp, "%d\n", tf_numOfPoints);

    ControlPoint* cur=tf_pointList;

    for (int i=0; i<tf_numOfPoints; i++) {
        fprintf(fp, "%g %g\n", ((double)(cur->x-15))/((double)tf_unitWidth), ((double)(cur->y-15))/((double)tf_winy-15));        
        cur=cur->next;
    }
    fprintf(fp, "\n");

    fprintf(fp, "%d\n", map->numOfKeys);
    cur=map->keyList;
    for (int i=0; i<map->numOfKeys; i++) {
        fprintf(fp, "%g\n", ((double)(cur->x-15))/((double)cm_unitWidth));
        cur=cur->next;
    }
    fprintf(fp, "\n");

    Color* curColor=map->keyColors;
    for (int i=0; i<map->numOfKeys; i++) {
        fprintf(fp, "%g %g %g\n", curColor->R, curColor->G, curColor->B);
        curColor=curColor->next;
    }
    fprintf(fp, "\n");

    //eye location
    //fprintf(fp, "%g %g %g\n\n", eye_shift_x, eye_shift_y, eyedistance);

    //object center
    //fprintf(fp, "%g %g %g\n\n", obj_center[0], obj_center[1], obj_center[2]);

    //bluntfin quater
    //fprintf(fp, "%g %g %g %g\n\n", bluntfin_quater.s, bluntfin_quater.x, bluntfin_quater.y, bluntfin_quater.z);

    //scale and field_of_view
    //fprintf(fp, "%g %g\n", control_point_scale, fovy);

    fprintf(fp, "END\n");
        
    fclose(fp);
}
