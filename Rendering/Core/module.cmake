vtk_module(vtkRenderingCore
  GROUPS
    Rendering
  DEPENDS
    vtkCommonExecutionModel
    vtkCommonTransforms
  PRIVATE_DEPENDS
    vtkFiltersSources
    vtkFiltersGeometry
    vtkFiltersExtraction
    vtksys
  TEST_DEPENDS
    vtkIOLegacy
    vtkIOXML
    vtkTestingCore
    vtkTestingRendering
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingFreeType
    vtkRenderingAnnotation
    vtkInteractionStyle
    vtkImagingSources
    vtkFiltersSources
    vtkFiltersProgrammable
  KIT
    vtkRendering
  )
