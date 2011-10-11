/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_VTKGRAPHICSOBJECT_H__
#define __RAPPTURE_VTKVIS_VTKGRAPHICSOBJECT_H__

#include <cmath>
#include <string>

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkProp3D.h>
#include <vtkProp3DCollection.h>
#include <vtkAssembly.h>
#include <vtkActor.h>
#include <vtkVolume.h>
#include <vtkProperty.h>
#include <vtkVolumeProperty.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkPlaneCollection.h>

#include "RpVtkDataSet.h"
#include "Trace.h"

namespace Rappture {
namespace VtkVis {

class Renderer;

/**
 * \brief Base class for graphics objects
 */
class VtkGraphicsObject {
public:
    enum CullFace {
        CULL_FRONT,
        CULL_BACK,
        CULL_FRONT_AND_BACK
    };

    VtkGraphicsObject() :
        _dataSet(NULL),
        _opacity(1.0),
        _edgeWidth(1.0f),
        _pointSize(1.0f),
        _lighting(true),
        _cullFace(CULL_BACK),
        _faceCulling(false)
    {
        _dataRange[0] = 0;
        _dataRange[1] = 1;
        _color[0] = 1.0f;
        _color[1] = 1.0f;
        _color[2] = 1.0f;
        _edgeColor[0] = 0.0f;
        _edgeColor[1] = 0.0f;
        _edgeColor[2] = 0.0f;
    }

    virtual ~VtkGraphicsObject()
    {}

    /**
     * \brief Return the name of the sublass of VtkGraphicsObject
     */
    virtual const char *getClassName() const = 0;

    /**
     * \brief Specify input DataSet
     *
     * Default implementation calls update()
     *
     * \param[in] dataSet DataSet to use in rendering
     */
    virtual void setDataSet(DataSet *dataSet)
    {
        if (_dataSet != dataSet) {
            _dataSet = dataSet;

            update();
        }
    }

    /**
     * \brief Specify input DataSet and information on cumulative data ranges
     *
     * Default implementation calls update() and stores scalarRange to
     * _dataRange based on cumulative range settings
     *
     * \param[in] dataSet DataSet to use in rendering
     * \param[in] renderer Pointer to Renderer -- may be used to get 
     * cumulative data ranges
     */
    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    /**
     * \brief Called when scalar or vector field changes (e.g. active field) or 
     * cumulative ranges or settings change
     *
     * \param[in] renderer Pointer to Renderer -- may be used to get 
     * cumulative data ranges
     */
    virtual void updateRanges(Renderer *renderer);

    /**
     * \brief Return the DataSet this object renders
     */
    inline DataSet *getDataSet()
    {
        return _dataSet;
    }

    /**
     * \brief Return the VTK prop object to render
     */
    inline vtkProp *getProp()
    {
        return _prop;
    }

    /**
     * \brief Cast the vktProp to a vtkProp3D
     *
     * \return NULL or a vtkProp3D pointer
     */
    inline vtkProp3D *getProp3D()
    {
        return vtkProp3D::SafeDownCast(_prop);
    }

    /**
     * \brief Cast the vktProp to a vtkActor
     *
     * \return NULL or a vtkActor pointer
     */
    inline vtkActor *getActor()
    {
        return vtkActor::SafeDownCast(_prop);
    }

    /**
     * \brief Cast the vktProp to a vtkVolume
     *
     * \return NULL or a vtkVolume pointer
     */
    inline vtkVolume *getVolume()
    {
        return vtkVolume::SafeDownCast(_prop);
    }

    /**
     * \brief Cast the vktProp to a vtkAssembly
     *
     * \return NULL or a vtkAssembly pointer
     */
    inline vtkAssembly *getAssembly()
    {
        return vtkAssembly::SafeDownCast(_prop);
    }

    /**
     * \brief Set an additional transform on the prop
     * 
     * prop must be a vtkProp3D.  The transform is 
     * concatenated with the internal transform created
     * by setting the prop position, orientation and scale
     */
    virtual void setTransform(vtkMatrix4x4 *matrix)
    {
        if (getProp3D() != NULL) {
            getProp3D()->SetUserMatrix(matrix);
        }
    }

    /**
     * \brief Set the prop center of rotation
     *
     * \param[in] origin The point about which the object rotates
     */
    virtual void setOrigin(double origin[3])
    {
        if (getProp3D() != NULL) {
            getProp3D()->SetOrigin(origin);
        }
    }

    /**
     * \brief Set the prop orientation with a quaternion
     *
     * \param[in] quat Quaternion with scalar part first
     */
    virtual void setOrientation(double quat[4])
    {
        if (getProp3D() != NULL) {
            double angle = vtkMath::DegreesFromRadians(2.0 * acos(quat[0]));
            double axis[3];
            if (angle < 1.0e-6) {
                axis[0] = 1;
                axis[1] = 0;
                axis[2] = 0;
            } else {
                double denom = sqrt(1. - quat[0] * quat[0]);
                axis[0] = quat[1] / denom;
                axis[1] = quat[2] / denom;
                axis[2] = quat[3] / denom;
             }
            setOrientation(angle, axis);
        }
    }

    /**
     * \brief Set the prop orientation with a rotation about an axis
     *
     * \param[in] angle Angle in degrees
     * \param[in] axis Axis of rotation
     */
    virtual void setOrientation(double angle, double axis[3])
    {
        if (getProp3D() != NULL) {
            getProp3D()->SetOrientation(0, 0, 0);
            getProp3D()->RotateWXYZ(angle, axis[0], axis[1], axis[2]);
        }
    }

    /**
     * \brief Set the prop position
     *
     * \param[in] pos Position in world coordinates
     */
    virtual void setPosition(double pos[3])
    {
        if (getProp3D() != NULL) {
            getProp3D()->SetPosition(pos);
        }
    }

    /**
     * \brief Set the prop scaling
     *
     * \param[in] scale Scaling in x,y,z
     */
    virtual void setScale(double scale[3])
    {
        if (getProp3D() != NULL) {
            getProp3D()->SetScale(scale);
        }
    }

    /**
     * \brief Toggle visibility of the prop
     */
    virtual void setVisibility(bool state)
    {
        if (_prop != NULL)
            _prop->SetVisibility((state ? 1 : 0));
    }

    /**
     * \brief Get visibility state of the prop
     */
    virtual bool getVisibility()
    {
        if (_prop != NULL &&
            _prop->GetVisibility() != 0) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * \brief Set the opacity of the object
     *
     * The prop must be a vtkActor or vtkAssembly
     */
    virtual void setOpacity(double opacity)
    {
        _opacity = opacity;
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetOpacity(opacity);
            if (_faceCulling && _opacity < 1.0) {
                setCulling(getActor()->GetProperty(), false);
                TRACE("Culling off");
            } else if (_faceCulling && _opacity == 1.0) {
                setCulling(getActor()->GetProperty(), true);
                TRACE("Culling on");
            }
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetOpacity(opacity);
                    if (_faceCulling && _opacity < 1.0) {
                        setCulling(vtkActor::SafeDownCast(prop)->GetProperty(), false);
                        TRACE("Culling off");
                    } else if (_faceCulling && _opacity == 1.0) {
                        setCulling(vtkActor::SafeDownCast(prop)->GetProperty(), true);
                        TRACE("Culling on");
                    }
                }
            }
        }
    }

    /**
     * \brief Get the opacity of the object
     */
    inline double getOpacity()
    {
        return _opacity;
    }

    /**
     * \brief Set the ambient material coefficient
     */
    virtual void setAmbient(double ambient)
    {
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetAmbient(ambient);
        } else if (getVolume() != NULL) {
            getVolume()->GetProperty()->SetAmbient(ambient);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetAmbient(ambient);
                } else if (vtkVolume::SafeDownCast(prop) != NULL) {
                    vtkVolume::SafeDownCast(prop)->GetProperty()->SetAmbient(ambient);
                }
            }
        }
    }

    /**
     * \brief Set the diffuse material coefficient
     */
    virtual void setDiffuse(double diffuse)
    {
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetDiffuse(diffuse);
        } else if (getVolume() != NULL) {
            getVolume()->GetProperty()->SetDiffuse(diffuse);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetDiffuse(diffuse);
                } else if (vtkVolume::SafeDownCast(prop) != NULL) {
                    vtkVolume::SafeDownCast(prop)->GetProperty()->SetDiffuse(diffuse);
                }
            }
        }
    }

    /**
     * \brief Set the specular material coefficient and power
     */
    virtual void setSpecular(double coeff, double power)
    {
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetSpecular(coeff);
            getActor()->GetProperty()->SetSpecularPower(power);
        } else if (getVolume() != NULL) {
            getVolume()->GetProperty()->SetSpecular(coeff);
            getVolume()->GetProperty()->SetSpecularPower(power);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetSpecular(coeff);
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetSpecularPower(power);
                } else if (vtkVolume::SafeDownCast(prop) != NULL) {
                    vtkVolume::SafeDownCast(prop)->GetProperty()->SetSpecular(coeff);
                    vtkVolume::SafeDownCast(prop)->GetProperty()->SetSpecularPower(power);
                }
            }
        }
    }

    /**
     * \brief Set material properties (coefficients and specular power) of object
     */
    virtual void setMaterial(double ambient, double diffuse, double specular, double specPower)
    {
        if (getActor() != NULL) {
            vtkProperty *property = getActor()->GetProperty();
            property->SetAmbient(ambient);
            property->SetDiffuse(diffuse);
            property->SetSpecular(specular);
            property->SetSpecularPower(specPower);
        } else if (getVolume() != NULL) {
            vtkVolumeProperty *property = getVolume()->GetProperty();
            property->SetAmbient(ambient);
            property->SetDiffuse(diffuse);
            property->SetSpecular(specular);
            property->SetSpecularPower(specPower);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkProperty *property = vtkActor::SafeDownCast(prop)->GetProperty();
                    property->SetAmbient(ambient);
                    property->SetDiffuse(diffuse);
                    property->SetSpecular(specular);
                    property->SetSpecularPower(specPower);
                } else if (vtkVolume::SafeDownCast(prop) != NULL) {
                    vtkVolumeProperty *property = vtkVolume::SafeDownCast(prop)->GetProperty();
                    property->SetAmbient(ambient);
                    property->SetDiffuse(diffuse);
                    property->SetSpecular(specular);
                    property->SetSpecularPower(specPower);
                }
            }
        }
    }

    /**
     * \brief Set the material color (sets ambient, diffuse, and specular)
     */
    virtual void setColor(float color[3])
    {
        for (int i = 0; i < 3; i++)
            _color[i] = color[i];
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetColor(color[0], color[1], color[2]);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetColor(color[0], color[1], color[2]);
                }
            }
        }
    }

    /**
     * \brief Set the material ambient color
     */
    virtual void setAmbientColor(float color[3])
    {
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetAmbientColor(color[0], color[1], color[2]);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetAmbientColor(color[0], color[1], color[2]);
                }
            }
        }
    }

    /**
     * \brief Set the material diffuse color
     */
    virtual void setDiffuseColor(float color[3])
    {
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetDiffuseColor(color[0], color[1], color[2]);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetDiffuseColor(color[0], color[1], color[2]);
                }
            }
        }
    }

    /**
     * \brief Set the material specular color
     */
    virtual void setSpecularColor(float color[3])
    {
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetSpecularColor(color[0], color[1], color[2]);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetSpecularColor(color[0], color[1], color[2]);
                }
            }
        }
    }

    /**
     * \brief Toggle lighting of the prop
     */
    virtual void setLighting(bool state)
    {
        _lighting = state;
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetLighting((state ? 1 : 0));
        } else if (getVolume() != NULL) {
            getVolume()->GetProperty()->SetShade((state ? 1 : 0));
         } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetLighting((state ? 1 : 0));
                } else if (vtkVolume::SafeDownCast(prop) != NULL) {
                    vtkVolume::SafeDownCast(prop)->GetProperty()->SetShade((state ? 1 : 0));
                }
            }
        }
    }

    /**
     * \brief Toggle drawing of edges
     */
    virtual void setEdgeVisibility(bool state)
    {
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetEdgeVisibility((state ? 1 : 0));
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetEdgeVisibility((state ? 1 : 0));
                }
            }
        }
    }

    /**
     * \brief Set color of edges
     */
    virtual void setEdgeColor(float color[3])
    {
        for (int i = 0; i < 3; i++)
            _edgeColor[i] = color[i];
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetEdgeColor(color[0], color[1], color[2]);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetEdgeColor(color[0], color[1], color[2]);
                }
            }
        }
    }

    /**
     * \brief Set pixel width of edges
     *
     * NOTE: May be a no-op if OpenGL implementation doesn't support wide lines
     */
    virtual void setEdgeWidth(float width)
    {
        _edgeWidth = width;
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetLineWidth(width);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetLineWidth(width);
                }
            }
        }
    }

    /**
     * \brief Set point size
     *
     * NOTE: May be a no-op if OpenGL implementation doesn't support wide points
     */
    virtual void setPointSize(float size)
    {
        _pointSize = size;
        if (getActor() != NULL) {
            getActor()->GetProperty()->SetPointSize(size);
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    vtkActor::SafeDownCast(prop)->GetProperty()->SetPointSize(size);
                }
            }
        }
    }

    /**
     * \brief Toggle wireframe rendering of prop
     */
    virtual void setWireframe(bool state)
    {
        if (getActor() != NULL) {
            if (state) {
                getActor()->GetProperty()->SetRepresentationToWireframe();
                getActor()->GetProperty()->LightingOff();
            } else {
                getActor()->GetProperty()->SetRepresentationToSurface();
                setLighting(_lighting);
            }
        } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    if (state) {
                        vtkActor::SafeDownCast(prop)->GetProperty()->SetRepresentationToWireframe();
                        vtkActor::SafeDownCast(prop)->GetProperty()->LightingOff();
                    } else {
                        vtkActor::SafeDownCast(prop)->GetProperty()->SetRepresentationToSurface();
                        vtkActor::SafeDownCast(prop)->GetProperty()->SetLighting((_lighting ? 1 : 0));
                    }
                }
            }
        }
    }

    /**
     * \brief Toggle culling of selected CullFace
     */
    virtual void setCulling(bool state)
    {
        _faceCulling = state;
        if (state && _opacity < 1.0)
            return;
        if (getActor() != NULL) {
            setCulling(getActor()->GetProperty(), state);
         } else if (getAssembly() != NULL) {
            vtkProp3DCollection *props = getAssembly()->GetParts();
            vtkProp3D *prop;
            props->InitTraversal();
            while ((prop = props->GetNextProp3D()) != NULL) {
                if (vtkActor::SafeDownCast(prop) != NULL) {
                    setCulling(vtkActor::SafeDownCast(prop)->GetProperty(), state);
                }
            }
        }
    }

    /**
     * \brief Specify which face(s) to cull when culling is enabled
     */
    virtual void setCullFace(CullFace cull)
    {
        _cullFace = cull;
        setCulling(_faceCulling);
    }

    /**
     * \brief Subclasses need to implement setting clipping planes in their mappers
     */
    virtual void setClippingPlanes(vtkPlaneCollection *planes) = 0;

protected:
    /**
     * \brief Create and initialize a VTK Prop to render the object
     */
    virtual void initProp()
    {
        if (_prop == NULL) {
            _prop = vtkSmartPointer<vtkActor>::New();
            vtkProperty *property = getActor()->GetProperty();
            property->SetColor(_color[0], _color[1], _color[2]);
            property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
            property->SetLineWidth(_edgeWidth);
            property->SetPointSize(_pointSize);
            property->EdgeVisibilityOff();
            property->SetOpacity(_opacity);
            property->SetAmbient(.2);
            if (!_lighting)
                property->LightingOff();
            if (_faceCulling && _opacity == 1.0) {
                setCulling(property, true);
            }
        }
    }

    /**
     * \brief Subclasses implement this to create the VTK pipeline
     * on a state change (e.g. new DataSet)
     */
    virtual void update() = 0;

    /**
     * \brief Convenience method to set culling state on a vtkProperty
     *
     * Note: Does not change the culling state flag of this VtkGraphicsObject
     */
    virtual void setCulling(vtkProperty *property, bool state)
    {
        switch (_cullFace) {
        case CULL_FRONT:
            property->SetFrontfaceCulling((state ? 1 : 0));
            property->BackfaceCullingOff();
            break;
        case CULL_BACK:
            property->SetBackfaceCulling((state ? 1 : 0));
            property->FrontfaceCullingOff();
            break;
        case CULL_FRONT_AND_BACK:
            property->SetBackfaceCulling((state ? 1 : 0));
            property->SetFrontfaceCulling((state ? 1 : 0));
            break;
        }
    }

    DataSet *_dataSet;
    double _dataRange[2];
    double _opacity;
    float _color[3];
    float _edgeColor[3];
    float _edgeWidth;
    float _pointSize;
    bool _lighting;
    CullFace _cullFace;
    bool _faceCulling;

    vtkSmartPointer<vtkProp> _prop;
};

}
}

#endif
