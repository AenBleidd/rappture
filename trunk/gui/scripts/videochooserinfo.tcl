# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: videochooserinfo - video info
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

package require Itk
package require BLT
package require Img
package require Rappture

itcl::class Rappture::VideoChooserInfo {
    inherit itk::Widget

    itk_option define -width width Width 96
    itk_option define -height height Height 54
    itk_option define -selectedwidth selectedwidth Selectedwidth 112
    itk_option define -selectedheight selectedheight Selectedheight 63
    itk_option define -variable variable Variable ""
    itk_option define -thumbsdir thumbsdir Thumbsdir ""

    constructor { args } {
        # defined below
    }
    destructor {
        # defined below
    }

    public method load {path about}
    public method select {}
    public method query {what}

    protected method _fixSize {}
    protected method _fixValue {args}

    private method _bindings {sequence}

    private variable _preview           ""
    private variable _selected          ""
    private variable _path              ""
    private variable _width             0
    private variable _height            0
    private variable _selectedwidth     0
    private variable _selectedheight    0
    private variable _variable          "" ;# variable associated with -variable
}


itk::usual VideoChooserInfo {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooserInfo::constructor {args} {

    itk_component add main {
        canvas $itk_interior.main
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    bind $itk_component(main) <Configure> [itcl::code $this _fixSize]

    blt::table $itk_interior \
        0,0 $itk_component(main) -fill both -pady 2

    blt::table configure $itk_interior c* -resize both
    blt::table configure $itk_interior r0 -resize both

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooserInfo::destructor {} {
    image delete ${_preview}
    image delete ${_selected}
}

# ----------------------------------------------------------------------
# USAGE: load
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooserInfo::load {path about} {

    if {![file readable $path]} {
        error "bad path $path: file not readable"
    }

    set _path $path
    set preview_fname ""
    set selected_fname ""

    set _preview [image create photo]
    set _selected [image create photo]

    if {[string compare "" $itk_option(-thumbsdir)] != 0} {
        set root [file tail [file rootname ${_path}]]
        set preview_fname [file join $itk_option(-thumbsdir) ${root}_${_width}x${_height}.png]
        set selected_fname [file join $itk_option(-thumbsdir) ${root}_${_selectedwidth}x${_selectedheight}.png]

        if {[file readable $preview_fname] && [file readable $selected_fname]} {
            set fid [open $preview_fname r]
            fconfigure $fid -translation binary
            ${_preview} put [read $fid]
            close $fid

            set fid [open $selected_fname r]
            fconfigure $fid -translation binary
            ${_selected} put [read $fid]
            close $fid
        }
    } else {
        # there are some incomplete movies that don't open up
        set err [catch {
            set movie [Rappture::Video file ${_path}]
            $movie seek 60

            ${_preview} put [$movie get image ${_width} ${_height}]
            ${_selected} put [$movie get image ${_selectedwidth} ${_selectedheight}]

            $movie release
        }]
        if {$err} {
            puts stderr "Error while opening movie: $movie"
            return
        }
    }

    set cw [expr round(${_selectedwidth}/2.0)]
    set ch [expr round(${_selectedheight}/2.0)]
    $itk_component(main) create image $cw $ch \
        -anchor center \
        -image ${_preview} \
        -activeimage ${_selected} \
        -tags preview

    $itk_component(main) bind preview <ButtonPress-1> [itcl::code $this _bindings b1press]
    $itk_component(main) bind preview <Enter> [itcl::code $this _bindings enter]
    $itk_component(main) bind preview <Leave> [itcl::code $this _bindings leave]

    _fixSize
}

# ----------------------------------------------------------------------
# USAGE: select
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooserInfo::select {} {
    if {"" == $_variable} {
        return
    }
    # send this object's movie to the preview window

    upvar #0 $_variable var
    set var ${_path}

    foreach {x0 y0 x1 y1} [$itk_component(main) bbox preview] break
    set x0 [expr 0]
    set y0 [expr 2]
    set x1 [expr [$itk_component(main) cget -width]-2]
    set y1 [expr [$itk_component(main) cget -height]-2]

    # there is something wrong with the way we place the
    # images on the canvas, or how the canvas is shapeed.
    # the placement of the small image seems to be off
    # by a pixel or two. for now we fudge the numbers to
    # make the red line accesnts look decent.

    $itk_component(main) create line $x0 $y0 $x1 $y0 \
        -fill red \
        -width 3 \
        -tags previewselected

    $itk_component(main) create line $x0 $y1 $x1 $y1 \
        -fill red \
        -width 3 \
        -tags previewselected

#    $itk_component(main) create rectangle \
        [$itk_component(main) bbox preview] \
        -outline red \
        -width 4 \
        -tags previewselected
#    $itk_component(main) configure -background red
}


# ----------------------------------------------------------------------
# USAGE: _bindings
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooserInfo::_bindings {sequence} {
    switch -- $sequence {
        "b1press" {
            select
        }
        "enter" {
            $itk_component(main) itemconfigure previewselected -state hidden
        }
        "leave" {
            $itk_component(main) itemconfigure previewselected -state normal
        }
        default {}
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixValue ?<name1> <name2> <op>?
#
# Invoked automatically whenever the -variable associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooserInfo::_fixValue {args} {
    if {"" == $itk_option(-variable)} {
        return
    }
    upvar #0 $itk_option(-variable) var
    if {[string compare $var ${_path}] != 0} {
        # unselect this object
        $itk_component(main) delete previewselected
        #$itk_component(main) configure -background black
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixSize
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooserInfo::_fixSize {} {
    $itk_component(main) configure -width ${_selectedwidth} -height ${_selectedheight}
}

# ----------------------------------------------------------------------
# USAGE: query
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooserInfo::query {what} {
    switch -- $what {
        "preview" {return ${_preview}}
        "selected" {return ${_selected}}
        "file" {return ${_path}}
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoChooserInfo::variable {
    if {"" != $_variable} {
        upvar #0 $_variable var
        trace remove variable var write [itcl::code $this _fixValue]
    }

    set _variable $itk_option(-variable)

    if {"" != $_variable} {
        upvar #0 $_variable var
        trace add variable var write [itcl::code $this _fixValue]

        # sync to the current value of this variable
        if {[info exists var]} {
            _fixValue
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoChooserInfo::width {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-width)] == 0} {
        error "bad value: \"$itk_option(-width)\": width should be an integer"
    }
    set _width $itk_option(-width)
    after idle [itcl::code $this _fixSize]
}

# ----------------------------------------------------------------------
# OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoChooserInfo::height {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-height)] == 0} {
        error "bad value: \"$itk_option(-height)\": height should be an integer"
    }
    set _height $itk_option(-height)
    after idle [itcl::code $this _fixSize]
}

# ----------------------------------------------------------------------
# OPTION: -selectedwidth
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoChooserInfo::selectedwidth {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-selectedwidth)] == 0} {
        error "bad value: \"$itk_option(-selectedwidth)\": selectedwidth should be an integer"
    }
    set _selectedwidth $itk_option(-selectedwidth)
    after idle [itcl::code $this _fixSize]
}

# ----------------------------------------------------------------------
# OPTION: -selectedheight
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoChooserInfo::selectedheight {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-selectedheight)] == 0} {
        error "bad value: \"$itk_option(-selectedheight)\": selectedheight should be an integer"
    }
    set _selectedheight $itk_option(-selectedheight)
    after idle [itcl::code $this _fixSize]
}
