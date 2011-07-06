/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceImageViewer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResliceImageViewer.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkResliceCursorWidget.h"
#include "vtkResliceCursorLineRepresentation.h"
#include "vtkResliceCursorThickLineRepresentation.h"
#include "vtkResliceCursorActor.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkPlane.h"
#include "vtkResliceCursor.h"
#include "vtkImageData.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkInteractorStyleImage.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkImageReslice.h"
#include "vtkLookupTable.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkPlane.h"
#include "vtkResliceImageViewerMeasurements.h"

vtkStandardNewMacro(vtkResliceImageViewer);

//----------------------------------------------------------------------------
vtkResliceImageViewer::vtkResliceImageViewer()
{
  // Default is to not use the reslice cursor widget, ie use fast
  // 3D texture mapping to display slices.
  this->ResliceMode = vtkResliceImageViewer::RESLICE_AXIS_ALIGNED;

  // Set up the reslice cursor widget, should it be used.

  this->ResliceCursorWidget = vtkResliceCursorWidget::New();

  vtkSmartPointer< vtkResliceCursor > resliceCursor =
    vtkSmartPointer< vtkResliceCursor >::New();
  resliceCursor->SetThickMode(0);
  resliceCursor->SetThickness(10, 10, 10);

  vtkSmartPointer< vtkResliceCursorLineRepresentation >
    resliceCursorRep = vtkSmartPointer<
      vtkResliceCursorLineRepresentation >::New();
  resliceCursorRep->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetResliceCursor(resliceCursor);
  resliceCursorRep->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetReslicePlaneNormal(this->SliceOrientation);
  this->ResliceCursorWidget->SetRepresentation(resliceCursorRep);

  this->PointPlacer = vtkBoundedPlanePointPlacer::New();

  this->Measurements = vtkResliceImageViewerMeasurements::New();
  this->Measurements->SetResliceImageViewer(this);

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
vtkResliceImageViewer::~vtkResliceImageViewer()
{
  this->Measurements->Delete();

  if (this->ResliceCursorWidget)
    {
    this->ResliceCursorWidget->Delete();
    this->ResliceCursorWidget = NULL;
    }

  this->PointPlacer->Delete();
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetThickMode( int t )
{
  vtkSmartPointer< vtkResliceCursor > rc = this->GetResliceCursor();

  if (t == this->GetThickMode())
    {
    return;
    }

  vtkSmartPointer< vtkResliceCursorLineRepresentation >
    resliceCursorRepOld = vtkResliceCursorLineRepresentation::SafeDownCast(
                          this->ResliceCursorWidget->GetRepresentation());
  vtkSmartPointer< vtkResliceCursorLineRepresentation > resliceCursorRepNew;

  this->GetResliceCursor()->SetThickMode(t);

  if (t)
    {
    resliceCursorRepNew = vtkSmartPointer<
        vtkResliceCursorThickLineRepresentation >::New();
    }
  else
    {
    resliceCursorRepNew = vtkSmartPointer<
        vtkResliceCursorLineRepresentation >::New();
    }

  int e = this->ResliceCursorWidget->GetEnabled();
  this->ResliceCursorWidget->SetEnabled(0);

  resliceCursorRepNew->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetResliceCursor(rc);
  resliceCursorRepNew->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetReslicePlaneNormal(this->SliceOrientation);
  this->ResliceCursorWidget->SetRepresentation(resliceCursorRepNew);
  resliceCursorRepNew->SetLookupTable(resliceCursorRepOld->GetLookupTable());

  resliceCursorRepNew->SetWindowLevel(
      resliceCursorRepOld->GetWindow(),
      resliceCursorRepOld->GetLevel(), 1);

  this->ResliceCursorWidget->SetEnabled(e);
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetResliceCursor( vtkResliceCursor * rc )
{
  vtkResliceCursorRepresentation *rep =
    vtkResliceCursorRepresentation::SafeDownCast(
          this->GetResliceCursorWidget()->GetRepresentation());
  rep->GetCursorAlgorithm()->SetResliceCursor(rc);

  // Rehook the observer to this reslice cursor.
  this->Measurements->SetResliceImageViewer(this);
}

//----------------------------------------------------------------------------
int vtkResliceImageViewer::GetThickMode()
{
  return (vtkResliceCursorThickLineRepresentation::
    SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetLookupTable( vtkLookupTable * l )
{
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    rep->SetLookupTable(l);
    }

  if (this->WindowLevel)
    {
    this->WindowLevel->SetLookupTable(l);
    this->WindowLevel->SetOutputFormatToRGBA();
    this->WindowLevel->PassAlphaToOutputOn();
    }
}

//----------------------------------------------------------------------------
vtkLookupTable * vtkResliceImageViewer::GetLookupTable()
{
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    return rep->GetLookupTable();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::UpdateOrientation()
{
    // Set the camera position

    vtkCamera *cam = this->Renderer ? this->Renderer->GetActiveCamera() : NULL;
    if (cam)
      {
      switch (this->SliceOrientation)
        {
        case vtkImageViewer2::SLICE_ORIENTATION_XY:
          cam->SetFocalPoint(0,0,0);
          cam->SetPosition(0,0,1); // -1 if medical ?
          cam->SetViewUp(0,1,0);
          break;

        case vtkImageViewer2::SLICE_ORIENTATION_XZ:
          cam->SetFocalPoint(0,0,0);
          cam->SetPosition(0,-1,0); // 1 if medical ?
          cam->SetViewUp(0,0,1);
          break;

        case vtkImageViewer2::SLICE_ORIENTATION_YZ:
          cam->SetFocalPoint(0,0,0);
          cam->SetPosition(1,0,0); // -1 if medical ?
          cam->SetViewUp(0,0,1);
          break;
        }
      }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::UpdateDisplayExtent()
{
  // Only update the display extent in axis aligned mode

  if (this->ResliceMode == RESLICE_AXIS_ALIGNED)
    {
    this->Superclass::UpdateDisplayExtent();
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::InstallPipeline()
{
  this->Superclass::InstallPipeline();

  if (this->Interactor)
    {
    this->ResliceCursorWidget->SetInteractor(this->Interactor);
    }

  if (this->Renderer)
    {
    this->ResliceCursorWidget->SetDefaultRenderer(this->Renderer);
    vtkCamera *cam = this->Renderer->GetActiveCamera();
    cam->ParallelProjectionOn();
    }

  if (this->ResliceMode == RESLICE_OBLIQUE)
    {
    this->ResliceCursorWidget->SetEnabled(1);
    this->ImageActor->SetVisibility(0);
    this->UpdateOrientation();

    double bounds[6];

    vtkCamera *cam = this->Renderer->GetActiveCamera();
    this->GetResliceCursor()->GetImage()->GetBounds(bounds);
    double *spacing = this->GetResliceCursor()->GetImage()->GetSpacing();
    double avg_spacing =
      (spacing[0] + spacing[1] + spacing[2]) / 3.0;
    cam->SetClippingRange(
      bounds[this->SliceOrientation * 2] - 100 * avg_spacing,
      bounds[this->SliceOrientation * 2 + 1] + 100 * avg_spacing);
    }
  else
    {
    this->ResliceCursorWidget->SetEnabled(0);
    this->ImageActor->SetVisibility(1);
    this->UpdateOrientation();
    }

  if (this->WindowLevel)
    {
    this->WindowLevel->SetLookupTable(this->GetLookupTable());
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::UnInstallPipeline()
{
  this->ResliceCursorWidget->SetEnabled(0);

  this->Superclass::UnInstallPipeline();
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::UpdatePointPlacer()
{
  if (this->ResliceMode == RESLICE_OBLIQUE)
    {
    this->PointPlacer->SetProjectionNormalToOblique();
    if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
      {
      const int planeOrientation =
        rep->GetCursorAlgorithm()->GetReslicePlaneNormal();
      vtkPlane *plane = this->GetResliceCursor()->GetPlane(planeOrientation);
      this->PointPlacer->SetObliquePlane(plane);
      }
    }
  else
    {

    vtkImageData *input = this->ImageActor->GetInput();
    if ( !input )
      {
      return;
      }

    double spacing[3];
    input->GetSpacing(spacing);

    double origin[3];
    input->GetOrigin(origin);

    double bounds[6];
    this->ImageActor->GetBounds(bounds);

    int displayExtent[6];
    this->ImageActor->GetDisplayExtent(displayExtent);

    int axis;
    double position;
    if ( displayExtent[0] == displayExtent[1] )
      {
      axis = vtkBoundedPlanePointPlacer::XAxis;
      position = origin[0] + displayExtent[0]*spacing[0];
      }
    else if ( displayExtent[2] == displayExtent[3] )
      {
      axis = vtkBoundedPlanePointPlacer::YAxis;
      position = origin[1] + displayExtent[2]*spacing[1];
      }
    else if ( displayExtent[4] == displayExtent[5] )
      {
      axis = vtkBoundedPlanePointPlacer::ZAxis;
      position = origin[2] + displayExtent[4]*spacing[2];
      }

    this->PointPlacer->SetProjectionNormal(axis);
    this->PointPlacer->SetProjectionPosition(position);
    }

}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::Render()
{
  this->UpdatePointPlacer();

  this->Superclass::Render();
}

//----------------------------------------------------------------------------
vtkResliceCursor * vtkResliceImageViewer::GetResliceCursor()
{
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    return rep->GetResliceCursor();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetInput(vtkImageData *in)
{
  if(!in)
    {
    return;
    }

  this->WindowLevel->SetInput(in);
  this->GetResliceCursor()->SetImage(in);
  this->GetResliceCursor()->SetCenter(in->GetCenter());
  this->UpdateDisplayExtent();

  double range[2];
  in->GetScalarRange(range);
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    if (vtkImageReslice *reslice =
        vtkImageReslice::SafeDownCast(rep->GetReslice()))
      {
      // default background color is the min value of the image scalar range
      reslice->SetBackgroundColor(range[0],range[0],range[0],range[0]);
      this->SetColorWindow(range[1]-range[0]);
      this->SetColorLevel((range[0]+range[1])/2.0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetInputConnection(vtkAlgorithmOutput* input)
{
  vtkErrorMacro( << "Use SetInput instead. " );
  this->WindowLevel->SetInputConnection(input);
  this->UpdateDisplayExtent();
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetResliceMode( int r )
{
  if (r == this->ResliceMode)
    {
    return;
    }

  this->ResliceMode = r;
  this->Modified();

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetColorWindow( double w )
{
  double rmin = this->GetColorLevel() - 0.5*fabs( w );
  double rmax = rmin + fabs( w );
  this->GetLookupTable()->SetTableRange( rmin, rmax );

  this->WindowLevel->SetWindow(w);
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    rep->SetWindowLevel(w, rep->GetLevel(), 1);
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetColorLevel( double w )
{
  double rmin = w - 0.5*fabs( this->GetColorWindow() );
  double rmax = rmin + fabs( this->GetColorWindow() );
  this->GetLookupTable()->SetTableRange( rmin, rmax );

  this->WindowLevel->SetLevel(w);
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    rep->SetWindowLevel(rep->GetWindow(), w, 1);
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::Reset()
{
  this->ResliceCursorWidget->ResetResliceCursor();
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResliceCursorWidget:\n";
  this->ResliceCursorWidget->PrintSelf(os,indent.GetNextIndent());
  os << indent << "ResliceMode: " << this->ResliceMode << endl;
  os << indent << "Point Placer: ";
  this->PointPlacer->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Measurements: ";
  this->Measurements->PrintSelf(os,indent.GetNextIndent());
}