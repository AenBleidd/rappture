# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <mesh> elements
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

import Rappture
import sys
import numpy as np

# uncomment these for debugging
# sys.stderr = open('mesh.err', 'w')
# sys.stdout = open('mesh.out', 'w')

rx = Rappture.PyXml(sys.argv[1])

meshtype = rx["input.choice(mesh).current"].value
contour = rx["input.boolean(contour).current"].value

if contour == "yes":
    view = 'contour'
else:
    view = 'heightmap'

mesh = rx['output.mesh']
mesh['dim'] = 2
mesh['units'] = "m"
mesh['hide'] = "yes"

x = np.linspace(0, 1, num=50, endpoint=True)
y = np.linspace(0, 1, num=50, endpoint=True)


if meshtype == 'cloud':
    mesh['about.label'] = "cloud in unstructured mesh"

    # Warning! Unstructured grids require newlines after each
    # set of points!.

    # use mgrid. weird syntax, but compact
    # x, y = np.mgrid[0:1:50j, 0:1:50j]
    # pts = (x.ravel('F'), y.ravel('F'))

    # OR create x and y vectors then use one of these methods

    # Use list comprehension to create string
    pts = ' '.join(["%s %s\n" % (a, b) for b in y for a in x])

    # Use meshgrid. Similar to mgrid()
    # X, Y = np.meshgrid(x, y)
    # pts = (X.ravel(), Y.ravel())

    mesh['unstructured.points'] = pts
    mesh['unstructured.celltypes'] = ""

if meshtype == 'regular':
    mesh['about.label'] = "uniform grid mesh"
    mesh['grid.xaxis.min'] = 0
    mesh['grid.xaxis.max'] = 1
    mesh['grid.xaxis.numpoints'] = 50
    mesh['grid.yaxis.min'] = 0
    mesh['grid.yaxis.max'] = 1
    mesh['grid.yaxis.numpoints'] = 50

if meshtype == 'irregular':
    mesh['about.label'] = "irregular grid mesh"
    mesh['grid.xcoords'] = x
    mesh['grid.ycoords'] = y

if meshtype == 'hybrid':
    mesh['about.label'] = "hybrid regular and irregular grid mesh"
    mesh['grid.xcoords'] = x
    mesh['grid.yaxis.min'] = 0
    mesh['grid.yaxis.max'] = 1
    mesh['grid.yaxis.numpoints'] = 50

if meshtype == 'triangular':
    mesh['about.label'] = "triangles in unstructured mesh"
    mesh.put('unstructured.points', 'points.txt', type='file')
    mesh.put('unstructured.triangles', 'triangles.txt', type='file')

if meshtype == 'unstructured':
    mesh['about.label'] = "Unstructured Grid"
    mesh['unstructured.celltypes'] = 'triangle'
    mesh.put('unstructured.points', 'points.txt', type='file')
    mesh.put('unstructured.cells', 'triangles.txt', type='file')

if meshtype == 'cells':
    # FIXME. Copied from tcl example, but looks just like 'unstructured'
    mesh['about.label'] = "Unstructured Grid with Heterogeneous Cells"
    mesh['unstructured.celltypes'] = 'triangle'
    mesh.put('unstructured.points', 'points.txt', type='file')
    mesh.put('unstructured.cells', 'triangles.txt', type='file')

if meshtype == 'vtkmesh':
    mesh['about.label'] = 'vtk mesh'
    mesh.put('vtk', 'mesh.vtk', type='file')

if meshtype == 'vtkfield':
    f = rx['output.field(substrate)']
    f['about.label'] = "Substrate Surface"
    f.put('component.vtk', 'file.vtk', type='file')
    rx.close()

f = rx['output.field(substrate)']
f['about.label'] = "Substrate Surface"
f['about.view'] = view
f['component.mesh'] = mesh.name
f.put('component.values', 'substrate_data.txt', type='file')

f = rx['output.field(particle)']
f['about.label'] = "Particle Surface"
f['about.view'] = view
f['component.mesh'] = mesh.name
f.put('component.values', 'particle_data.txt', type='file')

rx['output.string.about.label'] = "Mesh XML definition"
rx['output.string.current'] = mesh.xml()

rx.close()
