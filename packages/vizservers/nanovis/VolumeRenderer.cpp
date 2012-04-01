/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * VolumeRenderer.cpp : VolumeRenderer class for volume visualization
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
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>

#include <GL/glew.h>
#include <GL/glut.h>

#include <tcl.h>

#include <vector>

#include <R2/R2string.h>
#include <R2/R2FilePath.h>

#include "nanovis.h"
#include "VolumeRenderer.h"
#include "ConvexPolygon.h"

#include "NvStdVertexShader.h"
#include "Trace.h"
#include "Grid.h"

#define NUMDIGITS	6

VolumeRenderer::VolumeRenderer() :
    _sliceMode(false),
    _volumeMode(true)
{
    initShaders();

    const char *path = R2FilePath::getInstance()->getPath("Font.bmp");
    if (path == NULL) {
        ERROR("can't find Font.bmp\n");
        assert(path != NULL);
    }
    initFont(path);
    delete [] path;
    _volumeInterpolator = new VolumeInterpolator();
}

VolumeRenderer::~VolumeRenderer()
{
    delete _zincBlendeShader;
    delete _regularVolumeShader;
    delete _stdVertexShader;
    delete _volumeInterpolator;
}

//initialize the volume shaders
void VolumeRenderer::initShaders()
{
    //standard vertex program
    _stdVertexShader = new NvStdVertexShader();

    //volume rendering shader: one cubic volume
    _regularVolumeShader = new NvRegularVolumeShader();

    //volume rendering shader: one zincblende orbital volume.
    //This shader renders one orbital of the simulation.
    //A sim has S, P, D, SS orbitals. thus a full rendering requires 4 zincblende orbital volumes. 
    //A zincblende orbital volume is decomposed into 2 "interlocking" cubic 4-component volumes and passed to the shader. 
    //We render each orbital with a independent transfer functions then blend the result.
    //
    //The engine is already capable of rendering multiple volumes and combine them. Thus, we just invoke this shader on
    //S, P, D and SS orbitals with different transfor functions. The result is a multi-orbital rendering.
    _zincBlendeShader = new NvZincBlendeVolumeShader();
}

struct SortElement {
    float z;
    int volumeId;
    int sliceId;

    SortElement(float _z, int _v, int _s) :
        z(_z),
        volumeId(_v),
        sliceId(_s)
    {}
};

static int sliceSort(const void *a, const void *b)
{
    if ((*((SortElement*)a)).z > (*((SortElement*)b)).z)
        return 1;
    else
        return -1;
}

void 
VolumeRenderer::renderAll()
{
    size_t total_rendered_slices = 0;

    if (_volumeInterpolator->isStarted()) {
#ifdef notdef
        ani_vol = _volumeInterpolator->getVolume();
        ani_tf = ani_vol->transferFunction();
#endif
        TRACE("VOLUME INTERPOLATOR IS STARTED ----------------------------");
    }
    // Determine the volumes that are to be rendered.
    std::vector<Volume *> volumes;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    for (hPtr = Tcl_FirstHashEntry(&NanoVis::volumeTable, &iter); hPtr != NULL;
         hPtr = Tcl_NextHashEntry(&iter)) {
        Volume* volPtr;
        volPtr = (Volume *)Tcl_GetHashValue(hPtr);
        if (!volPtr->visible()) {
            continue; // Skip this volume
        }
        // BE CAREFUL: Set the number of slices to something slightly
        // different for each volume.  If we have identical volumes at exactly
        // the same position with exactly the same number of slices, the
        // second volume will overwrite the first, so the first won't appear
        // at all.
        volumes.push_back(volPtr);
        volPtr->numSlices(256 - volumes.size());
    }

    //two dimension pointer array
    ConvexPolygon ***polys = new ConvexPolygon**[volumes.size()];
    //number of actual slices for each volume
    size_t *actual_slices = new size_t[volumes.size()]; 

    TRACE("start loop %d\n", volumes.size());
    for (size_t i = 0; i < volumes.size(); i++) {
        Volume *volPtr = volumes[i];
        polys[i] = NULL;
        actual_slices[i] = 0;

        int n_slices = volPtr->numSlices();
        if (volPtr->isosurface()) {
            // double the number of slices
            n_slices <<= 1;
        }

        //volume start location
        Vector3 loc = volPtr->location();
        Vector4 shift_4d(loc.x, loc.y, loc.z, 0);

        double x0 = 0;
        double y0 = 0;
        double z0 = 0;

        Mat4x4 model_view_no_trans, model_view_trans;
        Mat4x4 model_view_no_trans_inverse, model_view_trans_inverse;

        double zNear, zFar;

        //initialize volume plane with world coordinates
        Plane volume_planes[6];
        volume_planes[0].set_coeffs( 1,  0,  0, -x0);
        volume_planes[1].set_coeffs(-1,  0,  0,  x0+1);
        volume_planes[2].set_coeffs( 0,  1,  0, -y0);
        volume_planes[3].set_coeffs( 0, -1,  0,  y0+1);
        volume_planes[4].set_coeffs( 0,  0,  1, -z0);
        volume_planes[5].set_coeffs( 0,  0, -1,  z0+1);

        //get modelview matrix with no translation
        glPushMatrix();
        glScalef(volPtr->aspectRatioWidth, 
                 volPtr->aspectRatioHeight, 
                 volPtr->aspectRatioDepth);

        glEnable(GL_DEPTH_TEST);

        GLfloat mv_no_trans[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, mv_no_trans);

        model_view_no_trans = Mat4x4(mv_no_trans);
        model_view_no_trans_inverse = model_view_no_trans.inverse();

        glPopMatrix();

        //get modelview matrix with translation
        glPushMatrix();
        glTranslatef(shift_4d.x, shift_4d.y, shift_4d.z);
        glScalef(volPtr->aspectRatioWidth, 
                 volPtr->aspectRatioHeight, 
                 volPtr->aspectRatioDepth);
        GLfloat mv_trans[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, mv_trans);

        model_view_trans = Mat4x4(mv_trans);
        model_view_trans_inverse = model_view_trans.inverse();

        //draw volume bounding box with translation (the correct location in
        //space)
        if (volPtr->outline()) {
            float olcolor[3];
            volPtr->getOutlineColor(olcolor);
            drawBoundingBox(x0, y0, z0, x0+1, y0+1, z0+1,
                (double)olcolor[0], (double)olcolor[1], (double)olcolor[2],
                1.5);
        }
        glPopMatrix();

        //draw labels
        glPushMatrix();
        glTranslatef(shift_4d.x, shift_4d.y, shift_4d.z);
        if(volPtr->outline()) {
            //draw_label(i);
        }
        glPopMatrix();

        //transform volume_planes to eye coordinates.
        for (size_t j = 0; j < 6; j++) {
            volume_planes[j].transform(model_view_no_trans);
        }
        getNearFarZ(mv_no_trans, zNear, zFar);

        //compute actual rendering slices
        float z_step = fabs(zNear-zFar)/n_slices;
        size_t n_actual_slices;

        if (volPtr->dataEnabled()) {
            n_actual_slices = (int)(fabs(zNear-zFar)/z_step + 1);
            polys[i] = new ConvexPolygon*[n_actual_slices];
        } else {
            n_actual_slices = 0;
            polys[i] = NULL;
        }
        actual_slices[i] = n_actual_slices;

        Vector4 vert1 = (Vector4(-10, -10, -0.5, 1));
        Vector4 vert2 = (Vector4(-10, +10, -0.5, 1));
        Vector4 vert3 = (Vector4(+10, +10, -0.5, 1));
        Vector4 vert4 = (Vector4(+10, -10, -0.5, 1));

        // Render cutplanes first with depth test enabled.  They will mark the
        // image with their depth values.  Then we render other volume slices.
        // These volume slices will be occluded correctly by the cutplanes and
        // vice versa.

        ConvexPolygon static_poly;
        for (int j = 0; j < volPtr->getCutplaneCount(); j++) {
            if (!volPtr->isCutplaneEnabled(j)) {
                continue;
            }
            float offset = volPtr->getCutplane(j)->offset;
            int axis = volPtr->getCutplane(j)->orient;
            
            if (axis == 3) {
                vert1 = Vector4(-10, -10, offset, 1);
                vert2 = Vector4(-10, +10, offset, 1);
                vert3 = Vector4(+10, +10, offset, 1);
                vert4 = Vector4(+10, -10, offset, 1);
                //continue;
            } else if (axis == 1) {
                vert1 = Vector4(offset, -10, -10, 1);
                vert2 = Vector4(offset, +10, -10, 1);
                vert3 = Vector4(offset, +10, +10, 1);
                vert4 = Vector4(offset, -10, +10, 1);
                //continue;
            } else if (axis == 2) {
                vert1 = Vector4(-10, offset, -10, 1);
                vert2 = Vector4(+10, offset, -10, 1);
                vert3 = Vector4(+10, offset, +10, 1);
                vert4 = Vector4(-10, offset, +10, 1);
                //continue;
            }

            vert1 = model_view_no_trans.transform(vert1);
            vert2 = model_view_no_trans.transform(vert2);
            vert3 = model_view_no_trans.transform(vert3);
            vert4 = model_view_no_trans.transform(vert4);

            ConvexPolygon *p = &static_poly;
            p->vertices.clear();

            p->appendVertex(vert1);
            p->appendVertex(vert2);
            p->appendVertex(vert3);
            p->appendVertex(vert4);

            for (size_t k = 0; k < 6; k++) {
                p->clip(volume_planes[k], true);
            }

            p->transform(model_view_no_trans_inverse);
            p->transform(model_view_trans);

            glPushMatrix();
            glScalef(volPtr->aspectRatioWidth,
                     volPtr->aspectRatioHeight,
                     volPtr->aspectRatioDepth);

            activateVolumeShader(volPtr, true);
            glPopMatrix();

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            glBegin(GL_POLYGON);
            p->emit(true); 
            glEnd();
            glDisable(GL_DEPTH_TEST);

            deactivateVolumeShader();
        } //done cutplanes

        //Now do volume rendering

        vert1 = (Vector4(-10, -10, -0.5, 1));
        vert2 = (Vector4(-10, +10, -0.5, 1));
        vert3 = (Vector4(+10, +10, -0.5, 1));
        vert4 = (Vector4(+10, -10, -0.5, 1));

        size_t counter = 0;

        //transform slices and store them
        float slice_z;
        for (size_t j = 0; j < n_actual_slices; j++) {
            slice_z = zFar + j * z_step; //back to front

            ConvexPolygon *poly = new ConvexPolygon();
            polys[i][counter] = poly;
            counter++;

            poly->vertices.clear();
            poly->setId(i);

            //Setting Z-coordinate 
            vert1.z = slice_z;
            vert2.z = slice_z;
            vert3.z = slice_z;
            vert4.z = slice_z;

            poly->appendVertex(vert1);
            poly->appendVertex(vert2);
            poly->appendVertex(vert3);
            poly->appendVertex(vert4);

            for (size_t k = 0; k < 6; k++) {
                poly->clip(volume_planes[k], true);
            }

            poly->transform(model_view_no_trans_inverse);
            poly->transform(model_view_trans);

            if (poly->vertices.size() >= 3)
                total_rendered_slices++;
        }
    } //iterate all volumes
    TRACE("end loop\n");

    // We sort all the polygons according to their eye-space depth, from
    // farthest to the closest.  This step is critical for correct blending

    SortElement *slices = (SortElement *)
        malloc(sizeof(SortElement) * total_rendered_slices);

    size_t counter = 0;
    for (size_t i = 0; i < volumes.size(); i++) {
        for (size_t j = 0; j < actual_slices[i]; j++) {
            if (polys[i][j]->vertices.size() >= 3) {
                slices[counter] = SortElement(polys[i][j]->vertices[0].z, i, j);
                counter++;
            }
        }
    }

    //sort them
    qsort(slices, total_rendered_slices, sizeof(SortElement), sliceSort);

    //Now we are ready to render all the slices from back to front
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    for (size_t i = 0; i < total_rendered_slices; i++) {
        Volume *volPtr = NULL;

        int volume_index = slices[i].volumeId;
        int slice_index = slices[i].sliceId;
        ConvexPolygon *cur = polys[volume_index][slice_index];

        volPtr = volumes[volume_index];

        glPushMatrix();
        glScalef(volPtr->aspectRatioWidth,
                 volPtr->aspectRatioHeight, 
                 volPtr->aspectRatioDepth);

#ifdef notdef
        TRACE("shading slice: volume %s addr=%x slice=%d, volume=%d\n", 
               volPtr->name(), volPtr, slice_index, volume_index);
#endif
        activateVolumeShader(volPtr, false);
        glPopMatrix();

        glBegin(GL_POLYGON);
        cur->emit(true);
        glEnd();

        deactivateVolumeShader();
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    //Deallocate all the memory used
    for (size_t i = 0; i < volumes.size(); i++) {
        for (size_t j = 0; j <actual_slices[i]; j++) {
            delete polys[i][j];
        }
        if (polys[i]) {
            delete[] polys[i];
        }
    }
    delete[] polys;
    delete[] actual_slices;
    free(slices);
}

void 
VolumeRenderer::drawBoundingBox(float x0, float y0, float z0, 
                                float x1, float y1, float z1,
                                float r, float g, float b, 
                                float line_width)
{
    glPushMatrix();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glColor4d(r, g, b, 1.0);
    glLineWidth(line_width);

    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z0);
        glVertex3d(x1, y0, z0);
        glVertex3d(x1, y1, z0);
        glVertex3d(x0, y1, z0);
    }
    glEnd();

    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z1);
        glVertex3d(x1, y0, z1);
        glVertex3d(x1, y1, z1);
        glVertex3d(x0, y1, z1);
    }
    glEnd();

    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z0);
        glVertex3d(x0, y0, z1);
        glVertex3d(x0, y1, z1);
        glVertex3d(x0, y1, z0);
    }
    glEnd();

    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x1, y0, z0);
        glVertex3d(x1, y0, z1);
        glVertex3d(x1, y1, z1);
        glVertex3d(x1, y1, z0);
    }
    glEnd();

#ifdef notdef
    /* Rappture doesn't supply axis units yet. So turn labeling off until we
     * can display the proper units with the distance of each bounding box
     * dimension.
     */
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f); 

    if (NanoVis::fonts != NULL) {
        double mv[16], prjm[16];
        int viewport[4];
        double wx, wy, wz;
        double dx, dy, dz;

        glGetDoublev(GL_MODELVIEW_MATRIX, mv);
        glGetDoublev(GL_PROJECTION_MATRIX, prjm);
        glGetIntegerv(GL_VIEWPORT, viewport);

        NanoVis::fonts->begin();
        dx = x1 - x0;
        dy = y1 - y0;
        dz = z1 - z0;
        if (gluProject((x0 + x1) * 0.5, y0, z0, mv, prjm, viewport, 
                       &wx, &wy, &wz)) {
            char buff[20];
            double min, max;

            NanoVis::grid->xAxis.GetDataLimits(min, max);
            glLoadIdentity();
            glTranslatef((int)wx, viewport[3] - (int) wy, 0.0f);
            const char *units;
            units = NanoVis::grid->xAxis.GetUnits();
            sprintf(buff, "%.*g %s", NUMDIGITS, max - min, units);
            NanoVis::fonts->draw(buff);
        }
        if (gluProject(x0, (y0 + y1) * 0.5, z0, mv, prjm, viewport, &
                       wx, &wy, &wz)) {
            char buff[20];
            double min, max;

            NanoVis::grid->yAxis.GetDataLimits(min, max);
            glLoadIdentity();
            glTranslatef((int)wx, viewport[3] - (int) wy, 0.0f);
            const char *units;
            units = NanoVis::grid->yAxis.GetUnits();
            sprintf(buff, "%.*g %s", NUMDIGITS, max - min, units);
            NanoVis::fonts->draw(buff);
        }
        if (gluProject(x0, y0, (z0 + z1) * 0.5, mv, prjm, viewport, 
                       &wx, &wy, &wz)) {
            glLoadIdentity();
            glTranslatef((int)wx, viewport[3] - (int) wy, 0.0f);

            double min, max;
            NanoVis::grid->zAxis.GetDataLimits(min, max);
            const char *units;
            units = NanoVis::grid->zAxis.GetUnits();
            char buff[20];
            sprintf(buff, "%.*g %s", NUMDIGITS, max - min, units);
            NanoVis::fonts->draw(buff);
        }
        NanoVis::fonts->end();
    };
#endif
    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}

void 
VolumeRenderer::activateVolumeShader(Volume* volPtr, bool sliceMode)
{
    //vertex shader
    _stdVertexShader->bind();
    TransferFunction *tfPtr  = volPtr->transferFunction();
    if (volPtr->volumeType() == Volume::CUBIC) {
        _regularVolumeShader->bind(tfPtr->id(), volPtr, sliceMode);
    } else if (volPtr->volumeType() == Volume::ZINCBLENDE) {
        _zincBlendeShader->bind(tfPtr->id(), volPtr, sliceMode);
    }
}

void VolumeRenderer::deactivateVolumeShader()
{
    _stdVertexShader->unbind();
    _regularVolumeShader->unbind();
    _zincBlendeShader->unbind();
}

void VolumeRenderer::getNearFarZ(const Mat4x4& mv, double& zNear, double& zFar)
{
    double x0 = 0;
    double y0 = 0;
    double z0 = 0;
    double x1 = 1;
    double y1 = 1;
    double z1 = 1;

    double zMin, zMax;
    zMin =  10000;
    zMax = -10000;

    double vertex[8][4];

    vertex[0][0]=x0; vertex[0][1]=y0; vertex[0][2]=z0; vertex[0][3]=1.0;
    vertex[1][0]=x1; vertex[1][1]=y0; vertex[1][2]=z0; vertex[1][3]=1.0;
    vertex[2][0]=x0; vertex[2][1]=y1; vertex[2][2]=z0; vertex[2][3]=1.0;
    vertex[3][0]=x0; vertex[3][1]=y0; vertex[3][2]=z1; vertex[3][3]=1.0;
    vertex[4][0]=x1; vertex[4][1]=y1; vertex[4][2]=z0; vertex[4][3]=1.0;
    vertex[5][0]=x1; vertex[5][1]=y0; vertex[5][2]=z1; vertex[5][3]=1.0;
    vertex[6][0]=x0; vertex[6][1]=y1; vertex[6][2]=z1; vertex[6][3]=1.0;
    vertex[7][0]=x1; vertex[7][1]=y1; vertex[7][2]=z1; vertex[7][3]=1.0;

    for (int i = 0; i < 8; i++) {
        Vector4 tmp = mv.transform(Vector4(vertex[i][0], vertex[i][1], vertex[i][2], vertex[i][3]));
        tmp.perspectiveDivide();
        vertex[i][2] = tmp.z;
        if (vertex[i][2] < zMin) zMin = vertex[i][2];
        if (vertex[i][2] > zMax) zMax = vertex[i][2];
    }

    zNear = zMax;
    zFar = zMin;
}

bool 
VolumeRenderer::initFont(const char *filename) 
{
    FILE *f;
    unsigned short int bfType;
    int bfOffBits;
    short int biPlanes;
    short int biBitCount;
    int biSizeImage;
    int width, height;
    int i;
    unsigned char temp;
    unsigned char* data;
    /* make sure the file is there and open it read-only (binary) */
    f = fopen(filename, "rb");
    if (f == NULL) {
        ERROR("can't open font file \"%s\"\n", filename);
        return false;
    }

    if (fread(&bfType, sizeof(short int), 1, f) != 1) {
        ERROR("can't read %lu bytes from font file \"%s\"\n", 
               (unsigned long)sizeof(short int), filename);
        goto error;
    }

    /* check if file is a bitmap */
    if (bfType != 19778) {
        ERROR("not a bmp file.\n");
        goto error;
    }

    /* get the file size */
    /* skip file size and reserved fields of bitmap file header */
    fseek(f, 8, SEEK_CUR);

    /* get the position of the actual bitmap data */
    if (fread(&bfOffBits, sizeof(int), 1, f) != 1) {
        ERROR("error reading file.\n");
        goto error;
    }
    //TRACE("Data at Offset: %ld\n", bfOffBits);

    /* skip size of bitmap info header */
    fseek(f, 4, SEEK_CUR);

    /* get the width of the bitmap */
    if (fread(&width, sizeof(int), 1, f) != 1) {
        ERROR("error reading file.\n");
        goto error;
    }
    //TRACE("Width of Bitmap: %d\n", texture->width);

    /* get the height of the bitmap */
    if (fread(&height, sizeof(int), 1, f) != 1) {
        ERROR("error reading file.\n");
        goto error;
    }
    //TRACE("Height of Bitmap: %d\n", texture->height);

    /* get the number of planes (must be set to 1) */
    if (fread(&biPlanes, sizeof(short int), 1, f) != 1) {
        ERROR("error reading file.\n");
        goto error;
    }
    if (biPlanes != 1) {
        ERROR("number of Planes not 1!\n");
        goto error;
    }

    /* get the number of bits per pixel */
    if (fread(&biBitCount, sizeof(short int), 1, f) != 1) {
        ERROR("error reading file.\n");
        goto error;
    }

    //TRACE("Bits per Pixel: %d\n", biBitCount);
    if (biBitCount != 24) {
        ERROR("Bits per Pixel not 24\n");
        goto error;
    }

    /* calculate the size of the image in bytes */
    biSizeImage = width * height * 3 * sizeof(unsigned char);
    data = (unsigned char*) malloc(biSizeImage);
    if (data == NULL) {
        ERROR("Can't allocate memory for image\n");
        goto error;
    }

    /* seek to the actual data */
    fseek(f, bfOffBits, SEEK_SET);
    if (fread(data, biSizeImage, 1, f) != 1) {
        ERROR("Error loading file!\n");
        goto error;
    }
    fclose(f);

    /* swap red and blue (bgr -> rgb) */
    for (i = 0; i < biSizeImage; i += 3) {
       temp = data[i];
       data[i] = data[i + 2];
       data[i + 2] = temp;
    }

    //insert alpha channel
    unsigned char* data_with_alpha;
    data_with_alpha = (unsigned char*)
        malloc(width*height*4*sizeof(unsigned char));
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char r, g, b, a;
            r = data[3*(i*width+j)];
            g = data[3*(i*width+j)+1];
            b = data[3*(i*width+j)+2];

            if (r==0 && g==0 && b==0)
                a = 0;
            else
                a = 255;

            data_with_alpha[4*(i*width+j)] = r;
            data_with_alpha[4*(i*width+j) + 1] = g;
            data_with_alpha[4*(i*width+j) + 2] = b;
            data_with_alpha[4*(i*width+j) + 3] = a;
        }
    }
    free(data);

    //create opengl texture 
    glGenTextures(1, &_fontTexture);
    glBindTexture(GL_TEXTURE_2D, _fontTexture);
    //glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_with_alpha);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    free(data_with_alpha);

    buildFont();
    return (glGetError()==0);

 error:
    fclose(f);
    return false;
}

void 
VolumeRenderer::drawLabel(Volume* vol)
{
    //glEnable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    //x
    glColor3f(0.5, 0.5, 0.5);

    int length = vol->label[0].size();
    glPushMatrix();

    glTranslatef(.5 * vol->aspectRatioWidth,
                 vol->aspectRatioHeight,
                 -0.1 * vol->aspectRatioDepth);
    glRotatef(180, 0, 0, 1);
    glRotatef(90, 1, 0, 0);

    glScalef(0.0008, 0.0008, 0.0008);
    for (int i = 0; i < length; i++) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, vol->label[0].c_str()[i]);
        glTranslatef(0.04, 0., 0.);
    }
    glPopMatrix();

    //y
    length = vol->label[1].size();
    glPushMatrix();
    glTranslatef(vol->aspectRatioWidth,
                 0.5*vol->aspectRatioHeight,
                 -0.1*vol->aspectRatioDepth);
    glRotatef(90, 0, 1, 0);
    glRotatef(90, 0, 0, 1);

    glScalef(0.0008, 0.0008, 0.0008);
    for (int i = 0; i < length; i++) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, vol->label[1].c_str()[i]);
        glTranslatef(0.04, 0., 0.);
    }
    glPopMatrix();

    //z
    length = vol->label[2].size();
    glPushMatrix();
    glTranslatef(0.,
                 1. * vol->aspectRatioHeight,
                 0.5 * vol->aspectRatioDepth);
    glRotatef(90, 0, 1, 0);

    glScalef(0.0008, 0.0008, 0.0008);
    for (int i = 0; i < length; i++) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, vol->label[2].c_str()[i]);
        glTranslatef(0.04, 0., 0.);
    }
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

void 
VolumeRenderer::buildFont() 
{
    GLfloat cx, cy;         /* the character coordinates in our texture */
    _fontBase = glGenLists(256);
    glBindTexture(GL_TEXTURE_2D, _fontTexture);
    for (int loop = 0; loop < 256; loop++) {
        cx = (float) (loop % 16) / 16.0f;
        cy = (float) (loop / 16) / 16.0f;
        glNewList(_fontBase + loop, GL_COMPILE);
        glBegin(GL_QUADS);
        glTexCoord2f(cx, 1 - cy - 0.0625f);
        glVertex3f(0, 0, 0);
        glTexCoord2f(cx + 0.0625f, 1 - cy - 0.0625f);
        glVertex3f(0.04, 0, 0);
        glTexCoord2f(cx + 0.0625f, 1 - cy);
        glVertex3f(0.04, 0.04, 0);
        glTexCoord2f(cx, 1 - cy);
        glVertex3f(0, 0.04, 0);
        glEnd();
        glTranslated(0.04, 0, 0);
        glEndList();
    }
}

void
VolumeRenderer::glPrint(char* string, int set)
{
    if (set > 1) {
        set = 1;
    }
    glBindTexture(GL_TEXTURE_2D, _fontTexture);
    glListBase(_fontBase - 32 + (128 * set));
    glCallLists(strlen(string), GL_BYTE, string);
}
