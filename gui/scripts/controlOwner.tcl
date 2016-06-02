# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: owner - manages Rappture controls
#
#  This object represents an entity managing Rappture controls.
#  It is typically a Tool, a DeviceEditor, or some other large entity
#  that manages a Rappture XML tree.  All controlling widgets are
#  registered with an owner, and the owner propagates notifications
#  out to clients who have an interest in a particular control.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

itcl::class Rappture::ControlOwner {
    constructor {owner} { # defined below }

    public method xml {args}

    public method load {newobj}
    public method widgetfor {path args}
    public method valuefor {path args}
    public method dependenciesfor {path args}
    public method ownerfor {path {skip ""}}
    public method changed {path}
    public method regularize {path}
    public method notify {option owner args}
    public method sync {}
    public method tool {}

    protected method _slave {option args}

    protected variable _owner ""     ;# ControlOwner containing this one
    protected variable _path ""      ;# paths within are relative to this
    protected variable _slaves ""    ;# this owner has these slaves
    protected variable _xmlobj ""    ;# Rappture XML description
    private variable _path2widget    ;# maps path => widget on this page
    private variable _path2controls  ;# maps path => panel containing widget
    private variable _owner2paths    ;# for notify: maps owner => interests
    private variable _type2curpath   ;# maps type(path) => path's current value
    private variable _callbacks      ;# for notify: maps owner/path => callback
    private variable _dependencies   ;# maps path => other paths dep on this
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::constructor {owner} {
    if {"" != $owner} {
        set parts [split $owner @]
        set _owner [lindex $parts 0]
        set _path [lindex $parts 1]
        $_owner _slave add $this
    }

    # we are adding this so notes can be used
    # in coordination with loaders inside the load function
    array set _type2curpath {
        drawing all
        choice current
        boolean current
        image current
        integer current
        loader current
        multichoice current
        note contents
        number current
        periodicelement current
        string current
    }
}

# ----------------------------------------------------------------------
# USAGE: xml <subcommand> ?<arg> <arg> ...?
# USAGE: xml object
#
# Used by clients to manipulate the underlying XML data for this
# tool.  The <subcommand> can be any operation supported by a
# Rappture::library object.  Clients can also request the XML object
# directly by using the "object" subcommand.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::xml {args} {
    if {"object" == $args} {
        return $_xmlobj
    }
    return [eval $_xmlobj $args]
}

# ----------------------------------------------------------------------
# USAGE: widgetfor <path> ?<widget>?
#
# Used by embedded widgets such as a Controls panel to register the
# various controls associated with this page.  That way, this
# ControlOwner knows what widgets to look at when syncing itself
# to the underlying XML data.
#
# There can only be one widget per path, since the control owner will
# later query the widgets for current values.  If there is already an
# existing widget registered for the <path>, then it will be deleted
# and the new <widget> will take its place.  If the caller doesn't
# want to replace an existing widget, it should check before calling
# this method and make sure that the return value is "".
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::widgetfor {path args} {
    # if this is a query operation, then look for the path
    if {[llength $args] == 0} {
        set owner [ownerfor $path]
        if {$owner ne $this && $owner ne ""} {
            return [$owner widgetfor $path]
        }
        if {[info exists _path2widget($path)]} {
            return $_path2widget($path)
        }
        return ""
    }

    # otherwise, associate the path with the given widget
    set widget [lindex $args 0]
    if {$widget ne ""} {
        # is there already a widget registered for this path?
        if {[info exists _path2widget($path)]} {
            # delete old widget and replace
            set panel $_path2controls($path)
            $panel delete $path
            set _path2controls($path) ""
        }

        # register the new widget for the path
        set _path2widget($path) $widget

        # look up the containing panel and store it too
        set w [winfo parent $widget]
        while {$w ne ""} {
            if {[string match *Controls [winfo class $w]]} {
                set _path2controls($path) $w
                break
            }
            set w [winfo parent $w]
        }
    } else {
        # empty name => forget about this widget
        catch {unset _path2widget($path)}
        catch {unset _path2controls($path)}
    }
}

# ----------------------------------------------------------------------
# USAGE: valuefor <path> ?<newValue>?
#
# Used by embedded widgets such as a Loader to query or set the
# value of another control.  With no extra args, it returns the
# value of the widget at the <path> in the XML.  Otherwise, it
# sets the value of the widget to <newValue>.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::valuefor {path args} {
    set owner [ownerfor $path]

    # if this is a query operation, then look for the path
    if {[llength $args] == 0} {
        if {$owner != $this && $owner != ""} {
            return [$owner valuefor $path]
        }
        if {[info exists _path2widget($path)]} {
            return [$_path2widget($path) value]
        }
        # can't find the path? try removing the prefix for this owner
        set plen [string length $_path]
        if {[string equal -length $plen $_path $path]} {
            set relpath [string range $path [expr {$plen+1}] end]
            if {[info exists _path2widget($relpath)]} {
                return [$_path2widget($relpath) value]
            }
        }
        return ""
    }

    # otherwise, set the value
    if {$owner != $this && $owner != ""} {
        return [eval $owner valuefor $path $args]
    }
    if {[llength $args] > 1} {
        error "wrong # args: should be \"valuefor path ?newValue?\""
    }

    if {[info exists _path2widget($path)]} {
        $_path2widget($path) value [lindex $args 0]
    } else {
        error "bad path \"$path\": should be one of [join [lsort [array names _path2widget]] {, }]"
    }
}

# ----------------------------------------------------------------------
# USAGE: dependenciesfor <path> ?<path>...?
#
# Used by embedded widgets such as a Controls panel to register the
# various controls that are dependent on another one.  If only one
# path is specified, then this method returns all known dependencies
# for the specified <path>.  Otherwise, the additional <path>'s are
# noted as being dependent on the first <path>.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::dependenciesfor {path args} {
    if {"" != $_owner} {
        #
        # Keep all dependencies at the highest level.
        # That way, a structure can come and go, but the
        # dependencies remain fixed in the topmost tool.
        #
        set plen [string length $_path]
        if {"" != $_path && ![string equal -length $plen $_path $path]} {
            set path $_path.$path
        }
        return [eval $_owner dependenciesfor $path $args]
    }

    # if this is a query operation, then look for the path
    if {[llength $args] == 0} {
        if {[info exists _dependencies($path)]} {
            return $_dependencies($path)
        }
        return ""
    }

    # add new dependencies
    if {![info exists _dependencies($path)]} {
        set _dependencies($path) ""
    }
    foreach dpath $args {
        set i [lsearch -exact $_dependencies($path) $dpath]
        if {$i < 0} {
            lappend _dependencies($path) $dpath
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: ownerfor <path> ?<skip>?
#
# Returns the ControlOwner that directly controls the specified <path>.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::ownerfor {path {skip ""}} {
    if {[info exists _path2widget($path)]} {
        return $this
    }

    # can't find the path? try removing the prefix for this owner
    set plen [string length $_path]
    if {[string equal -length $plen $_path $path]} {
        set relpath [string range $path [expr {$plen+1}] end]
        if {[info exists _path2widget($relpath)]} {
            return $this
        }
    }

    # couldn't find this path?  then check all subordinates
    foreach slave $_slaves {
        if {$slave == $skip} {
            continue  ;# skip this slave if it's already been searched
        }
        set rval [$slave ownerfor $path $this]
        if {"" != $rval} {
            return $rval
        }
    }

    # check the owner as a last resort
    if {"" != $_owner && $_owner != $skip} {
        set rval [$_owner ownerfor $path $this]
        if {"" != $rval} {
            return $rval
        }
    }

    return ""
}

# ----------------------------------------------------------------------
# USAGE: load <xmlobj>
#
# Loads the contents of a Rappture <xmlobj> into the controls
# associated with this tool.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::load {newobj} {
    if {![Rappture::library isvalid $newobj]} {
        error "\"$newobj\" is not a Rappture::library"
    }
    foreach path [array names _path2widget] {
        # the following elements do not accept "current" tags, skip them
        set type [[tool] xml element -as type $path]
        if {[lsearch {group separator control} $type] >= 0} {
            continue
        }

        set type [[tool] xml element -as type $path]
        if {[info exists _type2curpath($type)]} {
            if { $_type2curpath($type) == "all" } {
                set currentpath $path
            } else {
                set currentpath $path.$_type2curpath($type)
            }
        } else {
            # default incase i forgot an input type in _type2curpath
            set currentpath $path.current
        }

        # copy new value to the XML tree
        # also copy to the widget associated with the tree
        #
        # we only copy the values if they existed in newobj
        # so we don't overwrite values that were set in previous loads.
        # this is needed for when the users specify copy.from and copy.to
        # in a loader. in this case, _path2widget holds a list of all
        # widgets. if there are two loaders loading two different widgets,
        # and each loader uses the copy from/to functionality,
        # the second load could wipe out the values set in the first load
        # because on the second load, the copied paths from the first load no
        # longer exist in newobj and blanks are copied to the paths
        # in [tool] xml set by the first loader. the solution is to check
        # newobj and see if the path exists. if the path exists, then we copy
        # it over to [tool] xml, otherwise we ignore it.
        if {"" != [$newobj element -as type $currentpath]} {
            [tool] xml copy $currentpath from $newobj $currentpath
            set val [$newobj get $currentpath]
            if {[string length $val] > 0
                  || [llength [$newobj children $currentpath]] == 0} {
                $_path2widget($path) value $val
            } else {
                set obj [$newobj element -as object $currentpath]
                $_path2widget($path) value $obj
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: changed <path>
#
# Invoked automatically by the various widgets associated with this
# tool whenever their value changes.  Sends notifications to any
# client that has registered an interest via "notify add".
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::changed {path} {
    if {"" != $_owner} {
        set plen [string length $_path]
        if {"" != $_path && ![string equal -length $plen $_path $path]} {
            set path $_path.$path
        }
        $_owner changed $path
    } else {
        # send out any callback notifications
        foreach owner [array names _owner2paths] {
            foreach pattern $_owner2paths($owner) {
                if {[string match $pattern $path]} {
                    uplevel #0 $_callbacks($owner/$pattern)
                    break
                }
            }
        }

        # find the control panel for each dependency, and tell it
        # to update its layout.
        foreach cpath [dependenciesfor $path] {
            set wv [widgetfor $cpath]
            while {"" != $wv} {
                set wv [winfo parent $wv]
                if {[winfo class $wv] == "Controls"} {
                    $wv refresh
                    break
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: regularize <path>
#
# Clients use this to get a full, regularized path for the specified
# <path>, which may be relative to the current owner.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::regularize {path} {
    set owner [ownerfor $path]
    if {$owner != $this && $owner != ""} {
        return [$owner regularize $path]
    }
    set rpath ""
    if {"" != $_xmlobj} {
        set rpath [$_xmlobj element -as path $path]

        # can't find the path? try removing the prefix for this owner
        if {"" == $rpath} {
            set plen [string length $_path]
            if {[string equal -length $plen $_path $path]} {
                set relpath [string range $path [expr {$plen+2}] end]
                set rpath [$_xmlobj element -as path $relpath]
            }
        }

        if {"" != $rpath && "" != $_path} {
            # return a full name for the path
            set rpath "$_path.$rpath"
        }
    }
    return $rpath
}

# ----------------------------------------------------------------------
# USAGE: notify add <owner> <path> <callback>
# USAGE: notify info ?<owner>? ?<path>?
# USAGE: notify remove <owner> ?<path> ...?
#
# Clients use this to request notifications about changes to a
# particular <path> for a control under this tool.  Whenever the
# value associated with <path> changes, the client identified by
# <owner> is sent a message by invoking its <callback> routine.
#
# Notifications can be silenced by calling the "notify remove"
# function.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::notify {option args} {
    switch -- $option {
        add {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"notify add owner path callback\""
            }
            set owner [lindex $args 0]
            set path [lindex $args 1]
            set cb [lindex $args 2]

            if {[info exists _owner2paths($owner)]} {
                set plist $_owner2paths($owner)
            } else {
                set plist ""
            }

            set i [lsearch -exact $plist $path]
            if {$i < 0} { lappend _owner2paths($owner) $path }
            set _callbacks($owner/$path) $cb
        }
        info {
            if {[llength $args] == 0} {
                # no args? then return all owners
                return [array names _owner2paths]
            } else {
                set owner [lindex $args 0]
                if {[info exists _owner2paths($owner)]} {
                    set plist $_owner2paths($owner)
                } else {
                    set plist ""
                }
                if {[llength $args] == 1} {
                    # 1 arg? then return paths for this owner
                    return $plist
                } elseif {[llength $args] == 2} {
                    # 2 args? then return callback for this path
                    set path [lindex $args 1]
                    if {[info exists _callbacks($owner/$path)]} {
                        return $_callbacks($owner/$path)
                    }
                    return ""
                } else {
                    error "wrong # args: should be \"notify info ?owner? ?path?\""
                }
            }
        }
        remove {
            if {[llength $args] < 1} {
                error "wrong # args: should be \"notify remove owner ?path ...?\""
            }
            set owner [lindex $args 0]

            if {[llength $args] == 1} {
                # no args? then delete all paths for this owner
                if {[info exists _owner2paths($owner)]} {
                    set plist $_owner2paths($owner)
                } else {
                    set plist ""
                }
            } else {
                set plist [lrange $args 1 end]
            }

            # forget about the callback for each path
            foreach path $plist {
                catch {unset _callbacks($owner/$path)}

                if {[info exists _owner2paths($owner)]} {
                    set i [lsearch -exact $_owner2paths($owner) $path]
                    if {$i >= 0} {
                        set _owner2paths($owner) \
                            [lreplace $_owner2paths($owner) $i $i]
                    }
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: sync
#
# Used by descendents such as a Controls panel to register the
# various controls associated with this page.  That way, this
# ControlOwner knows what widgets to look at when syncing itself
# to the underlying XML data.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::sync {} {
    # sync all of the widgets under control of this owner
    if {"" != $_xmlobj} {
        foreach path [lsort [array names _path2widget]] {
            set type [$_xmlobj element -as type $path]
            if {[lsearch {group separator control note} $type] >= 0} {
                continue
            }
            $_xmlobj put $path.current [$_path2widget($path) value]
        }
    }

    # sync all subordinate slaves as well
    foreach slave $_slaves {
        $slave sync
    }
}

# ----------------------------------------------------------------------
# USAGE: tool
#
# Clients use this to figure out which tool is associated with
# this object.  If there is no parent ControlOwner, then this
# must be the top-level tool.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::tool {} {
    if {"" != $_owner} {
        return [$_owner tool]
    }
    return $this
}

# ----------------------------------------------------------------------
# USAGE: _slave add <newobj>...
#
# Used internally to register the parent-child relationship whenever
# one ControlOwner is registered to another.  When the parent syncs,
# it causes all of its children to sync.  When a name is being
# resolved, it is resolved locally first, then up to the parent for
# further resolution.
# ----------------------------------------------------------------------
itcl::body Rappture::ControlOwner::_slave {option args} {
    switch -- $option {
        add {
            eval lappend _slaves $args
        }
        default {
            error "bad option \"$option\": should be add"
        }
    }
}
