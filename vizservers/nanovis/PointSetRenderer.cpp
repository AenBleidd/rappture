#include "Nv.h"
#include <GL/gl.h>
#include "PointSetRenderer.h"
#include <PCASplit.h>
#include <imgLoaders/Image.h>
#include <imgLoaders/ImageLoaderFactory.h>
#include <imgLoaders/ImageLoader.h>
#include <stdio.h>
#include <R2/R2FilePath.h>

PointSetRenderer::PointSetRenderer()
{
    _shader = new PointShader();
    R2string path = R2FilePath::getInstance()->getPath("particle2.bmp");
    if (path.getLength() == 0)
    {
        printf("ERROR : file not found - %s\n", (const char*) path);
        fflush(stdout);
        return;
    }
    
    ImageLoader* loader = ImageLoaderFactory::getInstance()->createLoader("bmp");
    Image* image = loader->load(path, Image::IMG_RGBA);

    if (image)
    {
        _pointTexture = new Texture2D(image->getWidth(), image->getHeight(), GL_UNSIGNED_BYTE, GL_LINEAR,    
                                4, (float*) image->getImageBuffer());
    }
    else
    {
        printf("fail to load image [%s]\n", "particles2.bmp");
    }

    delete loader;

    _bucketSort = new PCA::BucketSort(1024);
}

void PointSetRenderer::renderPoints(PCA::Point* points, int length)
{
    PCA::Point* p = points;
    for (int i = 0; i < length; ++i)
    {
        glColor4f(p->color.x, p->color.y, p->color.z, p->color.w);
        glVertex3f(p->position.x, p->position.y, p->position.z);
    
        ++p;
    }
}

void PointSetRenderer::renderCluster(PCA::ClusterList** bucket, int size, int level)
{
    float quadratic[] = { 1.0f, 0.0f, 0.01f };


    glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, quadratic);
    glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 60.0f);
    glPointParameterfARB(GL_POINT_SIZE_MIN_ARB, 1.0f);
    glPointParameterfARB(GL_POINT_SIZE_MAX_ARB, 100);
    glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);

    glEnable(GL_POINT_SPRITE_ARB);

    bool setSize = false;
    glBegin(GL_POINTS);

    PCA::ClusterList* p;
    for (int i = size - 1; i >= 0; --i)
    {
        p = bucket[i];
        if (p)
        {
            if (!setSize) 
            {
                _shader->setScale(p->data->points[0].size);
                setSize = true;
            }
        }

        while (p)
        {
            renderPoints(p->data->points, p->data->numOfPoints);

            p = p->next;
        }
        
    }

    glEnd();

    glDisable(GL_POINT_SPRITE_ARB);

}

void PointSetRenderer::render(PCA::ClusterAccel* cluster, const Mat4x4& mat, int sortLevel)
{
    _bucketSort->init();
    _bucketSort->sort(cluster, mat, sortLevel);

    _pointTexture->activate();
    _shader->bind();
    glPushMatrix();
        glTranslatef(-0.5f, -0.5f, -0.5f);
        renderCluster(_bucketSort->getBucket(), _bucketSort->getSize(), 4);
    glPopMatrix();

    _shader->unbind();
    _pointTexture->deactivate();
}

