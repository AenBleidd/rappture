# ----------------------------------------------------------------------
#  EXAMPLE:  Rappture optimization for app-qdot
#
#  The purpose of this script is to use a scrpt like simple.tcl to interface with app-qdot.
#  The plotting functions created are done away with, since the problem is not confined to a 3-d space.
#  The inputs are configured for app-qdot. 
#  Run this as:  wish app-qdot.tcl ?-tool path/to/tool.xml?
#
# ======================================================================
#  AUTHOR:  Ganesh Hegde, Purdue University
#  Adapted from: simple.tcl
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
set auto_path [linsert $auto_path 0 /tmp/opt/lib]
package require BLT
package require Itcl
package require Rappture
package require RapptureGUI
set auto_path [linsert $auto_path 0 /home/ganesh/workspace/optim_post_dir_changes/src/lib]
package require -exact RapptureOptimizer 1.1
set popsize 100  ;# size of each population for genetic algorithm

set xml_path [lindex $argv 0]
puts $xml_path


# ----------------------------------------------------------------------
#  Create a Tool object based on the tool.xml file...
# ----------------------------------------------------------------------
Rappture::getopts argv params {
     value -tool tool.xml
}

after 2000
puts $params(-tool)
after 2000
#blt::bltdebug 100

# open the XML file containing the tool parameters
if {![file exists $params(-tool)]} {
    puts stderr "can't find tool \"$params(-tool)\""
    exit 1
}
set xmlobj [Rappture::library $params(-tool)]

set installdir [file dirname $params(-tool)]
if {"." == $installdir} {
    set installdir [pwd]
}

set tool [Rappture::Tool ::#auto $xmlobj $installdir]
# ----------------------------------------------------------------------
#  Create some plotting stuff so we can watch the progress of the
#  optimization as it runs.
# ----------------------------------------------------------------------
#blt::graph .space
#.space xaxis configure -title "x1" -min -2 -max 2
#.space yaxis configure -title "x2" -min -2 -max 2
#.space legend configure -hide no
#pack .space -side left -expand yes -fill both

#blt::graph .value
#.value xaxis configure -title "job number" -min 0
#.value yaxis configure -title "f(x1,x2)" -logscale yes -max 1
#.value legend configure -hide yes
#pack .value -side left -expand yes -fill both

#button .quit -text Quit -command "optim abort yes"
#pack .quit
#set colors {magenta purple blue DeepSkyBlue cyan green yellow Gold orange tomato red FireBrick black}

#for {set pop [expr [llength $colors]-1]} {$pop >= 0} {incr pop -1} {
#    blt::vector x1vec$pop
#    blt::vector x2vec$pop
#    .space element create spots$pop -xdata x1vec$pop -ydata x2vec$pop \
#        -color [lindex $colors $pop] -linewidth 0 -label "Population #$pop"

#    blt::vector jobvec$pop
#    blt::vector fvec$pop
#    .value element create line$pop -xdata jobvec$pop -ydata fvec$pop \
#        -color [lindex $colors $pop] -symbol none
#}


#set jobnumber 0
proc display_current_values {xmlobj} {
#    x1vec$pop append [$xmlobj get input.number(x1).current]
#    x2vec$pop append [$xmlobj get input.number(x2).current]
    set xl  [$xmlobj get output.number(xl).current]
    set yl  [$xmlobj get output.number(yl).current]
    set zl  [$xmlobj get output.number(zl).current]
    set Ef  [$xmlobj get output.number(Ef).current]
    set temp  [$xmlobj get output.number(temp).current]
    set theta  [$xmlobj get output.number(theta).current]
    set phi  [$xmlobj get output.number(phi).current]
    set fitness [$xmlobj get output.number(f).current]
    set max_string [$xmlobj get  output.string(maxlist).current]
    puts "Device and Light Paramters for this evaluation: Xl = $xl nm,  Yl = $yl nm,   Zl = $zl nm,   Ef = $Ef eV,   temp = $temp K,  theta = $theta,   deg phi = $phi deg"
    puts "The fitness in this evaluation is $fitness................."
    puts "Maxima(s) at x equals $max_string"
}

#proc find_fitness {xmlobj} {
#	set fitness 1000 #Arbitrarily setting fitness to a large value
#	set opt_abs_curve output.curve(abs).component.xy
#	puts $opt_abs_curve
#	set index 0
#	set xytuples [split $opt_abs_curve "\n"]
#	set no_of_curve_points [llength $xytuples]
#	foreach xytuple $xytuples {
#		set tuple [split $xytuple " "]
#		set x($index) [lindex $tuple 0]
#		set y($index) [lindex $tuple 1]
#		puts $x($index)
#		puts $y($index)
#		incr index
#	}
#	set size [llength xytuples]
#	for {set index 0} {$index <= ($size-1)} {incr index} {
#		if($index != ($size-2)){
#			set slope_at_x (($y($index+1)-$y($index))/($x($index+1)-$x($index)))
#			set slope_x_plus_one (($y($index+2)-$y($index+1))/($x($index+2)-$x($index+1)))
#		}else{
#			set slope_x_plus_one (($y($index)-$y($index-1))/($x($index)-$x($index-1)))
#			set slope_at_x (($y($index-1)-$y($index-2))/($x($index-1)-$x($index-2)))
#		}
#		if(slope_at_x > 0 && slope_x_plus_one <=0){
#			set maxima_point $x($index)
#			set fitness abs($maxima_point-2)
#			break
#			 ####Max required at 2 eV, so minimize the difference between the current max and required max 
#		}
#
#	}
#
#$xmlobj put output.number(f).about.label "Optimization of Absorption Plot for Qdot"
#$xmlobj put output.number(f).about.description \
#    "Fitness = abs(difference between first maxima in the current run and required value (Here 2 eV))"
#$xmlobj put output.number(f).current $fitness
#
#}

# ----------------------------------------------------------------------
#  Create an optimization context and configure the parameters used
#  for optimization...
# ----------------------------------------------------------------------
Rappture::optimizer optim -tool $tool -using pgapack

optim add number input.group(tabs).group(geometry).structure.current.parameters(boxparams).number(xlen) -min 5 -max 40 -randdist uniform -strictmin yes -strictmax yes -units nm
optim add number input.group(tabs).group(geometry).structure.current.parameters(boxparams).number(ylen)  -min 5 -max 40 -randdist uniform -strictmin yes -strictmax yes -units nm
optim add number input.group(tabs).group(geometry).structure.current.parameters(boxparams).number(zlen)  -min 5 -max 20 -randdist uniform -strictmin yes -strictmax yes -units nm
optim add number input.group(tabs).(light).number(Ef)  -min 0 -max 5 -randdist uniform -strictmin yes -strictmax yes -units eV
optim add number input.group(tabs).group(light).number(temperature)  -min 0 -max 1000 -randdist gaussian -mean 300 -stddev 10 -strictmin yes -strictmax yes -units K
optim add number input.group(tabs).group(light).number(theta)  -min 0 -max 90 -randdist uniform -stddev 0.5 -strictmin yes -strictmax yes -units deg
optim add number input.group(tabs).group(light).number(phi)  -min 0 -max 360 -randdist uniform  -strictmin yes -strictmax yes -units deg

optim configure -operation minimize -popsize 50 -maxruns 100 -mutnrate 0.1 -crossovrate 0.9 -randnumseed 9 -stpcriteria toosimilar

optim get configure 

set status [optim perform -fitness output.number(f).current -updatecommand display_current_values]
puts "done: $status"
