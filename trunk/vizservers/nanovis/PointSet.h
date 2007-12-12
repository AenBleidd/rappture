#ifndef __POINT_SET_H__
#define __POINT_SET_H__

#include <PCASplit.h>
#include <Vector4.h>

class PointSet {
    unsigned int _sortLevel;
    bool _visible;
    PCA::ClusterAccel* _cluster;
public :
    PointSet();
    ~PointSet();

public :
    void initialize(Vector4* values, const unsigned int count);
    bool isVisible() const;
    void setVisible(bool visible);
    unsigned int getSortLevel()const;
    PCA::ClusterAccel* getCluster();
    void updateColor();
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


#endif // 
