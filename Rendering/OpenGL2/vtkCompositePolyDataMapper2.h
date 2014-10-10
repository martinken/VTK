/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositePolyDataMapper2 - mapper for composite dataset consisting
// of polygonal data.
// .SECTION Description
// vtkCompositePolyDataMapper2 is similar to vtkCompositePolyDataMapper except
// that instead of creating individual mapper for each block in the composite
// dataset, it iterates over the blocks internally.

#ifndef __vtkCompositePolyDataMapper2_h
#define __vtkCompositePolyDataMapper2_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h" // for vtkSmartPointer
#include "vtkOpenGLPolyDataMapper.h"
 #include "vtkColor.h"

#include <map>
#include <stack>

class vtkCompositeDataDisplayAttributes;
class vtkCompositeMapperHelper;

class VTKRENDERINGOPENGL2_EXPORT vtkCompositePolyDataMapper2 : public vtkOpenGLPolyDataMapper
{
public:
  static vtkCompositePolyDataMapper2* New();
  vtkTypeMacro(vtkCompositePolyDataMapper2, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

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

  // Description:
  // Set/get the composite data set attributes.
  void SetCompositeDataDisplayAttributes(vtkCompositeDataDisplayAttributes *attributes);
  vtkCompositeDataDisplayAttributes* GetCompositeDataDisplayAttributes();

  // Description:
  // Set/get the visibility for a block given its flat index.
  void SetBlockVisibility(unsigned int index, bool visible);
  bool GetBlockVisibility(unsigned int index) const;
  void RemoveBlockVisibility(unsigned int index);
  void RemoveBlockVisibilites();

  // Description:
  // Set/get the color for a block given its flat index.
  void SetBlockColor(unsigned int index, double color[3]);
  void SetBlockColor(unsigned int index, double r, double g, double b)
    {
    double color[3] = {r, g, b};
    this->SetBlockColor(index, color);
    }
  double* GetBlockColor(unsigned int index);
  void RemoveBlockColor(unsigned int index);
  void RemoveBlockColors();

  // Description:
  // Set/get the opacity for a block given its flat index.
  void SetBlockOpacity(unsigned int index, double opacity);
  double GetBlockOpacity(unsigned int index);
  void RemoveBlockOpacity(unsigned int index);
  void RemoveBlockOpacities();

  // Description:
  // This calls RenderPiece (in a for loop if streaming is necessary).
  virtual void Render(vtkRenderer *ren, vtkActor *act);

//BTX
protected:
  vtkCompositePolyDataMapper2();
  ~vtkCompositePolyDataMapper2();

  // Description:
  // We need to override this method because the standard streaming
  // demand driven pipeline is not what we want - we are expecting
  // hierarchical data as input
  vtkExecutive* CreateDefaultExecutive();

  // Description:
  // Need to define the type of data handled by this mapper.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Need to loop over the hierarchy to compute bounds
  virtual void ComputeBounds();

  // Description:
  // Time stamp for computation of bounds.
  vtkTimeStamp BoundsMTime;

  // what "index" are we currently rendering, -1 means none
  int CurrentFlatIndex;
  std::map<const vtkShaderProgram *, bool> ShadersInitialized;
  std::map<const vtkDataSet *, vtkCompositeMapperHelper *> Helpers;

  // save resending uniforms every time
  bool GetShaderInitialized(vtkShaderProgram *);
  void SetShaderInitialized(vtkShaderProgram *, bool);

  class RenderBlockState
    {
  public:
    std::stack<bool> Visibility;
    std::stack<double> Opacity;
    std::stack<vtkColor3d> AmbientColor;
    std::stack<vtkColor3d> DiffuseColor;
    std::stack<vtkColor3d> SpecularColor;
    };

  RenderBlockState BlockState;
  void RenderBlock(vtkRenderer *renderer,
                   vtkActor *actor,
                   vtkDataObject *dobj,
                   unsigned int &flat_index);

  // Description:
  // Composite data set attributes.
  vtkSmartPointer<vtkCompositeDataDisplayAttributes> CompositeAttributes;

  friend class vtkCompositeMapperHelper;
  unsigned int ColorCoordinates;
  unsigned int NormalsOffset;
  unsigned int CoordComponents;

private:
  unsigned long int LastOpaqueCheckTime;
  bool LastOpaqueCheckValue;
  double ColorResult[3];

  vtkCompositePolyDataMapper2(const vtkCompositePolyDataMapper2&); // Not implemented.
  void operator=(const vtkCompositePolyDataMapper2&); // Not implemented.
//ETX
};

#endif
