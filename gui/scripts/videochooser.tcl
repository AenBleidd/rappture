# ----------------------------------------------------------------------
#  COMPONENT: videochooser - list videos
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2005-2010  Purdue Research Foundation
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

package require Itk
package require BLT
package require Img
package require Rappture
package require RapptureGUI

itcl::class Rappture::VideoChooser {
    inherit itk::Widget

    itk_option define -width width Width 300
    itk_option define -height height Height 300
    itk_option define -variable variable Variable ""

    constructor { args } {
        # defined below
    }
    destructor {
        # defined below
    }

    public method load {path}
    public method choose {path}
    protected method _fixSize {}

    private variable _vcnt 0            ;# counter of videos
    private variable _variable ""       ;# variable to pass along to each choice
    private variable _objs ""           ;# list of choice objects
}


itk::usual VideoChooser {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooser::constructor {args} {

    itk_component add scroller {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode auto \
            -yscrollmode off \
            -xscrollside bottom
    }


    $itk_component(scroller) contents frame


    pack $itk_component(scroller) -fill both -expand yes

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooser::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: load
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooser::load {paths} {

    set f [$itk_component(scroller) contents]

    foreach path $paths {
        incr _vcnt

        set ci [Rappture::VideoChooserInfo $f.vi${_vcnt} \
                    -variable ${_variable}]
        $ci load $path ""
        pack $ci -expand yes -fill both -side right

        # add the object to the list of video info objs
        lappend _objs $ci
    }

    _fixSize
}


# ----------------------------------------------------------------------
# USAGE: _fixSize
# ----------------------------------------------------------------------
itcl::body Rappture::VideoChooser::_fixSize {} {

    # fix the dimesions of the canvas
}

# ----------------------------------------------------------------------
# CONFIGURE: -variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoChooser::variable {
    set _variable $itk_option(-variable)

    if {"" != $_variable} {
        upvar #0 $_variable var

        foreach o ${_objs} {
            # tell this object's children about the variable change
            $o configure -variable $var
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoChooser::width {
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
itcl::configbody Rappture::VideoChooser::height {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-height)] == 0} {
        error "bad value: \"$itk_option(-height)\": height should be an integer"
    }
    set _height $itk_option(-height)
    after idle [itcl::code $this _fixSize]
}
