

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

set cell_vertices {
    0.0, 0.0, 0.0,
    2.71, 2.71, 0.0,
    0.0, 0.0, 0.0,
    2.71, 0.0, 2.71,
    0.0, 0.0, 0.0,
    0.0, 2.71, 2.71,
    2.71, 2.71, 0.0,
    5.42, 2.71, 2.71,
    2.71, 0.0, 2.71,
    5.42, 2.71, 2.71,
    2.71, 0.0, 2.71,
    2.71, 2.71, 5.42,
    2.71, 2.71, 0.0,
    2.71, 5.42, 2.71,
    0.0, 2.71, 2.71,
    2.71, 5.42, 2.71,
    5.42, 5.42, 5.42,
    2.71, 5.42, 2.71,
    5.42, 2.71, 2.71,
    5.42, 5.42, 5.42,
    0.0, 2.71, 2.71,
    2.71, 2.71, 5.42,
    5.42, 5.42, 5.42,
    2.71, 2.71, 5.42
}
$driver put -append no $components.molecule.pdb $pdb
$driver put -append no $components.cell.values $cell_vertices

Rappture::result $driver 
exit 0