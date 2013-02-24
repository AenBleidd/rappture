/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_DATASET_H__
#define __RAPPTURE_VTKVIS_DATASET_H__

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkDataSetReader.h>
#include <vtkProp.h>
#include <vtkActor.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>

#include <string>
#include <vector>

#include "RpTypes.h"
#include "Trace.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief VTK DataSet wrapper
 */
class DataSet {
public:
    enum DataAttributeType {
        POINT_DATA,
        CELL_DATA,
        FIELD_DATA
    };
    DataSet(const std::string& name);
    virtual ~DataSet();

    void writeDataFile(const char *filename);

    bool setDataFile(const char *filename);

    bool setData(char *data, int nbytes);

    bool setData(vtkDataSetReader *reader);

    bool setData(vtkDataSet *ds);

    vtkDataSet *copyData(vtkDataSet *ds);

    bool isXY() const;

    int numDimensions() const;

    bool is2D(PrincipalPlane *plane = NULL, double *offset = NULL) const;

    PrincipalPlane principalPlane() const;

    const std::string& getName() const;

    vtkDataSet *getVtkDataSet();

    const char *getVtkType() const;

    bool setActiveScalars(const char *name);

    const char *getActiveScalarsName() const;

    DataAttributeType getActiveScalarsType() const;

    bool setActiveVectors(const char *name);

    const char *getActiveVectorsName() const;

    DataAttributeType getActiveVectorsType() const;

    bool hasField(const char *fieldName) const
    {
        return getFieldInfo(fieldName, NULL, NULL);
    }

    bool hasField(const char *fieldName, DataAttributeType type) const
    {
        return getFieldInfo(fieldName, type, NULL);
    }

    bool getFieldInfo(const char *fieldName, DataAttributeType *type, int *numComponents) const;

    bool getFieldInfo(const char *fieldName, DataAttributeType type, int *numComponents) const;

    void getFieldNames(std::vector<std::string>& names,
                       DataAttributeType type, int numComponents = -1) const;

    bool getDataRange(double minmax[2], const char *fieldName,
                      DataAttributeType type, int component = -1) const;

    void getScalarRange(double minmax[2]) const;

    void getVectorRange(double minmax[2], int component = -1) const;

    void getBounds(double bounds[6]) const;

    void getCellSizeRange(double minmax[2], double *average);

    bool getScalarValue(double x, double y, double z, double *value) const;

    bool getVectorValue(double x, double y, double z, double vector[3]) const;

    void setOpacity(double opacity);

    /**
     * \brief Get the opacity setting for the DataSet
     *
     * This method is used for record-keeping.  The renderer controls
     * the visibility of related graphics objects.
     */
    inline double getOpacity()
    {
        return _opacity;
    }

    void setVisibility(bool state);

    bool getVisibility() const;

    void showOutline(bool state);

    void setOutlineColor(float color[3]);

    void setClippingPlanes(vtkPlaneCollection *planes)
    {
        if (_outlineMapper != NULL) {
            _outlineMapper->SetClippingPlanes(planes);
        }
    }

    /**
     * \brief Return the VTK prop object for the outline
     */
    inline vtkProp *getProp()
    {
        return _prop;
    }

    /**
     * \brief Set 2D aspect ratio scaling
     *
     * \param aspect 0=no scaling, otherwise aspect
     * is horiz/vertical ratio
     */
    void setAspect(double aspect)
    {
        double scale[3];
        scale[0] = scale[1] = scale[2] = 1.;

        if (aspect == 0.0) {
            setScale(scale);
            return;
        }
        if (_dataSet == NULL) {
            TRACE("Not setting aspect for empty data set");
            return;
        }

        PrincipalPlane plane;
        if (!is2D(&plane)) {
            TRACE("Not setting aspect for 3D data set");
            return;
        }

        double bounds[6];
        getBounds(bounds);

        double size[3];
        size[0] = bounds[1] - bounds[0];
        size[1] = bounds[3] - bounds[2];
        size[2] = bounds[5] - bounds[4];

        if (aspect == 1.0) {
            // Square
            switch (plane) {
            case PLANE_XY: {
                if (size[0] > size[1] && size[1] > 1.0e-6) {
                    scale[1] = size[0] / size[1];
                } else if (size[1] > size[0] && size[0] > 1.0e-6) {
                    scale[0] = size[1] / size[0];
                }
            }
                break;
            case PLANE_ZY: {
                if (size[1] > size[2] && size[2] > 1.0e-6) {
                    scale[2] = size[1] / size[2];
                } else if (size[2] > size[1] && size[1] > 1.0e-6) {
                    scale[1] = size[2] / size[1];
                }
            }
                break;
            case PLANE_XZ: {
                if (size[0] > size[2] && size[2] > 1.0e-6) {
                    scale[2] = size[0] / size[2];
                } else if (size[2] > size[0] && size[0] > 1.0e-6) {
                    scale[0] = size[2] / size[0];
                }
            }
                break;
            default:
                break;
            }
        } else {
            switch (plane) {
            case PLANE_XY: {
                if (aspect > 1.0) {
                    if (size[0] > size[1]) {
                        scale[1] = (size[0] / aspect) / size[1];
                    } else {
                        scale[0] = (size[1] * aspect) / size[0];
                    }
                } else {
                    if (size[1] > size[0]) {
                        scale[0] = (size[1] * aspect) / size[0];
                    } else {
                        scale[1] = (size[0] / aspect) / size[1];
                    }
                }
            }
                break;
            case PLANE_ZY: {
                if (aspect > 1.0) {
                    if (size[2] > size[1]) {
                        scale[1] = (size[2] / aspect) / size[1];
                    } else {
                        scale[2] = (size[1] * aspect) / size[2];
                    }
                } else {
                    if (size[1] > size[2]) {
                        scale[2] = (size[1] * aspect) / size[2];
                    } else {
                        scale[1] = (size[2] / aspect) / size[1];
                    }
                }
            }
                break;
            case PLANE_XZ: {
                if (aspect > 1.0) {
                    if (size[0] > size[2]) {
                        scale[2] = (size[0] / aspect) / size[2];
                    } else {
                        scale[0] = (size[2] * aspect) / size[0];
                    }
                } else {
                    if (size[2] > size[0]) {
                        scale[0] = (size[2] * aspect) / size[0];
                    } else {
                        scale[2] = (size[0] / aspect) / size[2];
                    }
                }
            }
            default:
                break;
            }
        }

        TRACE("obj %g,%g,%g", size[0], size[1], size[2]);
        TRACE("Setting scale to %g,%g,%g", scale[0], scale[1], scale[2]);
        setScale(scale);
    }

    /**
     * \brief Get the prop scaling
     *
     * \param[out] scale Scaling in x,y,z
     */
    void getScale(double scale[3])
    {
        if (_prop != NULL) {
            _prop->GetScale(scale);
        } else {
            scale[0] = scale[1] = scale[2] = 1.0;
        }
    }

    /**
     * \brief Set the prop scaling
     *
     * \param[in] scale Scaling in x,y,z
     */
    void setScale(double scale[3])
    {
        if (_prop != NULL) {
            _prop->SetScale(scale);
        }
    }

    static void print(vtkDataSet *ds);

private:
    DataSet();

    void setDefaultArrays();
    void print() const;

    void initProp();

    std::string _name;
    vtkSmartPointer<vtkDataSet> _dataSet;
    bool _visible;
    double _opacity;
    double _cellSizeRange[2];
    double _cellSizeAverage;
    vtkSmartPointer<vtkOutlineFilter> _outlineFilter;
    vtkSmartPointer<vtkActor> _prop;
    vtkSmartPointer<vtkPolyDataMapper> _outlineMapper;
};

}
}

#endif
