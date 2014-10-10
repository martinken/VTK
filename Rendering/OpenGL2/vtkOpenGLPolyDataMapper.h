/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLPolyDataMapper - PolyDataMapper using OpenGL to render.
// .SECTION Description
// PolyDataMapper that uses a OpenGL to do the actual rendering.

#ifndef __vtkOpenGLPolyDataMapper_h
#define __vtkOpenGLPolyDataMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkPolyDataMapper.h"
#include "vtkglVBOHelper.h" // used for ivars

class vtkOpenGLTexture;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkOpenGLPolyDataMapper* New();
  vtkTypeMacro(vtkOpenGLPolyDataMapper, vtkPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPieceStart(vtkRenderer *ren, vtkActor *act);
  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);
  virtual void RenderPieceFinish(vtkRenderer *ren, vtkActor *act);
  virtual void RenderEdges(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  vtkGetMacro(PopulateSelectionSettings,int);
  void SetPopulateSelectionSettings(int v) { this->PopulateSelectionSettings = v; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Used by vtkHardwareSelector to determine if the prop supports hardware
  // selection.
  virtual bool GetSupportsSelection() { return true; }

  // Description:
  // Returns if the mapper does not expect to have translucent geometry. This
  // may happen when using ScalarMode is set to not map scalars i.e. render the
  // scalar array directly as colors and the scalar array has opacity i.e. alpha
  // component. Note that even if this method returns true, an actor may treat
  // the geometry as translucent since a constant translucency is set on the
  // property, for example.
  // Overridden to use the actual data and ScalarMode to determine if we have
  // opaque geometry.
  virtual bool GetIsOpaque();

  // used by RenderPiece and functions it calls to reduce
  // calls to get the input and allow for rendering of
  // other polydata (not the input)
  vtkPolyData *CurrentInput;

protected:
  vtkOpenGLPolyDataMapper();
  ~vtkOpenGLPolyDataMapper();

  // Description:
  // Called in GetBounds(). When this method is called, the consider the input
  // to be updated depending on whether this->Static is set or not. This method
  // simply obtains the bounds from the data-object and returns it.
  virtual void ComputeBounds();

  // Description:
  // Make sure an appropriate shader is defined, compiled and bound.  This method
  // orchistrates the process, much of the work is done in other methods
  virtual void UpdateShader(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Does the shader source need to be recomputed
  virtual bool GetNeedToRebuildShader(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Build the shader source code, called by UpdateShader
  virtual void BuildShader(std::string &VertexCode,
                           std::string &fragmentCode,
                           std::string &geometryCode,
                           int lightComplexity,
                           vtkRenderer *ren, vtkActor *act);

  // Description:
  // Create the basic shaders before replacement
  virtual void GetShaderTemplate(std::string &VertexCode,
                           std::string &fragmentCode,
                           std::string &geometryCode,
                           int lightComplexity,
                           vtkRenderer *ren, vtkActor *act);

  // Description:
  // Perform string replacments on the shader templates
  virtual void ReplaceShaderValues(std::string &VertexCode,
                           std::string &fragmentCode,
                           std::string &geometryCode,
                           int lightComplexity,
                           vtkRenderer *ren, vtkActor *act);

  // Description:
  // Perform string replacments on the shader templates, called from
  // ReplaceShaderValues
  virtual void ReplaceShaderColorMaterialValues(std::string &VertexCode,
                           std::string &fragmentCode,
                           std::string &geometryCode,
                           int lightComplexity,
                           vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the mapper/input data, called by UpdateShader
  virtual void SetMapperShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to lighting, called by UpdateShader
  virtual void SetLightingShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the Camera, called by UpdateShader
  virtual void SetCameraShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the property, called by UpdateShader
  virtual void SetPropertyShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Update the VBO to contain point based values
  virtual void UpdateVBO(vtkActor *act);

  // The VBO and its layout.
  vtkgl::BufferObject VBO;
  vtkgl::VBOLayout Layout;

  // Structures for the various cell types we render.
  vtkgl::CellBO Points;
  vtkgl::CellBO Lines;
  vtkgl::CellBO Tris;
  vtkgl::CellBO TriStrips;
  vtkgl::CellBO *LastBoundBO;

  // values we use to determine if we need to rebuild
  int LastLightComplexity;
  vtkTimeStamp LightComplexityChanged;

  bool LastSelectionState;
  vtkTimeStamp SelectionStateChanged;

  int LastDepthPeeling;
  vtkTimeStamp DepthPeelingChanged;

  bool UsingScalarColoring;
  vtkTimeStamp OpenGLUpdateTime; // When was the OpenGL updated?
  vtkOpenGLTexture* InternalColorTexture;

  int PopulateSelectionSettings;
  int pickingAttributeIDOffset;

private:
  vtkOpenGLPolyDataMapper(const vtkOpenGLPolyDataMapper&); // Not implemented.
  void operator=(const vtkOpenGLPolyDataMapper&); // Not implemented.
};

#endif
