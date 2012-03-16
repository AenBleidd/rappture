/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <GL/glew.h>

#include <Image.h>
#include <ImageLoaderFactory.h>
#include <ImageLoader.h>
#include <R2/R2FilePath.h>

#include "PointSetRenderer.h"
#include "PCASplit.h"

#define USE_TEXTURE 
//#define USE_SHADER 
#define POINT_SIZE 5

PointSetRenderer::PointSetRenderer()
{
    _shader = new PointShader();
    const char *path = R2FilePath::getInstance()->getPath("particle2.bmp");
    if (path == NULL) {
        ERROR("pointset file not found - %s\n", path);
        return;
    }

    ImageLoader *loader = ImageLoaderFactory::getInstance()->createLoader("bmp");
    Image *image = loader->load(path, Image::IMG_RGBA);
    delete [] path;
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
        ERROR("fail to load image [%s]\n", "particles2.bmp");
    }

    delete loader;
    delete image;
    _bucketSort = new PCA::BucketSort(1024);
}

void PointSetRenderer::renderPoints(PCA::Point *points, int length)
{
    PCA::Point *p = points;
    for (int i = 0; i < length; ++i, ++p) {
        glColor4f(p->color.x, p->color.y, p->color.z, p->color.w);
        glVertex3f(p->position.x, p->position.y, p->position.z);
    }
}

void PointSetRenderer::renderCluster(PCA::ClusterList** bucket, int size, int level)
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

    glEnable(GL_POINT_SPRITE_ARB);

    bool setSize = false;
    glBegin(GL_POINTS);

    PCA::ClusterList* p;
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

void PointSetRenderer::render(PCA::ClusterAccel *cluster, const Mat4x4& mat,
                              int sortLevel, const Vector3& scale, const Vector3& origin)
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
    Vector3 shift(origin.x + scale.x * 0.5, origin.x + scale.x * 0.5, origin.x + scale.x * 0.5);
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
