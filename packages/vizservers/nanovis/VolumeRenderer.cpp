/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * VolumeRenderer.cpp : VolumeRenderer class for volume visualization
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdlib.h>
#include <float.h>

#include <vector>

#include <GL/glew.h>

#include <vrmath/Vector3f.h>
#include <vrmath/Matrix4x4d.h>

#include "nanovis.h"
#include "VolumeRenderer.h"
#include "Plane.h"
#include "ConvexPolygon.h"
#include "NvStdVertexShader.h"
#include "Trace.h"

using namespace nv;
using namespace vrmath;

VolumeRenderer::VolumeRenderer()
{
    initShaders();

    _volumeInterpolator = new VolumeInterpolator();
}

VolumeRenderer::~VolumeRenderer()
{
    delete _cutplaneShader;
    delete _zincBlendeShader;
    delete _regularVolumeShader;
    delete _stdVertexShader;
    delete _volumeInterpolator;
}

//initialize the volume shaders
void VolumeRenderer::initShaders()
{
    _cutplaneShader = new NvShader();
    _cutplaneShader->loadVertexProgram("cutplane_vp.cg", "main");
    _cutplaneShader->loadFragmentProgram("cutplane_fp.cg", "main");

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
    for (NanoVis::VolumeHashmap::iterator itr = NanoVis::volumeTable.begin();
         itr != NanoVis::volumeTable.end(); ++itr) {
        Volume *volume = itr->second;
        if (!volume->visible()) {
            continue; // Skip this volume
        }
        // BE CAREFUL: Set the number of slices to something slightly
        // different for each volume.  If we have identical volumes at exactly
        // the same position with exactly the same number of slices, the
        // second volume will overwrite the first, so the first won't appear
        // at all.
        volumes.push_back(volume);
        volume->numSlices(256 - volumes.size());
    }

    glPushAttrib(GL_ENABLE_BIT);

    //two dimension pointer array
    ConvexPolygon ***polys = new ConvexPolygon**[volumes.size()];
    //number of actual slices for each volume
    size_t *actual_slices = new size_t[volumes.size()];
    float *z_steps = new float[volumes.size()];

    TRACE("start loop %d", volumes.size());
    for (size_t i = 0; i < volumes.size(); i++) {
        Volume *volume = volumes[i];
        polys[i] = NULL;
        actual_slices[i] = 0;

        int n_slices = volume->numSlices();
        if (volume->isosurface()) {
            // double the number of slices
            n_slices <<= 1;
        }

        //volume start location
        Vector3f volPos = volume->location();
        Vector3f volScaling = volume->getPhysicalScaling();

        TRACE("VOL POS: %g %g %g",
              volPos.x, volPos.y, volPos.z);
        TRACE("VOL SCALE: %g %g %g",
              volScaling.x, volScaling.y, volScaling.z);

        double x0 = 0;
        double y0 = 0;
        double z0 = 0;

        Matrix4x4d model_view_no_trans, model_view_trans;
        Matrix4x4d model_view_no_trans_inverse, model_view_trans_inverse;

        //initialize volume plane with world coordinates
        nv::Plane volume_planes[6];
        volume_planes[0].setCoeffs( 1,  0,  0, -x0);
        volume_planes[1].setCoeffs(-1,  0,  0,  x0+1);
        volume_planes[2].setCoeffs( 0,  1,  0, -y0);
        volume_planes[3].setCoeffs( 0, -1,  0,  y0+1);
        volume_planes[4].setCoeffs( 0,  0,  1, -z0);
        volume_planes[5].setCoeffs( 0,  0, -1,  z0+1);

        //get modelview matrix with no translation
        glPushMatrix();
        glScalef(volScaling.x, volScaling.y, volScaling.z);

        glEnable(GL_DEPTH_TEST);

        GLdouble mv_no_trans[16];
        glGetDoublev(GL_MODELVIEW_MATRIX, mv_no_trans);

        model_view_no_trans = Matrix4x4d(mv_no_trans);
        model_view_no_trans_inverse = model_view_no_trans.inverse();

        glPopMatrix();

        //get modelview matrix with translation
        glPushMatrix();
        glTranslatef(volPos.x, volPos.y, volPos.z);
        glScalef(volScaling.x, volScaling.y, volScaling.z);

        GLdouble mv_trans[16];
        glGetDoublev(GL_MODELVIEW_MATRIX, mv_trans);

        model_view_trans = Matrix4x4d(mv_trans);
        model_view_trans_inverse = model_view_trans.inverse();

        model_view_trans.print();

        //draw volume bounding box with translation (the correct location in
        //space)
        if (volume->outline()) {
            float olcolor[3];
            volume->getOutlineColor(olcolor);
            drawBoundingBox(x0, y0, z0, x0+1, y0+1, z0+1,
                (double)olcolor[0], (double)olcolor[1], (double)olcolor[2],
                1.5);
        }
        glPopMatrix();

        // transform volume_planes to eye coordinates.
        // Need to transform without translation since we don't want
        // to translate plane normals, just rotate them
        for (size_t j = 0; j < 6; j++) {
            volume_planes[j].transform(model_view_no_trans);
        }
        double eyeMinX, eyeMaxX, eyeMinY, eyeMaxY, zNear, zFar;
        getEyeSpaceBounds(model_view_no_trans, 
                          eyeMinX, eyeMaxX,
                          eyeMinY, eyeMaxY,
                          zNear, zFar);

        //compute actual rendering slices
        float z_step = fabs(zNear-zFar)/n_slices;
        z_steps[i] = z_step;
        size_t n_actual_slices;

        if (volume->dataEnabled()) {
            if (z_step == 0.0f)
                n_actual_slices = 1;
            else
                n_actual_slices = (int)(fabs(zNear-zFar)/z_step + 1);
            polys[i] = new ConvexPolygon*[n_actual_slices];
        } else {
            n_actual_slices = 0;
            polys[i] = NULL;
        }
        actual_slices[i] = n_actual_slices;

        TRACE("near: %g far: %g eye space bounds: (%g,%g)-(%g,%g) z_step: %g slices: %d",
              zNear, zFar, eyeMinX, eyeMaxX, eyeMinY, eyeMaxY, z_step, n_actual_slices);

        Vector4f vert1, vert2, vert3, vert4;

        // Render cutplanes first with depth test enabled.  They will mark the
        // image with their depth values.  Then we render other volume slices.
        // These volume slices will be occluded correctly by the cutplanes and
        // vice versa.

        for (int j = 0; j < volume->getCutplaneCount(); j++) {
            if (!volume->isCutplaneEnabled(j)) {
                continue;
            }
            float offset = volume->getCutplane(j)->offset;
            int axis = volume->getCutplane(j)->orient;

            switch (axis) {
            case 1:
                vert1 = Vector4f(offset, 0, 0, 1);
                vert2 = Vector4f(offset, 1, 0, 1);
                vert3 = Vector4f(offset, 1, 1, 1);
                vert4 = Vector4f(offset, 0, 1, 1);
                break;
            case 2:
                vert1 = Vector4f(0, offset, 0, 1);
                vert2 = Vector4f(1, offset, 0, 1);
                vert3 = Vector4f(1, offset, 1, 1);
                vert4 = Vector4f(0, offset, 1, 1);
                break;
            case 3:
            default:
                vert1 = Vector4f(0, 0, offset, 1);
                vert2 = Vector4f(1, 0, offset, 1);
                vert3 = Vector4f(1, 1, offset, 1);
                vert4 = Vector4f(0, 1, offset, 1);
                break;
            }

            Vector4f texcoord1 = vert1;
            Vector4f texcoord2 = vert2;
            Vector4f texcoord3 = vert3;
            Vector4f texcoord4 = vert4;

            _cutplaneShader->bind();
            _cutplaneShader->setFPTextureParameter("volume", volume->textureID());
            _cutplaneShader->setFPTextureParameter("tf", volume->transferFunction()->id());

            glPushMatrix();
            glTranslatef(volPos.x, volPos.y, volPos.z);
            glScalef(volScaling.x, volScaling.y, volScaling.z);
            _cutplaneShader->setGLStateMatrixVPParameter("modelViewProjMatrix",
                                                         NvShader::MODELVIEW_PROJECTION_MATRIX);
            glPopMatrix();

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            glBegin(GL_QUADS);
            glTexCoord3f(texcoord1.x, texcoord1.y, texcoord1.z);
            glVertex3f(vert1.x, vert1.y, vert1.z);
            glTexCoord3f(texcoord2.x, texcoord2.y, texcoord2.z);
            glVertex3f(vert2.x, vert2.y, vert2.z);
            glTexCoord3f(texcoord3.x, texcoord3.y, texcoord3.z);
            glVertex3f(vert3.x, vert3.y, vert3.z);
            glTexCoord3f(texcoord4.x, texcoord4.y, texcoord4.z);
            glVertex3f(vert4.x, vert4.y, vert4.z);
            glEnd();

            glDisable(GL_DEPTH_TEST);
            _cutplaneShader->disableFPTextureParameter("tf");
            _cutplaneShader->disableFPTextureParameter("volume");
            _cutplaneShader->unbind();
        } //done cutplanes

        // Now prepare proxy geometry slices

        // Initialize view-aligned quads with eye space bounds of
        // volume
        vert1 = Vector4f(eyeMinX, eyeMinY, -0.5, 1);
        vert2 = Vector4f(eyeMaxX, eyeMinY, -0.5, 1);
        vert3 = Vector4f(eyeMaxX, eyeMaxY, -0.5, 1);
        vert4 = Vector4f(eyeMinX, eyeMaxY, -0.5, 1);

        size_t counter = 0;

        // Transform slices and store them
        float slice_z;
        for (size_t j = 0; j < n_actual_slices; j++) {
            slice_z = zFar + j * z_step; //back to front

            ConvexPolygon *poly = new ConvexPolygon();
            polys[i][counter] = poly;
            counter++;

            poly->vertices.clear();
            poly->setId(i);

            // Set eye space Z-coordinate of slice
            vert1.z = slice_z;
            vert2.z = slice_z;
            vert3.z = slice_z;
            vert4.z = slice_z;

            poly->appendVertex(vert1);
            poly->appendVertex(vert2);
            poly->appendVertex(vert3);
            poly->appendVertex(vert4);

            for (size_t k = 0; k < 6; k++) {
                if (!poly->clip(volume_planes[k], true))
                    break;
            }

            if (poly->vertices.size() >= 3) {
                poly->transform(model_view_no_trans_inverse);
                poly->transform(model_view_trans);
                total_rendered_slices++;
            }
        }
    } //iterate all volumes
    TRACE("end loop");

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
    // Non pre-multiplied alpha
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    for (size_t i = 0; i < total_rendered_slices; i++) {
        Volume *volume = NULL;

        int volume_index = slices[i].volumeId;
        int slice_index = slices[i].sliceId;
        ConvexPolygon *currentSlice = polys[volume_index][slice_index];
        float z_step = z_steps[volume_index];

        volume = volumes[volume_index];

        Vector3f volScaling = volume->getPhysicalScaling();

        glPushMatrix();
        glScalef(volScaling.x, volScaling.y, volScaling.z);

        // FIXME: compute view-dependent volume sample distance
        double avgSampleDistance = 1.0 / pow(volume->width() * volScaling.x * 
                                             volume->height() * volScaling.y * 
                                             volume->depth() * volScaling.z, 1.0/3.0);
        float sampleRatio = z_step / avgSampleDistance;

#ifdef notdef
        TRACE("shading slice: volume %s addr=%x slice=%d, volume=%d z_step=%g avgSD=%g", 
              volume->name(), volume, slice_index, volume_index, z_step, avgSampleDistance);
#endif
        activateVolumeShader(volume, false, sampleRatio);
        glPopMatrix();

        glBegin(GL_POLYGON);
        currentSlice->emit(true);
        glEnd();

        deactivateVolumeShader();
    }

    glPopAttrib();

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
    delete[] z_steps;
    free(slices);
}

void 
VolumeRenderer::drawBoundingBox(float x0, float y0, float z0, 
                                float x1, float y1, float z1,
                                float r, float g, float b, 
                                float line_width)
{
    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

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

    glPopMatrix();
    glPopAttrib();
}

void 
VolumeRenderer::activateVolumeShader(Volume *volume, bool sliceMode,
                                     float sampleRatio)
{
    //vertex shader
    _stdVertexShader->bind();
    TransferFunction *transferFunc  = volume->transferFunction();
    if (volume->volumeType() == Volume::CUBIC) {
        _regularVolumeShader->bind(transferFunc->id(), volume, sliceMode, sampleRatio);
    } else if (volume->volumeType() == Volume::ZINCBLENDE) {
        _zincBlendeShader->bind(transferFunc->id(), volume, sliceMode, sampleRatio);
    }
}

void VolumeRenderer::deactivateVolumeShader()
{
    _stdVertexShader->unbind();
    _regularVolumeShader->unbind();
    _zincBlendeShader->unbind();
}

void VolumeRenderer::getEyeSpaceBounds(const Matrix4x4d& mv,
                                       double& xMin, double& xMax,
                                       double& yMin, double& yMax,
                                       double& zNear, double& zFar)
{
    double x0 = 0;
    double y0 = 0;
    double z0 = 0;
    double x1 = 1;
    double y1 = 1;
    double z1 = 1;

    double zMin, zMax;
    xMin = DBL_MAX;
    xMax = -DBL_MAX;
    yMin = DBL_MAX;
    yMax = -DBL_MAX;
    zMin = DBL_MAX;
    zMax = -DBL_MAX;

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
        Vector4f eyeVert = mv.transform(Vector4f(vertex[i][0],
                                                 vertex[i][1],
                                                 vertex[i][2],
                                                 vertex[i][3]));
        if (eyeVert.x < xMin) xMin = eyeVert.x;
        if (eyeVert.x > xMax) xMax = eyeVert.x;
        if (eyeVert.y < yMin) yMin = eyeVert.y;
        if (eyeVert.y > yMax) yMax = eyeVert.y;
        if (eyeVert.z < zMin) zMin = eyeVert.z;
        if (eyeVert.z > zMax) zMax = eyeVert.z;
    }

    zNear = zMax;
    zFar = zMin;
}
