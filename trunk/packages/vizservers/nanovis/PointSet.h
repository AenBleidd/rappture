
#ifndef __POINT_SET_H__
#define __POINT_SET_H__

#include <PCASplit.h>
#include <Vector4.h>
#include <Vector3.h>

class PointSet {
    unsigned int _sortLevel;
    PCA::ClusterAccel* _cluster;

    Vector3 _scale;
    Vector3 _origin;
    float _max;
    float _min;
    bool _visible;
public :
    PointSet();
    ~PointSet();

public :
    void initialize(Vector4* values, const unsigned int count, const Vector3& scale, const Vector3& origin, float min, float max);
    bool isVisible() const;
    void setVisible(bool visible);
    unsigned int getSortLevel()const;
    PCA::ClusterAccel* getCluster();
    void updateColor(float* color, int  count);
    const Vector3& getScale() const;
    Vector3& getScale();
    const Vector3& getOrigin() const;
    Vector3& getOrigin();
};

inline bool PointSet::isVisible() const
{
    return _visible;
}

inline void PointSet::setVisible(bool visible)
{
    _visible = visible;
}

inline unsigned int PointSet::getSortLevel()const
{
    return _sortLevel;
}

inline PCA::ClusterAccel* PointSet::getCluster()
{
    return _cluster;
}

inline Vector3& PointSet::getScale()
{
    return _scale;
}

inline const Vector3& PointSet::getScale() const
{
    return _scale;
}

inline Vector3& PointSet::getOrigin()
{
    return _origin;
}

inline const Vector3& PointSet::getOrigin() const
{
    return _origin;
}

#endif // 
