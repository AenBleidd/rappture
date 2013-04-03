/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <GL/glew.h>

#include <Image.h>
#include <ImageLoaderFactory.h>
#include <ImageLoader.h>
#include <util/FilePath.h>

#include "PointSetRenderer.h"
#include "PCASplit.h"

using namespace nv;
using namespace nv::util;
using namespace vrmath;
using namespace PCA;

#define USE_TEXTURE 
//#define USE_SHADER 
#define POINT_SIZE 5

PointSetRenderer::PointSetRenderer()
{
    _shader = new PointShader();
    std::string path = FilePath::getInstance()->getPath("particle2.bmp");
    if (path.empty()) {
        ERROR("Particle image not found");
        return;
    }

    ImageLoader *loader = ImageLoaderFactory::getInstance()->createLoader("bmp");
    Image *image = loader->load(path.c_str(), Image::IMG_RGBA);
    unsigned char *bytes = (unsigned char *)image->getImageBuffer();
    if (bytes) {
        for (unsigned int y = 0; y < image->getHeight(); ++y) {
            for (unsigned int x = 0; x < image->getWidth(); ++x, bytes += 4) {
                bytes[3] = (bytes[0] == 0) ? 0 : 255;
            }
        }
    }

    if (image) {
        _pointTexture = new Texture2D(image->getWidth(), image->getHeight(), 
                                      GL_UNSIGNED_BYTE, GL_LINEAR,    
                                      4, image->getImageBuffer());
    } else {
        ERROR("Failed to load particle image");
    }

    delete loader;
    delete image;
    _bucketSort = new BucketSort(1024);
}

PointSetRenderer::~PointSetRenderer()
{
}

void PointSetRenderer::renderPoints(Point *points, int length)
{
    Point *p = points;
    for (int i = 0; i < length; ++i, ++p) {
        glColor4f(p->color.x, p->color.y, p->color.z, p->color.w);
        glVertex3f(p->position.x, p->position.y, p->position.z);
    }
}

void PointSetRenderer::renderCluster(ClusterList** bucket, int size, int level)
{
    float quadratic[] = { 1.0f, 0.0f, 0.01f };

    glEnable(GL_POINT_SPRITE_ARB);
    glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, quadratic);
    glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 60.0f);
    glPointParameterfARB(GL_POINT_SIZE_MIN_ARB, 1.0f);
    glPointParameterfARB(GL_POINT_SIZE_MAX_ARB, 100);
#ifdef USE_TEXTURE 
    glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
#endif

    bool setSize = false;
    glBegin(GL_POINTS);

    ClusterList *p;
    for (int i = size - 1; i >= 0; --i) {
        p = bucket[i];
        if (p) {
            if (!setSize) {
#ifdef USE_SHADER 
                _shader->setScale(p->data->points[0].size);
#endif
                setSize = true;
            }
        }

        while (p) {
            renderPoints(p->data->points, p->data->numOfPoints);

            p = p->next;
        }
    }

    glEnd();

    glDisable(GL_POINT_SPRITE_ARB);
    glPointSize(1);
}

void PointSetRenderer::render(ClusterAccel *cluster, const Matrix4x4d& mat,
                              int sortLevel, const Vector3f& scale, const Vector3f& origin)
{
    _bucketSort->init();
    _bucketSort->sort(cluster, mat, sortLevel);

    glDisable(GL_TEXTURE_2D);

#ifdef USE_TEXTURE 
    _pointTexture->activate();
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#endif

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);

    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glPushMatrix();
    float s = 1.0f / scale.x;
    Vector3f shift(origin.x + scale.x * 0.5, origin.x + scale.x * 0.5, origin.x + scale.x * 0.5);
    glScalef(s, scale.y / scale.x * s, scale.z / scale.x * s);

    //glTranslatef(-shift.x, -shift.y, -shift.z);

#ifdef USE_SHADER 
    _shader->bind();
#else
    glPointSize(POINT_SIZE);
#endif
    renderCluster(_bucketSort->getBucket(), _bucketSort->getSize(), 4);
#ifdef USE_SHADER 
    _shader->unbind();
#else
    glPointSize(1.0f);
#endif

    glPopMatrix();

    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);

#ifdef USE_TEXTURE 
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    _pointTexture->deactivate();
#endif
}
