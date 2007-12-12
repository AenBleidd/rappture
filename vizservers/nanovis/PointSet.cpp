#include "PointSet.h"
#include "PCASplit.h"
#include <stdlib.h>

PointSet::PointSet()
: _visible(false), _cluster(0), _sortLevel(4)
{
}

PointSet::~PointSet()
{
    if (_cluster)
    {
        delete _cluster;
    }
}

void PointSet::initialize(Vector4* values, const unsigned int count)
{
    PCA::PCASplit pcaSplit;

    PCA::Point* points = (PCA::Point*) malloc(sizeof(PCA::Point) * count);
    for (unsigned int i = 0; i < count; ++i)
    {
        points[i].position.set(values[i].x, values[i].y, values[i].z);

        // TBD
        points[i].color.set(1.0f, 1.0f, 1.0f, 0.2f);

        points[i].value = values[i].w;
    }

    if (_cluster != 0)
    {
        delete _cluster;
    }

    
    _cluster = pcaSplit.doIt(points, count);
}

void PointSet::updateColor()
{
    
}

