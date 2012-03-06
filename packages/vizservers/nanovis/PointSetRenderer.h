/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __POINT_SET_RENDERER_H__
#define __POINT_SET_RENDERER_H__

#include "PCASplit.h"
#include "BucketSort.h"
#include "PointShader.h"
#include "Texture2D.h"
#include <Mat4x4.h>

class PointSetRenderer {
    PCA::BucketSort* _bucketSort;
    PointShader* _shader;
    Texture2D* _pointTexture;
public : 
    PointSetRenderer();
    ~PointSetRenderer();

private :
    void renderPoints(PCA::Point* points, int length);
    void renderCluster(PCA::ClusterList** bucket, int size, int level);

public :
    void render(PCA::ClusterAccel* cluster, const Mat4x4& mat, int sortLevel, const Vector3& scale, const Vector3& origin);
};

#endif

