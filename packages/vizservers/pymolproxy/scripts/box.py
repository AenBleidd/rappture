#
# box.py -- create a Compiled Graphics Object for a unit cell box

from pymol.cgo import *
from pymol import cmd

# Arguments: 
#   lv: List of vertices; each vertex is a [x,y,z] list
#   lw: Line width
#    r: Red component of box color
#    g: Green component of box color
#    b: Blue component of box color
# name: Name of box, which can be used with other PyMOL commands
def draw_box(lv, lw=2.0, r=0.5, g=0.5, b=0.5, name='unitcell'):
   box = [LINEWIDTH, lw, BEGIN, LINES, COLOR, r, g, b]
   for v in lv:
      box.append(VERTEX)
      box.append(v[0])
      box.append(v[1])
      box.append(v[2])
   box.append(END)
   cmd.load_cgo(box, name)

# Extend the PyMOL interpeter with the new "draw_box" command
cmd.extend("draw_box", draw_box)
