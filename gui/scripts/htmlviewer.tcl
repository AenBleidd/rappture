# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: htmlviewer - easy way to display HTML text
#
#  This is a thin layer on top of the Tkhtml widget.  It makes it
#  easier to load HTML text and render it with proper behaviors.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require Tkhtml
package require Img

option add *HTMLviewer.width 3i widgetDefault
option add *HTMLviewer.height 0 widgetDefault
option add *HTMLviewer.maxLines 4 widgetDefault

itcl::class Rappture::HTMLviewer {
    inherit itk::Widget

    itk_option define -foreground foreground Foreground black
    itk_option define -background background Background white
    itk_option define -linknormalcolor linkNormalColor LinkColor blue
    itk_option define -linkactivecolor linkActiveColor LinkColor black
    itk_option define -linkactivebackground linkActiveBackground LinkActiveBackground #cccccc
    itk_option define -xscrollcommand xScrollCommand XScrollCommand ""
    itk_option define -yscrollcommand yScrollCommand YScrollCommand ""
    itk_option define -width width Width 0
    itk_option define -height height Height 0
    itk_option define -maxlines maxLines MaxLines 0

    constructor {args} { # defined below }

    public method load {htmlText args}
    public method add {htmlText args}
    public method followLink {url}

    # this widget works with scrollbars
    public method xview {args} { eval $itk_component(html) xview $args }
    public method yview {args} { eval $itk_component(html) yview $args }

    protected method _config {args}
    private variable _dispatcher "" ;# dispatcher for !events

    protected method _fixHeight {args}
    private variable _linesize ""   ;# height of default font size

    protected method _mouse {option x y}
    private variable _hover ""   ;# list of nodes that mouse is hovering over

    protected method _getImage {file}
    protected method _freeImage {file}
    private common _file2icon        ;# maps file name => image handle
    private common _icon2file        ;# maps image handle => file name
    private variable _dirlist "" ;# list of directories where HTML came from
}

itk::usual HTMLviewer {
    keep -cursor
    keep -foreground -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::HTMLviewer::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !config
    $_dispatcher dispatch $this !config [itcl::code $this _config]
    $_dispatcher register !fixHeight
    $_dispatcher dispatch $this !fixHeight [itcl::code $this _fixHeight]

    itk_component add html {
        html $itk_interior.html -imagecmd [itcl::code $this _getImage]
    } {
        # no real options to work with for this widget
    }
    pack $itk_component(html) -expand yes -fill both

    bind $itk_component(html) <Motion> \
        [itcl::code $this _mouse over %x %y]
    bind $itk_component(html) <ButtonPress-1> \
        [itcl::code $this _mouse over %x %y]
    bind $itk_component(html) <ButtonRelease-1> \
        [itcl::code $this _mouse release %x %y]

    # measure the default font height
    $itk_component(html) reset
    $itk_component(html) parse -final "Testing"
    set node [$itk_component(html) node]
    foreach {x0 y0 x1 y1} [$itk_component(html) bbox $node] break
    set _linesize [expr {$y1-$y0}]
    $itk_component(html) reset

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: load <htmlText> ?-in <fileName>?
#
# Clients use this to clear the contents and load a new string of
# <htmlText>.  If the text is empty, this has the effect of clearing
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::HTMLviewer::load {htmlText args} {
    Rappture::getopts args params {
        value -in ""
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"load text ?-in name?\""
    }

    $itk_component(html) reset
    set _hover ""
    set _dirlist ""

    $itk_component(html) parse $htmlText

    if {"" != $params(-in) && [file exists $params(-in)]} {
        if {[file isdirectory $params(-in)]} {
            lappend _dirlist $params(-in)
        } else {
            lappend _dirlist [file dirname $params(-in)]
        }
    }
    $_dispatcher event -now !config
}

# ----------------------------------------------------------------------
# USAGE: add <htmlText> ?-in <fileName>?
#
# Clients use this to add the <htmlText> to the bottom of the contents
# being displayed in the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::HTMLviewer::add {htmlText args} {
    Rappture::getopts args params {
        value -in ""
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"add text ?-in name?\""
    }

    $itk_component(html) parse $htmlText

    if {"" != $params(-in) && [file exists $params(-in)]} {
        if {[file isdirectory $params(-in)]} {
            lappend _dirlist $params(-in)
        } else {
            lappend _dirlist [file dirname $params(-in)]
        }
    }
    $_dispatcher event -now !config
}

# ----------------------------------------------------------------------
# USAGE: followLink <url>
#
# Invoked automatically whenever the user clicks on a link within
# an HTML page.  Tries to follow the <url> by invoking "exportfile"
# to pop up further information.  If the <url> starts with http://
# or https://, then it is used directly.  Otherwise, it is treated
# as a relative file path and resolved with respect to the -in
# options passed into load/add.
# ----------------------------------------------------------------------
itcl::body Rappture::HTMLviewer::followLink {url} {
    if {[regexp -nocase {^https?://} $url]} {
        foreach prog {
                clientaction
                /apps/bin/clientaction
                /apps/xvnc/bin/clientaction
                /usr/lib/mw/bin/clientaction
                ""
        } {
            if {"" != [auto_execok $prog]} {
                break
            }
        }
        if {"" != $prog} {
            exec $prog url $url &
        } else {
            bell
        }
        return
    }

    # must be a file -- use exportfile
    set url [string trimleft $url /]
    set path ""
    foreach dir $_dirlist {
        if {[file readable [file join $dir $url]]} {
            set path [file join $dir $url]
            break
        }
    }

    foreach prog {exportfile /apps/bin/exportfile ""} {
        if {"" != [auto_execok $prog]} {
            break
        }
    }
    if {"" != $path && "" != $prog} {
        exec $prog --format html $path &
    } else {
        bell
    }
}

# ----------------------------------------------------------------------
# USAGE: _mouse over <x> <y>
# USAGE: _mouse release <x> <y>
#
# Invoked automatically as the mouse pointer moves around or otherwise
# interacts with this widget.  When the mouse is over a link, the link
# becomes annotated with the "hover" style, so it can be highlighted.
# Clicking and releasing on the same link invokes the action associated
# with the link.
# ----------------------------------------------------------------------
itcl::body Rappture::HTMLviewer::_mouse {option x y} {
    switch -- $option {
        over {
            # get a list of nodes with tags we care about
            set nlist ""
            foreach node [$itk_component(html) node $x $y] {
                while {"" != $node} {
                    if {[$node tag] == "a"} {
                        lappend nlist $node
                        break
                    }
                    set node [$node parent]
                }
            }

            # over something new? then tag it with "hover"
            if {$nlist != $_hover} {
                foreach node $_hover {
                    catch {$node dynamic clear hover}
                }
                set _hover $nlist
                foreach node $_hover {
                    catch {$node dynamic set hover}
                }
            }
        }
        release {
            set prev $_hover
            _mouse over $x $y

            # mouse release on same node as mouse click? then follow link
            if {$prev == $_hover} {
                foreach node $_hover {
                    if {[$node tag] == "a"} {
                        followLink [$node attr -default {} href]
                    }
                }
            }
        }
        default {
            error "bad option \"$option\": should be over or release"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _config <arg> <arg>...
#
# Invoked automatically whenever the style-related configuration
# options change for this widget.  Changes the main style sheet to
# configure the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::HTMLviewer::_config {args} {
    component html style -id "author" "
      body {
        background: $itk_option(-background);
        color: $itk_option(-foreground);
        font: 12px helvetica,arial;
      }
      a {
        color: $itk_option(-linknormalcolor);
        text-decoration: underline;
      }
      a:hover {
        color: $itk_option(-linkactivecolor);
        background: $itk_option(-linkactivebackground);
      }
    "
}

# ----------------------------------------------------------------------
# USAGE: _fixHeight <arg> <arg>...
#
# Invoked automatically whenever the height-related configuration
# options change for this widget.  If -height is set to 0, then this
# routine figures out a good height for the widget based on -maxlines.
# ----------------------------------------------------------------------
itcl::body Rappture::HTMLviewer::_fixHeight {args} {
    set ht [winfo pixels $itk_component(html) $itk_option(-height)]
    if {$ht <= 0} {
        # figure out a good size automatically
        set realht [winfo pixels $itk_component(html) 1i]
        set node [$itk_component(html) node]
        if {"" != $node} {
            set bbox [$itk_component(html) bbox $node]
            if { [llength $bbox] == 4 } {
                set realht [expr {[lindex $bbox 3]-[lindex $bbox 1]}]
            }
        }
        if {$itk_option(-maxlines) > 0} {
            set ht [expr {$itk_option(-maxlines)*$_linesize}]
            if {$realht < $ht} {
                set ht $realht
            }
        } else {
            set ht $realht
        }
    }
    $itk_component(html) configure -height $ht
}

# ----------------------------------------------------------------------
# USAGE: _getImage <fileName>
#
# Used internally to convert a <fileName> to its corresponding image
# handle.  If the <fileName> is relative, then it is loaded with
# respect to the paths given by the -in option for the load/add
# methods.  Returns an image handle for the image within the file,
# or the broken image icon if anything goes wrong.
# ----------------------------------------------------------------------
itcl::body Rappture::HTMLviewer::_getImage {fileName} {
    if {[info exists _file2icon($fileName)]} {
        set imh $_file2icon($fileName)
        return [list $imh [itcl::code $this _freeImage]]
    }

    set searchlist $fileName
    if {[file pathtype $fileName] != "absolute"} {
        foreach dir $_dirlist {
            lappend searchlist [file join $dir $fileName]
        }
    }

    foreach name $searchlist {
        if {[catch {image create photo -file $name} imh] == 0} {
            set _file2icon($fileName) $imh
            set _icon2file($imh) $fileName
            return [list $imh [itcl::code $this _freeImage]]
        }
    }
    puts stderr "Problem in your html: image \"$fileName\" does not exist"
    # The htmlwidget assumes it owns the image and will delete it.
    # Always create a copy of the image.
    set img [Rappture::icon exclaim]
    set file [$img cget -file]
    set img [image create photo -file $file]
    return $img
}

itcl::body Rappture::HTMLviewer::_freeImage { imh } {
    if {[info exists _icon2file($imh)]} {
        image delete $imh
        set fileName $_icon2file($imh)
        unset _icon2file($imh)
        unset _file2icon($fileName)
    }
}

# ----------------------------------------------------------------------
# OPTION: -background
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::background {
    $_dispatcher event -idle !config
}

# ----------------------------------------------------------------------
# OPTION: -foreground
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::foreground {
    $_dispatcher event -idle !config
}

# ----------------------------------------------------------------------
# OPTION: -linknormalcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::linknormalcolor {
    $_dispatcher event -idle !config
}

# ----------------------------------------------------------------------
# OPTION: -linkactivecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::linkactivecolor {
    $_dispatcher event -idle !config
}

# ----------------------------------------------------------------------
# OPTION: -linkactivebackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::linkactivebackground {
    $_dispatcher event -idle !config
}

# ----------------------------------------------------------------------
# OPTION: -xscrollcommand
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::xscrollcommand {
    $itk_component(html) configure -xscrollcommand $itk_option(-xscrollcommand)
}

# ----------------------------------------------------------------------
# OPTION: -yscrollcommand
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::yscrollcommand {
    $itk_component(html) configure -yscrollcommand $itk_option(-yscrollcommand)
}

# ----------------------------------------------------------------------
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::width {
    $itk_component(html) configure -width $itk_option(-width)
}

# ----------------------------------------------------------------------
# OPTION: -maxlines
# Sets the maximum number of lines for widgets with "-height 0"
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::maxlines {
    $_dispatcher event -idle !fixHeight
}

# ----------------------------------------------------------------------
# OPTION: -height
# Set to absolute size ("3i") or 0 for default size based on -maxlines.
# ----------------------------------------------------------------------
itcl::configbody Rappture::HTMLviewer::height {
    $_dispatcher event -idle !fixHeight
}
