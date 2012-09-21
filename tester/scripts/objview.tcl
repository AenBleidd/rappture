# ----------------------------------------------------------------------
#  COMPONENT: objview - show an overview of a Rappture object
#
#  This component is used when examining the details of a particular
#  diff.  It shows an overview of the affected object, so the user
#  can understand where the diff is coming from.  The overview can be
#  short or long, and the widget can show the object itself or any
#  diffs in the test object.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require RapptureGUI

namespace eval Rappture::Tester { # forward declaration }

option add *ObjView.headingFont {Arial -12 bold} widgetDefault
option add *ObjView.infoFont {Arial -12} widgetDefault
option add *ObjView.codeFont {Courier -12} widgetDefault
option add *ObjView.addedBackground #ccffcc widgetDefault
option add *ObjView.deletedBackground #ffcccc widgetDefault

itcl::class Rappture::Tester::ObjView {
    inherit itk::Widget 

    itk_option define -testobj testObj TestObj ""
    itk_option define -path path Path ""
    itk_option define -details details Details "min"
    itk_option define -showdiffs showDiffs ShowDiffs "off"

    itk_option define -addedbackground addedBackground Background ""
    itk_option define -deletedbackground deletedBackground Background ""
    itk_option define -infofont infoFont Font ""
    itk_option define -codefont codeFont Font ""

    constructor {args} { # defined later }

    protected method _reload {}

    private variable _dispatcher ""  ;# dispatcher for !events
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::ObjView::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !reload
    $_dispatcher dispatch $this !reload "[itcl::code $this _reload]; list"

    # object icon on the left-hand side
    itk_component add icon {
        label $itk_interior.icon
    }

    # create label/value for "path" parameter -- this is a special case
    itk_component add lpath {
        label $itk_interior.lpath -text "Identifier:"
    } {
        usual
        rename -font -headingfont headingFont Font
    }

    itk_component add vpath {
        label $itk_interior.vpath -justify left -anchor w
    } {
        usual
        rename -font -codefont codeFont Font
    }

    # set up the layout to resize properly
    grid columnconfigure $itk_interior 2 -weight 1

    eval itk_initialize $args
}

itk::usual ObjView {
    keep -background -foreground -cursor
}

# ----------------------------------------------------------------------
# USAGE: _reload
#
# Called internally to load information from the object at the
# current -path in the -testobj XML definition.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::ObjView::_reload {} {
    set testobj $itk_option(-testobj)
    set path $itk_option(-path)

    # clear everything out
    foreach win [grid slaves $itk_interior] {
        grid forget $win
    }
    grid $itk_component(icon) -row 0 -rowspan 6 -column 0 \
        -sticky ne -padx {0 20}

    # look for an object at the path
    set type ""
    if {$testobj ne "" && $path ne ""} {
        set type [$testobj getTestInfo element -as type $path]
        if {$type eq ""} {
            set type [$testobj getRunInfo element -as type $path]
        }
    }

    if {$type eq ""} {
        $itk_component(icon) configure -image ""
    } else {
        set icon [Rappture::objects::get $type -image]
        $itk_component(icon) configure -image $icon

        # decide what to show -- short or long format
        set items {label description path}
        foreach rec [Rappture::objects::get $type -attributes] {
            set name [lindex $rec 0]
            array set attr [lrange $rec 1 end]
            set titles($name) $attr(-title)
            set loc($name) $path.$attr(-path)
            set path2attr($path.$attr(-path)) $name

            if {$itk_option(-details) == "max" && [lsearch $items $name] < 0} {
                lappend items $name
            }
        }

        # if we're showing diffs, query them now
        if {$itk_option(-showdiffs)} {
            foreach rec [$testobj getDiffs $path] {
                catch {unset df}
                array set df $rec
                if {[lindex $df(-what) 0] eq "attr"} {
                    set apath $df(-path).[lindex $df(-what) 2]

                    if {[info exists path2attr($apath)]} {
                        # store the diff for this attribute -- we need it below
                        set name $path2attr($apath)
                        set diffs($name) [list [lindex $df(-what) 1] $df(-v1) $df(-v2)]
                    }
                }
            }
        }

        # run through the items and show values
        set row 0
        foreach aname $items {
            set show 0
            if {$aname == "path"} {
                # use a special widget to handle the "path" report
                set lcomp "lpath"
                set vcomp "vpath"
                $itk_component(vpath) configure -text $path
                set show 1
            } else {
                # get a label with the attribute name for this row
                set lcomp "lattr$row"
                if {![info exists itk_component($lcomp)]} {
                    itk_component add $lcomp {
                        label $itk_interior.$lcomp
                    } {
                        usual
                        rename -font -headingfont headingFont Font
                    }
                }
                $itk_component($lcomp) configure -text "$titles($aname):"

                set vcomp ""
                set bg [cget -background]
                set fn $itk_option(-infofont)

                if {[info exists diffs($aname)]} {
                    set show 1
                    set fn $itk_option(-codefont)

                    foreach {op v1 v2} $diffs($aname) break
                    switch -- $op {
                        - {
                            # use a normal label below, but change the bg
                            set bg $itk_option(-deletedbackground)
                            set str $v1
                        }
                        + {
                            # use a normal label below, but change the bg
                            set bg $itk_option(-addedbackground)
                            set str $v2
                        }
                        c {
                            # get a value that can show diffs on this row
                            set vcomp "vdiff$row"
                            if {![info exists itk_component($vcomp)]} {
                                itk_component add $vcomp {
                                    Rappture::Scroller \
                                        $itk_interior.$vcomp \
                                        -xscrollmode auto -yscrollmode auto \
                                        -height 1i
                                }
                                itk_component add ${vcomp}val {
                                    Rappture::Diffview \
                                        $itk_component($vcomp).dv \
                                        -borderwidth 0 \
                                        -diff 1->2 -layout inline
                                }
                                $itk_component($vcomp) contents \
                                    $itk_component(${vcomp}val)
                            }
                            set dv $itk_component(${vcomp}val)
                            $dv text 1 $v1
                            $dv text 2 $v2

                            # more than a 1-line diff? then show scrollbars
                            if {[llength [split $v1 \n]] > 1
                                 || [llength [split $v2 \n]] > 1} {
                                $itk_component($vcomp) configure \
                                    -yscrollmode auto -height 1i
                            } else {
                                foreach {x0 y0 x1 y1} [$dv bbox all] break
                                set bd [$dv cget -borderwidth]
                                set yht [expr {$y1-$y0+2*$bd}]
                                $itk_component($vcomp) configure \
                                    -yscrollmode off -height $yht
                            }
                        }
                    }
                } else {
                    # no diff -- get the attribute value
                    set str [$testobj getTestInfo $loc($aname)]
                }

                # don't have a diff comp from above? then create a viewer here
                if {$vcomp eq "" && $str ne ""} {
                    set show 1
                    set lines [split $str \n]
                    if {[llength $lines] > 2
                          || [string length [lindex $lines 0]] > 40
                          || [string length [lindex $lines 1]] > 40} {

                        # long string -- use a scroller and a text area
                        set vcomp "vattrlong$row"
                        if {![info exists itk_component($vcomp)]} {
                            itk_component add $vcomp {
                                Rappture::Scroller $itk_interior.$vcomp \
                                    -xscrollmode auto -yscrollmode auto \
                                    -height 1i
                            }
                            itk_component add ${vcomp}val {
                                text $itk_component($vcomp).tx -wrap word
                            }
                            $itk_component($vcomp) contents \
                                $itk_component(${vcomp}val)
                        }
                        set tx $itk_component(${vcomp}val)
                        $tx configure -state normal
                        $tx delete 1.0 end
                        $tx insert end $str
                        $tx configure -state disabled -background $bg -font $fn
                    } else {
                        # short string -- use a 1-line label
                        set vcomp "vattr$row"
                        if {![info exists itk_component($vcomp)]} {
                            itk_component add $vcomp {
                                label $itk_interior.$vcomp \
                                    -justify left -anchor w
                            } {
                                usual
                                rename -font -infofont infoFont Font
                            }
                        }
                        $itk_component($vcomp) configure -text $str \
                            -background $bg -font $fn
                    }
                }

            }
            grid rowconfigure $itk_interior $row -weight 0

            if {$show} {
                grid $itk_component($lcomp) -row $row -column 1 \
                    -sticky e -pady 2
                grid $itk_component($vcomp) -row $row -column 2 \
                    -sticky ew -pady 2
                incr row
            }
        }
        grid rowconfigure $itk_interior $row -weight 1
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTIONS
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::ObjView::testobj {
    $_dispatcher event -idle !reload
}
itcl::configbody Rappture::Tester::ObjView::path {
    $_dispatcher event -idle !reload
}
itcl::configbody Rappture::Tester::ObjView::details {
    if {[lsearch {min max} $itk_option(-details)] < 0} {
        error "bad value \"$itk_option(-details)\": should be min, max"
    }
    $_dispatcher event -idle !reload
}
itcl::configbody Rappture::Tester::ObjView::showdiffs {
    if {![string is boolean -strict $itk_option(-showdiffs)]} {
        error "bad value \"$itk_option(-showdiffs)\": should be boolean"
    }
    $_dispatcher event -idle !reload
}
