/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef POINT_SET_RENDERER_H
#define POINT_SET_RENDERER_H

#include <vrmath/Vector3f.h>
#include <vrmath/Matrix4x4d.h>

#include "PCASplit.h"
#include "BucketSort.h"
#include "PointShader.h"
#include "Texture2D.h"

namespace nv {

class PointSetRenderer
{
public:
    PointSetRenderer();
    ~PointSetRenderer();

    void render(PCA::ClusterAccel *cluster, const vrmath::Matrix4x4d& mat,
                int sortLevel, const vrmath::Vector3f& scale, const vrmath::Vector3f& origin);

private:
    void renderPoints(PCA::Point *points, int length);

    void renderCluster(PCA::ClusterList **bucket, int size, int level);

    PCA::BucketSort *_bucketSort;
    PointShader *_shader;
    Texture2D *_pointTexture;
};

}

#endif
