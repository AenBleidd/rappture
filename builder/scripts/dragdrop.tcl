# ----------------------------------------------------------------------
#  COMPONENT: dragdrop - drag-n-drop support for widgets
#
#  Derived classes inherit the basic drag-n-drop support from this
#  widget class.  Widgets can register themselves as drag-n-drop
#  sources and also as targets.
#
#  Derived classes should overload the following methods:
#
#    dd_get_source .... examines click point and returns drag data
#
#    dd_scan_target ... scans target at drop point and returns 1 if
#                         data is accepted, and 0 if rejected
#
#    dd_finalize ...... handles final drag-and-drop operation,
#                         transferring data from the start widget
#                         to the final target.  Called with "-op start"
#                         on the starting widget, and "-op end" on the
#                         target widget.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

itcl::class Rappture::Dragdrop {
    public method dragdrop {option args}

    private method _get_source {widget x y}
    private method _do_scan {x y {option indicate}}
    private method _do_drop {x y}

    # derived classes override these methods
    protected method dd_get_source {widget x y}
    protected method dd_scan_target {x y data}
    protected method dd_finalize {option args}

    private variable _enabled 1      ;# set to 0 to disable drag-n-drop
    private variable _dragwin ""     ;# widget controlling drag-n-drop cursor
    private variable _dragowner ""   ;# mega-widget that initiated drag-n-drop
    private variable _dragx ""       ;# drag-n-drop started at this x
    private variable _dragy ""       ;# drag-n-drop started at this y
    private variable _dragdata ""    ;# description of drag-n-drop source data
    private variable _lastcursor ""  ;# cursor before start of drag-n-drop
    private variable _lasttarget ""  ;# last target found by _do_scan

    set dir [file dirname [info script]]
    private common _dcursor "@[file join $dir images drag.xbm] [file join $dir images dragm.xbm] black white"
}

# ----------------------------------------------------------------------
# USAGE: dragdrop protocol
# USAGE: dragdrop enabled ?<boolean>?
# USAGE: dragdrop source <widget>
# USAGE: dragdrop indicate -from <w> -to <w> -x <x> -y <y> -data <data>
# USAGE: dragdrop finalize -op <dir> -from <w> -to <w> -x <x> -y <y> -data <data>
# USAGE: dragdrop cancel
#
# Derived classes use this to register component widgets as drag-n-drop
# sources and targets.
# ----------------------------------------------------------------------
itcl::body Rappture::Dragdrop::dragdrop {option args} {
    switch -- $option {
        protocol {
            return "1.0"
        }
        enabled {
            if {[llength $args] == 0} {
                return $_enabled
            } elseif {[llength $args] != 1} {
                error "wrong # args: should be \"dragdrop enabled ?boolean?\""
            }
            set state [lindex $args 0]
            set _enabled [expr {($state) ? 1 : 0}]
            return $_enabled
        }
        source {
            set widget [lindex $args 0]
            bind $widget <ButtonPress> [itcl::code $this _get_source %W %X %Y]
            bind $widget <B1-Motion> [itcl::code $this _do_scan %X %Y]
            bind $widget <ButtonRelease> [itcl::code $this _do_drop %X %Y]
        }
        indicate {
            array set params $args
            return [dd_scan_target $params(-x) $params(-y) $params(-data)]
        }
        finalize {
            return [eval dd_finalize drop $args]
        }
        cancel {
            dd_finalize cancel
        }
        default {
            error "bad option \"$option\": should be protocol, source, indicate, finalize, cancel"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _get_source <widget> <x> <y>
#
# Invoked when the user clicks on a widget to start a drag-n-drop
# operation.  Saves the <x>,<y> coordinate, and waits for a _do_scan
# call.  If the user drags the mouse a short distance away from the
# click point, then the drag-n-drop operation begins.
# ----------------------------------------------------------------------
itcl::body Rappture::Dragdrop::_get_source {widget x y} {
    set _dragowner ""
    set _lasttarget ""

    set winx [expr {$x-[winfo rootx $widget]}]
    set winy [expr {$y-[winfo rooty $widget]}]
    set d [dd_get_source $widget $winx $winy]

    if {"" != $d && $_enabled} {
        set _dragwin $widget
        set w $widget
        while {$w != "."} {
            if {[catch {$w dragdrop protocol}] == 0} {
                set _dragowner $w
                break
            }
            set w [winfo parent $w]
        }
        set _dragx $x
        set _dragy $y
        set _dragdata $d
        set _lastcursor [$widget cget -cursor]
        $widget configure -cursor $_dcursor
    } else {
        set _dragwin ""
        set _dragowner ""
        set _dragx ""
        set _dragy ""
        set _dragdata ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _do_scan <x> <y> ?<operation>?
#
# Invoked as the user moves the mouse pointer during a drag-n-drop
# operation.  Scans for a widget at the given <x>,<y> root window
# coordinate that will accept the drag data.
# ----------------------------------------------------------------------
itcl::body Rappture::Dragdrop::_do_scan {x y {option indicate}} {
    if {"" != $_dragowner} {
        set dx [expr {abs($x-$_dragx)}]
        set dy [expr {abs($y-$_dragy)}]

        # moved significantly away from click?
        if {$dx > 3 || $dy > 3} {
            set win [winfo containing $x $y]
            if {"" != $win} {
                set found 0
                set wx [expr {$x-[winfo rootx $win]}]
                set wy [expr {$y-[winfo rooty $win]}]
                while {"." != $win} {
                    # find the containing win that handles dragdrop and
                    # accepts the given data type
                    if {[catch {$win dragdrop $option -op end -from $_dragowner -to $win -x $wx -y $wy -data $_dragdata} result] == 0 && [string is boolean -strict $result] && $result} {
                        if {"" != $_lasttarget && $_lasttarget != $win} {
                            # if we had a different target, cancel it
                            catch {$_lasttarget dragdrop cancel}
                        }
                        set _lasttarget $win
                        set found 1

                        # if this is a finalize operation, then send final
                        # message to starting side
                        if {$option == "finalize"} {
                            catch {$_dragowner dragdrop finalize -op start -from $_dragowner -to $win -x $wx -y $wy -data $_dragdata}
                        }
                        break
                    }
                    set win [winfo parent $win]
                }

                if {!$found && "" != $_lasttarget} {
                    # no drop target at this point? cancel any previous target
                    catch {$_lasttarget dragdrop cancel}
                    set _lasttarget ""
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _do_drop <x> <y>
#
# Invoked when the user releases the mouse pointer during a drag-n-drop
# operation.  Tries to send the drag data to the widget at the current
# <x>,<y> root window coordinate to complete the operation.
# ----------------------------------------------------------------------
itcl::body Rappture::Dragdrop::_do_drop {x y} {
    if {"" != $_dragowner} {
        _do_scan $x $y indicate
        _do_scan $x $y finalize
        $_dragwin configure -cursor $_lastcursor
    }
}

# ----------------------------------------------------------------------
# USAGE: dd_get_source <widget> <x> <y>
#
# Derived classes override this method to look at the given <widget>
# and <x>,<y> coordinate and figure out what data value the source
# is exporting.  Returns a string that identifies the type of the
# data.  This string is passed along to targets via the dd_scan_target
# method.  If the target may check the source type and reject the data.
# ----------------------------------------------------------------------
itcl::body Rappture::Dragdrop::dd_get_source {widget x y} {
    return "?"
}

# ----------------------------------------------------------------------
# USAGE: dd_scan_target <x> <y> <data>
#
# Derived classes override this method to look within themselves at
# the given <x>,<y> coordinate and decide whether or not it can
# accept the <data> type at that location.  Returns 1 if the target
# accepts the data, and 0 otherwise.
# ----------------------------------------------------------------------
itcl::body Rappture::Dragdrop::dd_scan_target {x y data} {
    return 0
}

# ----------------------------------------------------------------------
# USAGE: dd_finalize drop -op start|end -from <w> -to <w> \
#                           -x <x> -y <y> -data <data>
# USAGE: dd_finalize cancel
#
# Derived classes override this method to handle the end of a drag
# and drop operation.  The operation can be completed with a successful
# drop of data, or cancelled.  Returns 1 if drop was successful, and
# 0 otherwise.
# ----------------------------------------------------------------------
itcl::body Rappture::Dragdrop::dd_finalize {option args} {
    return 0
}
