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
#include <stdio.h>
#include <assert.h>

#include "ColorGradient.h"
#include "MainWindow.h"
#include "TransferFunctionMain.h"
#include "ColorGradientGLUTWindow.h"
#include "TransferFunctionGLUTWindow.h"

ColorGradient::ColorGradient()
{
    numOfKeys = 2;
    //keyColors = new Color[numOfKeys];


    //color list with two initial colors
    keyColors = new Color(1,1,1);
    keyColors->next = new Color(1,1,1);

    //keyList with two initial points at 0 and 1
    keyList = new ControlPoint(15, main_winy/3);
    keyList->next= new ControlPoint(15+tf_winx*0.95, main_winy/3);
}

ColorGradient::~ColorGradient()
{
}

ControlPoint* ColorGradient::addKey(double x, Color* color)
{
    if (color==0) 
        return 0;

    Color* curColor=keyColors;
    Color* preColor=curColor;
    ControlPoint* curKey=keyList;
    ControlPoint* preKey=curKey;

    if (x==15+cm_unitWidth) {//only update color for the last point
        while(curColor->next!=0){
            preColor=curColor;
            curKey=curKey->next;
            curColor=curColor->next;
        }
        preColor->next=color;
        delete(curColor);
        return curKey;
    } else if (x==15) {
        //only update color for the first point
        color->next=keyColors->next;
        delete(keyColors);
        keyColors=color;
        return keyList;
    }


    curColor=keyColors;
    preColor=curColor;
    curKey=keyList;
    preKey=curKey;

    ControlPoint* newPoint=0;
    while (curKey!=0) {
        if (x<=curKey->x) {
            newPoint=new ControlPoint(x,100);
            newPoint->next=preKey->next;
            preKey->next=newPoint;
            color->next=preColor->next;
            preColor->next=color;
            numOfKeys++;

            //printf("Key added");
            break;
        }
        preKey=curKey;
        curKey=curKey->next;
        preColor=curColor;
        curColor=curColor->next;
    }

    glutSetWindow(colorMapWindow);
    glutPostRedisplay();
    //cm_gvSelectedPoint=newPoint;
    //newPoint->selected=true;
    return newPoint;
}

void ColorGradient::removeKey(double x)
{
    ControlPoint* cur=keyList;
    ControlPoint* pre=cur;
    Color* curColor=keyColors;
    Color* preColor=curColor;

    //if x is the edge point, only reset the color to white
    if (x==15) {
        keyColors->R=1; keyColors->G=1; keyColors->B=1;
        return;
    } else if (x==15+cm_unitWidth) {
        while (curColor->next!=0) {
            curColor=curColor->next;
        }
        curColor->R=1; curColor->G=1; curColor->B=1;
    }

    curColor=keyColors;
    preColor=curColor;
    while (cur->next!=0) {
        if (cur->x==x) {
            pre->next=cur->next;
            delete(cur);
            preColor->next=curColor->next;
            delete(curColor);
            numOfKeys--;
            break;
        }
        pre=cur;
        cur=cur->next;
        preColor=curColor;
        curColor=curColor->next;
    }

    //TransferFunctionGlutWindow::WriteControlPoints();
    ColorGradientGLUTWindow::WriteControlPoints();
    glutSetWindow(colorMapWindow);
    glutPostRedisplay();
}

void ColorGradient::changeColor(double r, double g, double b)
{
    if (cm_gvSelectedPoint==0)
        return;

    Color* curColor=map->keyColors;
    ControlPoint* curPoint=map->keyList;
    while (curPoint!=cm_gvSelectedPoint){
        curPoint=curPoint->next;
        curColor=curColor->next;
    }

    curColor->R=r;
    curColor->G=g;
    curColor->B=b;

    ColorGradientGLUTWindow::WriteControlPoints();

    glutSetWindow(colorMapWindow);
    glutPostRedisplay();
}

void ColorGradient::cleanUp()
{
    if (numOfKeys<2)
        return;

    Color* curColor=keyColors->next;
    Color* preColor=keyColors;

    if (numOfKeys==2){
        preColor->R=1; preColor->G=1; preColor->B=1;
        curColor->R=1; curColor->G=1; curColor->B=1;
        return;
    }

    while (curColor->next!=0){
        if(preColor!=keyColors)
            delete(preColor);
        preColor=curColor;
        curColor=curColor->next;
    }

    delete(preColor);
    keyColors->next=curColor;

    keyColors->R=1; keyColors->B=1; keyColors->B=1;
    keyColors->next->R=1; keyColors->next->G=1; keyColors->next->B=1;

    ControlPoint* cur=keyList->next;
    ControlPoint* pre=keyList;

    while (cur->next!=0) {
        if(pre!=keyList)
            delete(pre);
        pre=cur;
        cur=cur->next;
    }

    delete(pre);
    keyList->next=cur;

    keyList->x=15;
    keyList->next->x=15+cm_unitWidth;
    numOfKeys=2;

    ColorGradientGLUTWindow::WriteControlPoints();
}

void ColorGradient::ReadColorGradientFile(char *fileName)
{
    /*
    FILE *gradFile;

    float *f1;
    int  NumberOfGradients;
    int  i;

    if( (gradFile = fopen(fileName, "rb") ) == NULL) {
        printf("Can't open color gradient file %s.\n", fileName);
        return;
    }

    if( fscanf(gradFile, "%d", &NumberOfGradients) <= 0) {
        printf("Read color gradient file error (File: %s). \n", fileName);
            return;
    }

    // if (keyColors!=NULL) 
    //     delete keyColors;

    // if (keyPoints!=NULL)
    //     delete keyPoints;

    numOfKeys = NumberOfGradients;
    currentKeys = 0;


    keyColors = new Color[numOfKeys];
    keyPoints = new double[numOfKeys];


    assert(NumberOfGradients > 0);
	
    float flag_f = -1.0;
    f1 = new float[NumberOfGradients * 4];
    for(i = 0; i <  NumberOfGradients; i++) {
        if( fscanf(gradFile, "%f%f%f%f", f1+i*4, f1+i*4+1, f1+i*4+2, f1+i*4+3)!= 4 ) {
            printf("Read color gradient file error (File: %s). \n", fileName);
            return;
        }

        assert(f1[i*4] >= 0.0 && f1[i*4] <=1.0);
        assert(f1[i*4+1] >= 0.0 && f1[i*4+1] <=1.0);
        assert(f1[i*4+2] >= 0.0 && f1[i*4+2] <=1.0);
        assert(f1[i*4+3] >= 0.0 && f1[i*4+3] <=1.0);

        //All of the addKey's must be in increasing order
        if (flag_f >= f1[i*4]) {
            printf("Color gradient should be in increasing order. \n");
            return;
        }
    }

    for (i = 0; i <  NumberOfGradients; i++) {
        //this->addKey(f1[i*4], Color(f1[i*4+1], f1[i*4+2],f1[i*4+3]));
    }

    if (gradFile)
        fclose(gradFile);
    */
}

void ColorGradient::SaveColorGradientFile(char *fileName)
{
    /*
    FILE *gradFile;
	
    int  i;
	
    if( (gradFile = fopen(fileName, "wt") ) == NULL) {
        printf("Can't open file for writing, this be some serious error.\n");
        assert(false);
    }

    fprintf(gradFile, "%d\n", numOfKeys);

    for (i=0; i<numOfKeys; i++) {
        fprintf(gradFile, "%.3lf %.3lf %.3lf %.3lf\n", keyPoints[i], keyColors[i].R, keyColors[i].G, keyColors[i].B); 
    }

    fclose(gradFile);
    */
}

void ColorGradient::scaleKeys(double scaleX, int cm_unitWidth)
{
    ControlPoint* cur=keyList;

    while (cur->next!=0) {
        cur->x=(cur->x-15)*scaleX+15;
        cur=cur->next;
    }

    cur->x=15+cm_unitWidth;
}
