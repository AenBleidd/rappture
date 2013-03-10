/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRpAxisFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRpAxisFollower - a subclass of vtkFollower that ensures that
// data is always parallel to the axis defined by a vtkRpAxisActor.
// .SECTION Description
// vtkRpAxisFollower is a subclass of vtkFollower that always follows its
// specified axis. More specifically it will not change its position or scale,
// but it will continually update its orientation so that it is aliged with the
// axis and facing at angle to the camera to provide maximum visibilty.
// This is typically used for text labels for 3d plots.
// .SECTION see also
// vtkActor vtkFollower vtkCamera vtkRpAxisActor vtkCubeAxesActor


#ifndef __vtkRpAxisFollower_h
#define __vtkRpAxisFollower_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkFollower.h"

#include "vtkWeakPointer.h" // For vtkWeakPointer

// Forward declarations.
class vtkRpAxisActor;
class vtkRenderer;

class VTKRENDERINGANNOTATION_EXPORT vtkRpAxisFollower : public vtkFollower
{
public:
 vtkTypeMacro(vtkRpAxisFollower,vtkFollower);
 virtual void PrintSelf(ostream& os, vtkIndent indent);

 // Description:
 // Creates a follower with no camera set
 static vtkRpAxisFollower *New();

 // Description:
 // Set axis that needs to be followed.
 virtual void SetAxis(vtkRpAxisActor*);
 virtual vtkRpAxisActor* GetAxis();

 // Description:
 // Set/Get state of auto center mode where additional
 // translation will be added to make sure the underlying
 // geometry has its pivot point at the center of its bounds.
 vtkSetMacro(AutoCenter, int);
 vtkGetMacro(AutoCenter, int);
 vtkBooleanMacro(AutoCenter, int);

 // Description:
 // Enable / disable use of distance based LOD. If enabled the actor
 // will not be visible at a certain distance from the camera.
 // Default is false.
 vtkSetMacro(EnableDistanceLOD, int);
 vtkGetMacro(EnableDistanceLOD, int);

 // Description:
 // Set distance LOD threshold (0.0 - 1.0).This determines at what fraction
 // of camera far clip range, actor is not visible.
 // Default is 0.80.
 vtkSetClampMacro(DistanceLODThreshold, double, 0.0, 1.0);
 vtkGetMacro(DistanceLODThreshold, double);

 // Description:
 // Enable / disable use of view angle based LOD. If enabled the actor
 // will not be visible at a certain view angle.
 // Default is true.
 vtkSetMacro(EnableViewAngleLOD, int);
 vtkGetMacro(EnableViewAngleLOD, int);

 // Description:
 // Set view angle LOD threshold (0.0 - 1.0).This determines at what view
 // angle to geometry will make the geometry not visibile.
 // Default is 0.34.
 vtkSetClampMacro(ViewAngleLODThreshold, double, 0.0, 1.0);
 vtkGetMacro(ViewAngleLODThreshold, double);

 // Description:
 // Set/Get the desired screen offset from the axis.
 vtkSetMacro(ScreenOffset, double);
 vtkGetMacro(ScreenOffset, double);

 // Description:
 // This causes the actor to be rendered. It in turn will render the actor's
 // property, texture map and then mapper. If a property hasn't been
 // assigned, then the actor will create one automatically.
 virtual int RenderOpaqueGeometry(vtkViewport *viewport);
 virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
 virtual void Render(vtkRenderer *ren);

 // Description:
 // Generate the matrix based on ivars. This method overloads its superclasses
 // ComputeMatrix() method due to the special vtkFollower matrix operations.
 virtual void ComputeTransformMatrix(vtkRenderer *ren);

 // Description:
 // Shallow copy of a follower. Overloads the virtual vtkProp method.
 void ShallowCopy(vtkProp *prop);

 // Description:
 // Calculate scale factor to maintain same size of a object
 // on the screen.
 static double AutoScale(vtkViewport *viewport, vtkCamera * camera,
                         double screenSize, double position[3]);

protected:
 vtkRpAxisFollower();
 ~vtkRpAxisFollower();

 void CalculateOrthogonalVectors(double Rx[3], double Ry[3], double Rz[3],
                                 vtkRpAxisActor *axis1, double *dop,
                                 vtkRenderer *ren);


 void ComputeRotationAndTranlation(vtkRenderer *ren, double translation[3],
                                   double Rx[3], double Ry[3], double Rz[3],
                                   vtkRpAxisActor *axis);

 // \NOTE: Not used as of now.
 void ComputerAutoCenterTranslation(const double& autoScaleFactor,
                                    double translation[3]);


 int  TestDistanceVisibility();
 void ExecuteViewAngleVisibility(double normal[3]);

 bool IsTextUpsideDown(double* a, double* b);

 int          AutoCenter;

 int          EnableDistanceLOD;
 double       DistanceLODThreshold;

 int          EnableViewAngleLOD;
 double       ViewAngleLODThreshold;

 double       ScreenOffset;

 vtkWeakPointer<vtkRpAxisActor> Axis;


private:

 int TextUpsideDown;
 int VisibleAtCurrentViewAngle;

 vtkRpAxisFollower(const vtkRpAxisFollower&);  // Not implemented.
 void operator =(const vtkRpAxisFollower&);  // Not implemented.

 // hide the two parameter Render() method from the user and the compiler.
 virtual void Render(vtkRenderer *, vtkMapper *) {};

 //Internal matrices to avoid New/Delete for performance reasons
 vtkMatrix4x4 *InternalMatrix;

};

#endif // __vtkRpAxisFollower_h
