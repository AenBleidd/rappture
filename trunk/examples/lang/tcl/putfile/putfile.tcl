package require Rappture

# open the XML file containing the run parameters
set opt [Rappture::library [lindex $argv 0]]

if {[$opt isa ::Rappture::LibraryObj] != 1} {
    puts "creation of library failed"
}

$opt put output.string(datafile).about.label "Binary Data File"
$opt put -type file -compress yes output.string(datafile).current addrbook.mat
$opt put output.string(datafile).filetype ".mat"

$opt put output.string(tarfile).about.label "Binary TarGZ File"
$opt put -type file -compress yes output.string(tarfile).current addrbook.tgz
$opt put output.string(tarfile).filetype ".tgz"

Rappture::result $opt
exit 0
