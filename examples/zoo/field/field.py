# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <field> elements
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
import sympy
from sympy.abc import _clash
from sympy.utilities.lambdify import lambdify

# uncomment these for debugging
# sys.stderr = open('tool.err', 'w')
# sys.stdout = open('tool.out', 'w')

rx = Rappture.PyXml(sys.argv[1])

# Rather than using eval() which is unsafe and can collide with
# local variables, we use sympy to parse the symbolic expression.
# lambdify turns it into a callable function.
formula = rx['input.string(formula).current'].value
try:
        formula = sympy.sympify(formula, _clash)
except:
    raise ValueError("Formula is not a valid expression.")
formula = lambdify(formula.free_symbols, formula, modules=['numpy', 'mpmath', 'sympy'])

xmin, xmax = 0, 4
ymin, ymax = 0, 4
num_steps = 5

# Generate a uniform grid 2D mesh and field values...
m2d = rx['output.mesh(m2d)']
m2d['about.label'] = "2D Mesh"
m2d['dim'] = 2
m2d['units'] = "um"
m2d['hide'] = "yes"
m2d['grid.xaxis.min'] = xmin
m2d['grid.xaxis.max'] = xmax
m2d['grid.xaxis.numpoints'] = num_steps
m2d['grid.yaxis.min'] = ymin
m2d['grid.yaxis.max'] = ymax
m2d['grid.yaxis.numpoints'] = num_steps

f2d = rx['output.field(f2d)']
f2d['about.label'] = "2D Field"
f2d['component.mesh'] = "output.mesh(m2d)"

x = np.linspace(xmin, xmax, num_steps)
y = np.linspace(ymin, ymax, num_steps)

# Important. meshgrid defaults to matlab compatibility
# mode which transposes things. Tell it to use
# normal 'ij' indexing instead.  Or use mgrid()
xx, yy = np.meshgrid(x, y, indexing='ij')
pts = formula(xx, yy, 1)
f2d['component.values'] = pts

vizmethod = rx['input.choice(3D).current'].value

if vizmethod == "grid":
    #
    # Generate a uniform grid 3D mesh and field values...
    #
    m3d = rx['output.mesh(m3d)']
    m3d['about.label'] = "3D Uniform Mesh"
    m3d['dim'] = 3
    m3d['units'] = "um"
    m3d['hide'] = "yes"
    m3d['grid.xaxis.min'] = xmin
    m3d['grid.xaxis.max'] = xmax
    m3d['grid.xaxis.numpoints'] = 5
    m3d['grid.yaxis.min'] = ymin
    m3d['grid.yaxis.max'] = ymax
    m3d['grid.yaxis.numpoints'] = 5
    m3d['grid.zaxis.min'] = 0.0
    m3d['grid.zaxis.max'] = 1.0
    m3d['grid.zaxis.numpoints'] = 2

    f3d = rx['output.field(f3d)']
    f3d['about.label'] = "3D Field"
    f3d['component.mesh'] = "output.mesh(m3d)"

    xx, yy, zz = np.mgrid[xmin:xmax:5j, ymin:ymax:5j, 0:1:2j]
    pts = formula(xx, yy, zz)
    f3d['component.values'] = pts

if vizmethod == "unstructured":
    #
    # Generate an unstructured grid 3D mesh and field values...
    #
    m3d = rx['output.mesh(m3d)']

    m3d['about.label'] = "3D Unstructured Mesh"
    m3d['dim'] = 3
    m3d['units'] = "um"
    m3d['hide'] = "yes"

    f3d = rx['output.field(f3d)']
    f3d['about.label'] = "3D Field"
    f3d['component.mesh'] = "output.mesh(m3d)"

    x = np.linspace(xmin, xmax, 5)
    y = np.linspace(ymin, ymax, 5)
    z = np.linspace(0, 1, 2)
    xx, yy, zz = np.meshgrid(x, y, z, indexing='ij')

    # create list on index points with x changing fastest
    upts = ''
    for c in z:
        for b in y:
            for a in x:
                upts += "%s %s %s\n" % (a, b, c)

    # or you could use a list comprehension
    # upts = ' '.join(["%s %s %s\n" % (a, b, c) for c in z for b in y for a in x])
    m3d['unstructured.points'] = upts

    pts = formula(xx, yy, zz)
    f3d['component.values'] = pts

    m3d['unstructured.hexahedrons'] = """
    0 1 6 5 25 26 31 30
    1 2 7 6 26 27 32 31
    2 3 8 7 27 28 33 32
    3 4 9 8 28 29 34 33
    5 6 11 10 30 31 36 35
    8 9 14 13 33 34 39 38
    10 11 16 15 35 36 41 40
    13 14 19 18 38 39 44 43
    15 16 21 20 40 41 46 45
    16 17 22 21 41 42 47 46
    17 18 23 22 42 43 48 47
    18 19 24 23 43 44 49 48
    """

if vizmethod == "dx":
    #
    # Generate a uniform 3D mesh in OpenDX format...
    #
    f3d = rx['output.field(f3d)']
    f3d['about.label'] = "3D Field"
    f3d['component.style'] = "-color blue:yellow:red -levels 6"

    dx = """object 1 class gridpositions counts 5 5 2
        origin 0 0 0
        delta 1 0 0
        delta 0 1 0
        delta 0 0 1
        object 2 class gridconnections counts 5 5 2
        object 3 class array type double rank 0 items 50 data follows
    """

    xx, yy, zz = np.mgrid[xmin:xmax:5j, ymin:ymax:5j, 0:1:2j]
    pts = formula(xx, yy, zz)
    # Axis ordering for OpenDX is reversed (z fastest)
    dx += '\n'.join(map(str, (pts.ravel())))

    dx += """attribute "dep" string "positions"
    object "regular positions regular connections" class field
    component "positions" value 1
    component "connections" value 2
    component "data" value 3"""

    data = Rappture.encoding.encode(dx, Rappture.RPENC_ZB64)
    f3d['component.dx'] = data


rx.close()
