
set exec_prefix "/usr/local/rappture-trunk"
set bindir "${exec_prefix}/bin"
set libdir "${exec_prefix}/lib"

# NanoVis server --
#
register_server nanovis 2000 {
    ${bindir}/nanovis -p ${libdir}/shaders:${libdir}/resources
} {
  LD_LIBRARY_PATH ${libdir}
}

# VtkVis server --
#    3D drawings (scene graphs), Contour and surface graphs.
#
register_server vtkvis 2010 {
    ${bindir}/vtkvis
} {
  LD_LIBRARY_PATH ${libdir}:${libdir}/vtk-5.6
}

# Pymol server --
#    Molecular layouts from PDB description.  Must set PYMOL_PATH.
#    
register_server pymol 2020 {
    ${bindir}/pymolproxy ${bindir}/pymol -p -q -i -x -X 0 -Y 0
} {
   PYMOL_PATH ${libdir}/pymol
}
