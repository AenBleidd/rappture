/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef POINT_SET_RENDERER_H
#define POINT_SET_RENDERER_H

#include "PCASplit.h"
#include "BucketSort.h"
#include "PointShader.h"
#include "Texture2D.h"
#include "Mat4x4.h"

class PointSetRenderer
{
 public: 
    PointSetRenderer();
    ~PointSetRenderer();

    void render(PCA::ClusterAccel *cluster, const Mat4x4& mat,
                int sortLevel, const Vector3& scale, const Vector3& origin);

private:
    void renderPoints(PCA::Point *points, int length);

    void renderCluster(PCA::ClusterList **bucket, int size, int level);

    PCA::BucketSort *_bucketSort;
    PointShader *_shader;
    Texture2D *_pointTexture;
};

#endif
