# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: ImageEntry - widget for displaying images
#
#  This widget represents an <image> entry on a control panel.
#  It displays images which help explain the input.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require Img

itcl::class Rappture::ImageEntry {
    inherit itk::Widget

    itk_option define -state state State "normal"

    constructor {owner path args} { # defined below }
    destructor { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _redraw {}
    protected method _outline {imh color}
    protected method _uploadValue {args}
    protected method _downloadValue {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this image
    private variable _data ""     ;# current image data
    private variable _imh ""      ;# image handle for current value
    private variable _resize ""   ;# image for resize operations

    private common _thumbsize 100 ;# std size for thumbnail images
}

itk::usual ImageEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path
    set _resize [image create photo]

    #
    # Create the widget and configure it properly based on other
    # hints in the XML.  Two ways to display:  Old apps use images
    # without labels as decorations.  In that case, show the image
    # alone, probably full size.  Newer apps use images as inputs.
    # In that case, show a thumbnail of the image with some extra
    # facts about image type, file size, etc.
    #
    itk_component add image {
        ::label $itk_interior.image -borderwidth 0
    }

    itk_component add info {
        ::label $itk_interior.info -borderwidth 0 -width 5 \
            -anchor w -justify left
    }

    itk_component add rmenu {
        menu $itk_interior.menu -tearoff 0
    } {
        usual
        ignore -tearoff
    }
    $itk_component(rmenu) add command \
        -label [Rappture::filexfer::label upload] \
        -command [itcl::code $this _uploadValue -start]
    $itk_component(rmenu) add command \
        -label [Rappture::filexfer::label download] \
        -command [itcl::code $this _downloadValue]


    if {[string length [label]] == 0} {
        # old mode -- big image
        pack $itk_component(image) -expand yes -fill both
        bind $itk_component(image) <Configure> [itcl::code $this _redraw]
    } else {
        # new mode -- thumbnail and details
        pack $itk_component(image) -side left
        pack $itk_component(info) -side left -expand yes -fill both -padx 4

        bind $itk_component(image) <<PopupMenu>> \
            [list tk_popup $itk_component(rmenu) %X %Y]
        bind $itk_component(info) <<PopupMenu>> \
            [list tk_popup $itk_component(rmenu) %X %Y]

        _redraw  ;# draw Empty image/info
    }

    # Don't trim this string.  It make be important.
    set str [$_owner xml get $path.current]
    if {[string length $str] == 0} {
        set str [$_owner xml get $path.default]
    }
    if {[string length $str] > 0} {
        value $str
    }

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::destructor {} {
    if {"" != $_imh} { image delete $_imh }
    if {"" != $_resize} { image delete $_resize }
}

# ----------------------------------------------------------------------
# USAGE: value ?-check? ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.  If the -check flag is included, the
# new value is not actually applied, but just checked for correctness.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        if {$onlycheck} {
            # someday we may add validation...
            return
        }
        set newval [lindex $args 0]
        if {[string length $newval] > 0} {
            set imh [image create photo -data $newval]
        } else {
            set imh ""
        }

        if {$_imh != ""} {
            image delete $_imh
        }
        set _imh $imh
        set _data $newval

        _redraw

        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    set bytes $_data
    set fmt [string trim [$_owner xml get $_path.convert]]
    if {"" != $fmt && "" != $_imh} {
        if {"pgm" == $fmt} { set fmt "ppm -grayscale" }
        set bytes [eval $_imh data -format $fmt]
        set bytes [Rappture::encoding::decode -as b64 $bytes]
    }
    return $bytes
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::label {} {
    set label [string trim [$_owner xml get $_path.about.label]]
    return $label
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::tooltip {} {
    set str [string trim [$_owner xml get $_path.about.description]]
    return $str
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to update the image displayed in this entry.
# If the <resize> parameter is set, then the image is resized.
# The "auto" value resizes to the current area.  The "width=XX" or
# "height=xx" form resizes to a particular size.  The "none" value
# leaves the image as-is; this is the default.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::_redraw {} {
    if {"" == $_imh} {
        # generate a big diagonal cross-hatch image
        set diag [Rappture::icon diag]
        set dw [image width $diag]
        set dh [image height $diag]
        $_resize configure -width $_thumbsize -height $_thumbsize
        for {set i 0} {$i < $_thumbsize/$dw+1} {incr i} {
            for {set j 0} {$j < $_thumbsize/$dh+1} {incr j} {
                set x [expr {$i*$dw}]
                set y [expr {$j*$dh}]
                $_resize copy $diag -to $x $y
            }
        }
        _outline $_resize black
        $itk_component(image) configure -image $_resize
        $itk_component(info) configure -text "Empty"
        return
    }

    set iw [image width $_imh]
    set ih [image height $_imh]
    $itk_component(image) configure -image "" -width $iw -height $ih

    #
    # Build a description of the image if the info is showing.
    #
    set desc ""
    if {[string length [label]] != 0} {
        # if data is base64-encoded, try to decode it
        if {![regexp {^[a-zA-Z0-9+/=]+(\n[a-zA-Z0-9+/=]+)*$} $_data]
              || [catch {Rappture::encoding::decode -as b64 $_data} bytes]} {
            # oops! not base64 -- use data directly
            set bytes $_data
        }
        set desc [Rappture::utils::datatype $bytes]
        if {[string equal $desc "Binary data"]} {
            # generic description -- we can do a little better
            set iw [image width $_imh]
            set ih [image height $_imh]
            set desc "Image, ${iw} x ${ih}"
        }
        append desc "\n[Rappture::utils::binsize [string length $_data]]"
    }
    $itk_component(info) configure -text $desc

    #
    # Put up the preview image, resizing if necessary.
    #
    set str [string trim [$_owner xml get $_path.resize]]
    if {"" == $str} {
        set str "none"
    }
    switch -glob -- $str {
        width=* - height=* {
            if {[regexp {^width=([0-9]+)$} $str match size]} {
                set w $size
                set h [expr {round($w*$ih/double($iw))}]
                $_resize configure -width $w -height $h
                $_resize blank
                blt::winop resample $_imh $_resize box
                _outline $_resize black
                $itk_component(image) configure -image $_resize \
                    -width $w -height $h
            } elseif {[regexp {^height=([0-9]+)$} $str match size]} {
                set h $size
                set w [expr {round($h*$iw/double($ih))}]
                $_resize configure -width $w -height $h
                $_resize blank
                blt::winop resample $_imh $_resize box
                _outline $_resize black
                $itk_component(image) configure -image $_resize \
                    -width $w -height $h
            } else {
                $itk_component(image) configure -image $_imh
            }
        }
        auto - none - default {
            if {[string length [label]] == 0} {
                # old mode -- big image with no label
                $itk_component(image) configure -image $_imh
            } else {
                # new mode -- thumbnail and image info
                set w $_thumbsize
                set h $_thumbsize
                $itk_component(image) configure -width $w -height $h

                if {$iw <= $_thumbsize && $ih <= $_thumbsize} {
                    $_resize configure -width $iw -height $ih
                    $_resize copy $_imh
                    _outline $_resize black
                } else {
                    # large image -- scale it down
                    if {$iw > $ih} {
                        set h [expr {round($w/double($iw)*$ih)}]
                    } else {
                        set w [expr {round($h/double($ih)*$iw)}]
                    }
                    $_resize configure -width $w -height $h
                    $_resize blank
                    blt::winop resample $_imh $_resize box
                    _outline $_resize black
                }
                $itk_component(image) configure -image $_resize
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _outline <image> <color>
#
# Used internally to outline the given <image> with a single-pixel
# line of the specified <color>.  Updates the image in place.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::_outline {im color} {
    if {"" != $im} {
        set w [image width $im]
        set h [image height $im]
        $im put $color -to 0 0 $w 1
        $im put $color -to 0 0 1 $h
        $im put $color -to 0 [expr {$h-1}] $w $h
        $im put $color -to [expr {$w-1}] 0 $w $h
    }
}

# ----------------------------------------------------------------------
# USAGE: _uploadValue -start
# USAGE: _uploadValue -assign <key> <value> <key> <value> ...
#
# Used internally to initiate an upload operation.  Prompts the
# user to upload into the image area of this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::_uploadValue {args} {
    set opt [lindex $args 0]
    switch -- $opt {
        -start {
            set tool [Rappture::Tool::resources -appname]
            set cntls [list $_path [label] [tooltip]]
            Rappture::filexfer::upload \
                $tool $cntls [itcl::code $this _uploadValue -assign]
        }
        -assign {
            array set data [lrange $args 1 end] ;# skip option
            if {[info exists data(error)]} {
                Rappture::Tooltip::cue $itk_component(image) $data(error)
            }
            if {[info exists data(data)]} {
                Rappture::Tooltip::cue hide  ;# take down note about the popup
                if {[catch {value $data(data)} err]} {
                    Rappture::Tooltip::cue $itk_component(image) "Upload failed:\n$err"
                }
            }
        }
        default {
            error "bad option \"$opt\": should be -start or -assign"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _downloadValue
#
# Used internally to initiate a download operation.  Takes the current
# value and downloads it to the user in a new browser window.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageEntry::_downloadValue {} {
    set bytes [Rappture::encoding::decode -as b64 [$_imh data -format png]]
    set mesg [Rappture::filexfer::download $bytes image.png]

    if {"" != $mesg} {
        Rappture::Tooltip::cue $itk_component(image) $mesg
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::ImageEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(image) configure -state $itk_option(-state)
}
