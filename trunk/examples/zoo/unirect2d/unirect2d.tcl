
# Example of using unirect2d meshes in a field object in Rappture.

package require Rappture

# Read in the data since we're not simulating anything...
source data.tcl

# Open an XML run file to write into
set driver [Rappture::library [lindex $argv 0]]

# Create a unirect2d mesh.  The mesh will be used in the "field" object
# below. We will specify a mesh and then provide the data points to the field.

$driver put unirect2d(mygrid).about.label "Fermi-Dirac Factor"
$driver put unirect2d(mygrid).xaxis.label "Fermi-Dirac Factor"
$driver put unirect2d(mygrid).yaxis.label "Energy"

# The uniform grid that we are specifying is a mesh of 50 points on the x-axis
# and 50 points on the y-axis.  The 50 points are equidistant between 0.0 and
# 1.0

# Specify the x-axis of the mesh
$driver put unirect2d(mygrid).xaxis.min 0.0 
$driver put unirect2d(mygrid).xaxis.max 1.0
$driver put unirect2d(mygrid).xaxis.numpoints 50

# Specify the y-axis of the mesh
$driver put unirect2d(mygrid).yaxis.min 0.0 
$driver put unirect2d(mygrid).yaxis.max 1.0
$driver put unirect2d(mygrid).yaxis.numpoints 50


# Create 2 field objects for the two sets of data we have (particle and
# surface).  Note that we can use the same unirect2d mesh in more than one
# "field" object.

# Create a "field" object to display the substrate surface.

$driver put output.field(one).about.label "Substrate Surface"
$driver put output.field(one).component.mesh "unirect2d(mygrid)"
$driver put output.field(one).component.values $substrate_data

# Create another "field" object to display the particle surface.

$driver put output.field(two).about.label "Particle Surface"
$driver put output.field(two).component.mesh "unirect2d(mygrid)"
$driver put output.field(two).component.values $particle_data

# save the updated XML describing the run...
Rappture::result $driver
exit 0

