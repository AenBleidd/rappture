

package require Rappture

set driver [Rappture::library [lindex $argv 0]]

set components "output.structure.components"

set pdb {
ATOM      0   Si UNK A   1       0.000   0.000   0.000  1.00
ATOM      1   Si UNK A   1       2.710   2.710   0.000  1.00
ATOM      2   Si UNK A   1       2.710   0.000   2.710  1.00
ATOM      3   Si UNK A   1       0.000   2.710   2.710  1.00
ATOM      4   Si UNK A   1       5.420   2.710   2.710  1.00
ATOM      5   Si UNK A   1       2.710   5.420   2.710  1.00
ATOM      6   Si UNK A   1       2.710   2.710   5.420  1.00
ATOM      7   Si UNK A   1       5.420   5.420   5.420  1.00
ATOM      8   Si UNK A   1       1.355   1.355   1.355  1.00
TER       9      UNK A   1
}

$driver put $components.molecule.pdb $pdb

$driver put $components.parallelepiped.origin "0.0 0.0 0.0"
$driver put $components.parallelepiped.scale "5.42"
#$driver put $components.parallelepiped.scale "1.0"
 
$driver put $components.parallelepiped.vector(1) ".5 .5 .0"
$driver put $components.parallelepiped.vector(2) ".5 .0 .5"
$driver put $components.parallelepiped.vector(3) ".0 .5 .5"

Rappture::result $driver 
exit 0
