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
#include <math.h>

#include "ColorPaletteWindow.h"
#include "ColorGradientGLUTWindow.h"
#include "TransferFunctionGLUTWindow.h"
#include "MainWindow.h"
#include "TransferFunctionMain.h"

int cpWinId;
int cp_winx, cp_winy;
int cp_color_r, cp_color_g, cp_color_b; //the RGB model
int cp_color_H, cp_color_S, cp_color_B; //the HSB model
int color_model;
float control_point_scale=1.0;
GLUI * cp_glui;
GLUI_Spinner *color_r, *color_g, *color_b, *color_H, *color_S, *color_B;
GLUI_StaticText *eye_distance_label, *fps_label;

namespace ColorPaletteWindow {
    int  cpwin_pos_x = 100;
    int  cpwin_pos_y = 425;
    enum colorMode{RGB, GRAY, HSB};
    Color* hue_color;
    ControlPoint* color_point_1;
    ControlPoint* color_point_2;

    //char fileName[255];
    static char* fileName= new char[255];// (char*) malloc(sizeof(char)*256);
};

using namespace ColorPaletteWindow;

ColorPalette::ColorPalette()
{
}

ColorPalette::~ColorPalette()
{
}

void ColorPalette::cpSyncLive()
{
    if (cp_glui!=0) {
        cp_glui->sync_live();
    }
}

void ColorPalette::cpInit()
{
    color_model = 0;

    //fileName=(char*) malloc(sizeof(char)*30);
    for (int i=0;i<200;i++) {
        fileName[i]=0;
    }
    cp_winx = 430;
    cp_winy = 350;

    hue_color=new Color(1,0,0);
    color_point_1=new ControlPoint(0,cp_winy);
    color_point_2=new ControlPoint(0,2*cp_winy/3-15);

    glutInitWindowPosition( cpwin_pos_x, cpwin_pos_y);
    glutInitWindowSize( cp_winx, cp_winy );

    colorPaletteWindow = glutCreateWindow("Color Palette");

    glutDisplayFunc( cpDisplay );
    glutReshapeFunc( cpReshape );
    glutKeyboardFunc ( cpKeyboard );
    glutIdleFunc (cpIdle);
    glutMouseFunc (cpMouse);
    glutMotionFunc (cpMotion);

    GLUI_Master.set_glutReshapeFunc(cpReshape);
    GLUI_Master.set_glutDisplayFunc( cpDisplay );
    GLUI_Master.set_glutKeyboardFunc(cpKeyboard);
    GLUI_Master.set_glutSpecialFunc(NULL);
    GLUI_Master.set_glutMouseFunc(cpMouse);
    GLUI_Master.set_glutIdleFunc(cpIdle);

    createGLUIWidgets();
}

void update_tf_texture();

void ColorPalette::cmdHandler(int arg)
{
    switch (arg) {
    case 1: //add color 
        cm_editState=1;
        break;
    case 2:
        if (cm_gvSelectedPoint!=0)
            map->changeColor((double)cp_color_r/255, (double)cp_color_g/255, (double)cp_color_b/255);
        update_tf_texture();
        break;
    case 3:
        if (cm_gvSelectedPoint!=0)
            map->removeKey(cm_gvSelectedPoint->x);
        update_tf_texture();
        break;
    case 4:
        MainTransferFunctionWindow::loadFile(fileName);
        break;
    case 5:
        MainTransferFunctionWindow::saveFile(fileName);
        break;
    case 6:
        map->cleanUp();
        update_tf_texture();
        break;
    case 7:
        TransferFunctionGLUTWindow::cleanUpPoints();    
        //map->cleanUp();
        update_tf_texture();
        break;
    case 8:
        if (color_model==0) {
            color_r->enable();
            color_g->enable();
            color_b->enable();

            cp_color_r=255;
            cp_color_g=0;
            cp_color_b=0;

            color_H->disable();
            color_S->disable();
            color_B->disable();

            cp_color_H=0;
            cp_color_S=0;
            cp_color_B=0;

            color_point_1->x=0;
            color_point_1->y=cp_winy;

            cp_glui->sync_live();
        } else if (color_model==1) {
            color_r->enable();
            color_g->enable();
            color_b->enable();

            cp_color_r=0;
            cp_color_g=0;
            cp_color_b=0;

            color_H->disable();
            color_S->disable();
            color_B->disable(); 

            cp_color_H=0;
            cp_color_S=0;
            cp_color_B=0;

            color_point_1->x=0;
            color_point_1->y=cp_winy;

            cp_glui->sync_live();
        } else if (color_model==2) {
            color_r->enable();
            color_g->enable();
            color_b->enable();

            cp_color_r=255;
            cp_color_g=255;
            cp_color_b=255;

            color_H->enable();
            color_S->enable();
            color_B->enable();  

            cp_color_H=0;
            cp_color_S=100;
            cp_color_B=100;

            color_point_1->x=0;
            color_point_1->y=cp_winy;

            color_point_2->x=0;

            cp_glui->sync_live();
        }

        glutSetWindow(colorPaletteWindow);
        glutPostRedisplay();
        break;
    case 9:
        break;
    case 10:
        break;
    default:
        break;
    }
}

void ColorPalette::openFile()
{
}

void ColorPalette::createGLUIWidgets()
{
    cp_glui = GLUI_Master.create_glui_subwindow(colorPaletteWindow, GLUI_SUBWINDOW_BOTTOM);
    cp_glui->set_main_gfx_window(colorPaletteWindow);
    //cp_glui->add_column( false );
    cp_glui->add_statictext("RGB Color:");
    color_r= cp_glui->add_spinner("R:", GLUI_SPINNER_INT, &cp_color_r);         
    color_r->set_int_limits(0, 255, GLUI_LIMIT_CLAMP);
    color_r->set_alignment(GLUI_ALIGN_CENTER);
    color_g= cp_glui->add_spinner("G:", GLUI_SPINNER_INT, &cp_color_g);
    color_g->set_int_limits(0, 255, GLUI_LIMIT_CLAMP);
    color_g->set_alignment(GLUI_ALIGN_CENTER);
    color_b = cp_glui->add_spinner("B:", GLUI_SPINNER_INT, &cp_color_b);
    color_b->set_int_limits(0, 255, GLUI_LIMIT_CLAMP);
    color_b->set_alignment(GLUI_ALIGN_CENTER);

    cp_glui->add_statictext("HSB Color:");
    color_H= cp_glui->add_spinner("H:Deg.", GLUI_SPINNER_INT, &cp_color_H);             
    color_H->set_int_limits(0, 360, GLUI_LIMIT_CLAMP);
    color_H->set_alignment(GLUI_ALIGN_CENTER);
    color_H->disable();
    color_S= cp_glui->add_spinner("S:%", GLUI_SPINNER_INT, &cp_color_S);
    color_S->set_int_limits(0, 100, GLUI_LIMIT_CLAMP);
    color_S->set_alignment(GLUI_ALIGN_CENTER);
    color_S->disable();
    color_B = cp_glui->add_spinner("B:%", GLUI_SPINNER_INT, &cp_color_B);
    color_B->set_int_limits(0, 100, GLUI_LIMIT_CLAMP);
    color_B->set_alignment(GLUI_ALIGN_CENTER);
    color_B->disable();

    cp_glui->add_column(true);

    GLUI_Panel* model_panel=cp_glui->add_panel("Color Model", GLUI_PANEL_EMBOSSED);
    GLUI_RadioGroup* model=cp_glui->add_radiogroup_to_panel(model_panel,&color_model, 8, cmdHandler);
    cp_glui->add_radiobutton_to_group(model, "RGB");
    cp_glui->add_radiobutton_to_group(model, "Grayscale");
    cp_glui->add_radiobutton_to_group(model, "HSB");

    cp_glui->add_statictext("Color Controls:");
    //cp_glui->add_button("Add Color", 1, cmdHandler);
    cp_glui->add_button("Set Color", 2, cmdHandler);
    cp_glui->add_button("Remove Color", 3, cmdHandler);

    cp_glui->add_separator();

    cp_glui->add_button("Reset Colormap", 6, cmdHandler);
    cp_glui->add_button("Reset TF", 7, cmdHandler);

    cp_glui->add_column(true);
    cp_glui->add_statictext("File:");
    cp_glui->add_edittext("Name: ",GLUI_EDITTEXT_TEXT, fileName);
    cp_glui->add_button("Load", 4, cmdHandler);
    cp_glui->add_button("Save", 5, cmdHandler);

    cp_glui->add_separator();
    cp_glui->add_spinner("Scale:", GLUI_SPINNER_FLOAT, &control_point_scale);
    eye_distance_label = cp_glui->add_statictext("Eye Distance: "); 
    fps_label = cp_glui->add_statictext("FPS: ");
}

void ColorPalette::cpIdle()
{
    //glutSetWindow(mainWindow);
    //glutPostRedisplay();
}

void ColorPalette::confirm(int arg)
{
    if (cm_gvSelectedPoint!=0)
        map->changeColor((double)cp_color_r/255, (double)cp_color_g/255, (double)cp_color_b/255);
    /*
      glutSetWindow(mainWindow);
      glutDestroyWindow(colorPaletteWindow);
      glutPostRedisplay();
      colorPaletteWindow=-1;
    */
}

void ColorPalette::cpDisplay()
{
    //draw selectPoint
    //drawSelectPoint();

    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glColor3d(0.0, 0.0, 0.0);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);

    if (color_model==RGB) {
        Color* leftColor = new Color(1, 0, 0);
        Color* rightColor = new Color(0, 0, 1);

        GLfloat left_color[3];
        GLfloat right_color[3];

        leftColor->GetRGB(left_color);
        rightColor->GetRGB(right_color);

        GLfloat rainbowColor[18] = {
            1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1
        };

        int i=0;
        float unitWidth = cp_winx/6;

        glBegin(GL_QUADS);
        for (i=0; i<6; i++) {            

            glColor3f(rainbowColor[3*i], rainbowColor[3*i+1], rainbowColor[3*i+2]);
            glVertex2d(unitWidth*i, 0);
            glVertex2d(unitWidth*i, cp_winy);

            if (i==5) {
                glColor3f(rainbowColor[0], rainbowColor[1], rainbowColor[2]);
                glVertex2d(cp_winx, cp_winy);
                glVertex2d(cp_winx, 0);
            } else {
                glColor3f(rainbowColor[3*i+3], rainbowColor[3*i+4], rainbowColor[3*i+5]);
                glVertex2d(unitWidth*(i+1), cp_winy);
                glVertex2d(unitWidth*(i+1), 0);
            }
        }
        glEnd();

        //draw color point
        glColor3d(0,0,0);
        color_point_1->glDraw_3();
    } else if (color_model==GRAY) {
        glBegin(GL_QUADS);              
        glColor3f(0, 0, 0);
        glVertex2d(0, 0);
        glVertex2d(0, cp_winy);

        glColor3f(1, 1, 1);
        glVertex2d(cp_winx, cp_winy);
        glVertex2d(cp_winx, 0);
        glEnd();

        glColor3d(1,0,0);
        color_point_1->glDraw_3();
    } else if(color_model==HSB) {
        glBegin(GL_QUADS);              
        glColor3f(0, 0, 0);
        glVertex2d(0, 2*cp_winy/3);
        glColor3f(1, 1, 1);
        glVertex2d(0, cp_winy);

        glColor3f(hue_color->R, hue_color->G, hue_color->B);
        glVertex2d(cp_winx, cp_winy);
        glColor3f(0, 0, 0);
        glVertex2d(cp_winx, 2*cp_winy/3);
        glEnd();

        //draw rainbow
        Color* leftColor = new Color(1, 0, 0);
        Color* rightColor = new Color(0, 0, 1);

        GLfloat left_color[3];
        GLfloat right_color[3];

        leftColor->GetRGB(left_color);
        rightColor->GetRGB(right_color);

        GLfloat rainbowColor[18] = {
            1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1
        };

        int i=0;
        float unitWidth = cp_winx/6;

        glBegin(GL_QUADS);
        for (i=0; i<6; i++) {            
            glColor3f(rainbowColor[3*i], rainbowColor[3*i+1], rainbowColor[3*i+2]);
            glVertex2d(unitWidth*i, 2*cp_winy/3-15);
            glVertex2d(unitWidth*i, 2*cp_winy/3);

            if (i==5){
                glColor3f(rainbowColor[0], rainbowColor[1], rainbowColor[2]);
                glVertex2d(cp_winx, 2*cp_winy/3);
                glVertex2d(cp_winx, 2*cp_winy/3-15);
            } else {
                glColor3f(rainbowColor[3*i+3], rainbowColor[3*i+4], rainbowColor[3*i+5]);
                glVertex2d(unitWidth*(i+1), 2*cp_winy/3);
                glVertex2d(unitWidth*(i+1), 2*cp_winy/3-15);
            }
        }
        glEnd();

        glColor3d(1-hue_color->R, 1-hue_color->G, 1-hue_color->B);
        color_point_1->glDraw_3();

        glColor3d(0,0,0);
        color_point_2->glDraw_2();
        /*
        //draw bar
        glLineWidth(1.0);
        glBegin(GL_LINES);
        glVertex2d(0, cp_winy/2-1);
        glVertex2d(cp_winx, cp_winy/2-1);
        glEnd();
        */

        /*
          i=0;
          Color* curColor=map->keyColors;
          ControlPoint* curKey=map->keyList;
          for (i=0; i<map->numOfKeys ; i++){
          //glColor3d(curColor->R, curColor->G, curColor->B);
          glColor3d(0.0, 0.0, 0.0);
          curKey->glDraw_2();
          curColor=curColor->next;
          curKey=curKey->next;
          }
        */
    }

    glColor3f(0, 0, 0);
    glutSwapBuffers();
}

void ColorPalette::cpReshape(int x, int y)
{
    //printf("cpreshape=%d",x);

    if (x==0 || y==0)
        return;

    cp_winx=x;
    cp_winy=y;
    glViewport( 0, 0, x, y );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Set up most convenient projection
    gluOrtho2D(0, x, 0, y);
    glutPostRedisplay();
}

void ColorPalette::cpKeyboard(unsigned char key, int x, int y)
{
    //glutPostRedisplay();
}

void ColorPalette::cpMouse(int button, int state, int x, int y){

    GLint viewportVector[4];
    GLdouble scX, scY; //the coordinates of selection point on rainbow

    glGetIntegerv(GL_VIEWPORT, viewportVector);

    scX = x;
    scY = viewportVector[3] - (GLint) y - 1;

    if (color_model!=HSB){
        color_point_1->x=scX;
        color_point_1->y=scY;
    } else { //HSB
        if (scY>=2*cp_winy/3) {
            color_point_1->x=scX;
            color_point_1->y=scY;
            getColor(scX, scY);
        }
        if (scY<2*cp_winy/3) {
            color_point_2->x=scX;
            //color_point_1->x=0;
            //color_point_1->y=cp_winy;
            getColor(scX, color_point_2->y);
        }
    }
    //printf("(%g, %g)\n", scX, scY);

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        getColor(scX, scY);
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
    } else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) {
    } else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP){
    }

    cp_glui->sync_live();
    glutPostRedisplay();
}

void ColorPalette::cpMotion(int x, int y)
{
}

//interpolate GRB color
Color ColorPalette::getColor(double x, double y)
{
    if (color_model==RGB) {
        float unitWidth = cp_winx/6;            
        GLfloat rainbowColor[21] = {1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0};

        int pos = (int)floor(x/unitWidth);
        int red_left = (int)rainbowColor[3*pos];
        int red_right = (int)rainbowColor[3*(pos+1)];

        int green_left = (int)rainbowColor[3*pos+1];
        int green_right = (int)rainbowColor[3*(pos+1)+1];

        int blue_left = (int)rainbowColor[3*pos+2];
        int blue_right = (int)rainbowColor[3*(pos+1)+2];

        double r, g, b;
        r = (red_left+(x/unitWidth-pos)*(red_right-red_left));
        g = (green_left+(x/unitWidth-pos)*(green_right-green_left));
        b = (blue_left+(x/unitWidth-pos)*(blue_right-blue_left));

        cp_color_r = (int)(255.0*r);
        cp_color_g = (int)(255.0*g);
        cp_color_b = (int)(255.0*b);

        cp_glui->sync_live();
        return Color(r,g,b);
    } else if (color_model==GRAY) {
        float t=((float)x)/((float)cp_winx);
        double r, g, b;
        r = t;
        g = t;
        b = t;

        cp_color_r = (int)(255.0*r);
        cp_color_g = (int)(255.0*g);
        cp_color_b = (int)(255.0*b);

        cp_glui->sync_live();
        return Color(r,g,b);
    } else if (color_model==HSB) {
        double r, g, b;
        if (y==color_point_2->y) {
            float unitWidth = cp_winx/6;                
            GLfloat rainbowColor[21] = {1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0};

            int pos = (int)floor(x/unitWidth);
            int red_left = (int)rainbowColor[3*pos];
            int red_right = (int)rainbowColor[3*(pos+1)];

            int green_left = (int)rainbowColor[3*pos+1];
            int green_right = (int)rainbowColor[3*(pos+1)+1];

            int blue_left = (int)rainbowColor[3*pos+2];
            int blue_right = (int)rainbowColor[3*(pos+1)+2];
                
            r = (red_left+(x/unitWidth-pos)*(red_right-red_left));
            g = (green_left+(x/unitWidth-pos)*(green_right-green_left));
            b = (blue_left+(x/unitWidth-pos)*(blue_right-blue_left));

            cp_color_H= (int)(360.0*(x/(double)cp_winx));

            hue_color->R=r;
            hue_color->G=g;
            hue_color->B=b;
        } else {
            cp_color_S=(int)(100.0*(1-(double)color_point_1->x/(double)cp_winx));
            cp_color_B=(int)(100.0*((double)(color_point_1->y-(double)(2*cp_winy/3))/((double)cp_winy/3)));
        }

        //convert HSB to RGB

        //1. horizontal interpolation:
        //upper left: (1,1,1)
        //upper right: hue color
        r=1.0+(double)color_point_1->x/((double)cp_winx)*(double)(hue_color->R-1);
        g=1.0+(double)color_point_1->x/((double)cp_winx)*(double)(hue_color->G-1);
        b=1.0+(double)color_point_1->x/((double)cp_winx)*(double)(hue_color->B-1);

        //2. vertical interpolation:
        //buttom (0,0,0)
        r += ((double)cp_winy-(double)color_point_1->y)/((double)cp_winy/3)*(-r);
        g += ((double)cp_winy-(double)color_point_1->y)/((double)cp_winy/3)*(-g);
        b += ((double)cp_winy-(double)color_point_1->y)/((double)cp_winy/3)*(-b);

        cp_color_r = (int)(255.0*r);
        cp_color_g = (int)(255.0*g);
        cp_color_b = (int)(255.0*b);

        //printf("(%d,%d,%d)    ", cp_color_r, cp_color_g, cp_color_b);

        cp_glui->sync_live();

        return Color(r,g,b);
    }

    return Color(0,0,0);
}
