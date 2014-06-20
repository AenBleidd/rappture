# ----------------------------------------------------------------------
#  USER INTERFACE BUILDER
#
#  This program allows the user to create a tool.xml file, or reload
#  and edit an existing tool.xml.  It also produces some skeleton
#  code for the main program that queries inputs and saves outputs.
#
#  RUN AS FOLLOWS:
#    wish main.tcl ?-tool <toolfile>?
#
#  If the <toolfile> is not specified, it defaults to "tool.xml" in
#  the current working directory.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

# take the main window down for now, so we can avoid a flash on the screen
wm withdraw .

package require Itcl
package require Rappture
package require RapptureGUI
package require Img

option add *Tooltip.background white
option add *Editor.background white
option add *Gauge.textBackground white
option add *TemperatureGauge.textBackground white
option add *Switch.textBackground white
option add *Progress.barColor #ffffcc
option add *Balloon.titleBackground #6666cc
option add *Balloon.titleForeground white
option add *Balloon*Label.font -*-helvetica-medium-r-normal-*-12-*
option add *Balloon*Radiobutton.font -*-helvetica-medium-r-normal-*-12-*
option add *Balloon*Checkbutton.font -*-helvetica-medium-r-normal-*-12-*
option add *BugReport*banner*foreground white
option add *BugReport*banner*background #a9a9a9
option add *BugReport*banner*highlightBackground #a9a9a9
option add *BugReport*banner*font -*-helvetica-bold-r-normal-*-18-*
option add *Filmstrip.background #aaaaaa
option add *Hierlist.selectBackground #9999ff
option add *errorHighlightColor #9999ff
option add *previewButtonBackground #4a758c
option add *previewButtonActiveBackground #a3c3cc
option add *previewButtonForeground white
option add *previewButtonIcon arrow-up-white
option add *build*cntls*background #cccccc
option add *build*cntls*highlightBackground #cccccc
option add *build*cntls*font {helvetica -12}
option add *build*cntls*Button.borderWidth 1
option add *build*cntls*Button.relief flat
option add *build*cntls*Button.overRelief raised
option add *build*cntls*Button.padX 2
option add *build*cntls*Editor.background white
option add *build*editelem*background #cccccc
option add *build*editelem*font {helvetica -12}
option add *build*editelem*title.font {helvetica -12 italic}
option add *build*editelem*vopt.font {helvetica -12 italic}
option add *options*Entry.background white
option add *options*Listbox.background white
option add *options*Text.background white
option add *options*Text.font {Helvetica -12}
option add *options*Button.font {Helvetica -10}
option add *options*Button.padX 2
option add *options*Button.padY 2
option add *options.errs*Background black
option add *options.errs*highlightBackground black
option add *options.errs*Foreground white
option add *options.errs*Button.relief flat
option add *options.errs*Button.padX 1
option add *options.errs*Button.padY 1
option add *saveas*toolv*Label.font {Helvetica -12}
option add *saveas*toolv*Button.font {Helvetica -12}
option add *saveas*toolv*Button.borderWidth 1
option add *saveas*toolv*Button.padX 2
option add *saveas*toolv*Button.padY 2
option add *saveas*progv*Label.font {Helvetica -12}
option add *saveas*progv*Button.font {Helvetica -12}
option add *saveas*progv*Button.borderWidth 1
option add *saveas*progv*Button.padX 2
option add *saveas*progv*Button.padY 2

switch $tcl_platform(platform) {
    unix {
        event add <<PopupMenu>> <ButtonPress-3>

        # make Ctrl-V binding work:
        # not strict Motif, and remove conflicting binding in Tk library
        set tk_strictMotif 0
        bind Text <Control-v> ""
    }
    windows {
        event add <<PopupMenu>> <ButtonPress-3>
    }
    macintosh {
        event add <<PopupMenu>> <Control-ButtonPress-1>
    }
}

wm protocol . WM_DELETE_WINDOW main_exit
wm title . "Rappture Builder"

# install a better bug handler
Rappture::bugreport::install
# fix the "grab" command to support a stack of grab windows
Rappture::grab::init

#
# Process command line args to get the names of files to load...
#
Rappture::getopts argv params {
    value -tool ""
}

# load type and object definitions
set dir [file dirname [info script]]
set auto_path [linsert $auto_path 0 $dir]

Rappture::icon foo  ;# force loading of this module
lappend Rappture::icon::iconpath [file join $dir images]

# automatically load all Rappture object types
if {[catch {Rappture::objects::init} err]} {
    puts stderr "Error loading object definitions:\n$err"
    exit 1
}
if {[catch {Rappture::objects::load [file join $dir tool.rp]} err]} {
    puts stderr "Error loading tool object definition:\n$err"
    exit 1
}

# load all languages recognized by the builder
RapptureBuilder::templates::init

# ----------------------------------------------------------------------
#  HACK ALERT!  Make it so the Analyzer can't possibly enable its
#    simulate button in "preview" mode, or else the user might
#    actually launch a simulation.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_simState {state args} {
    if {"" != $itk_option(-simcontrolbackground)} {
        set simcbg $itk_option(-simcontrolbackground)
    } else {
        set simcbg $itk_option(-background)
    }
    $itk_interior.simol configure \
        -background $itk_option(-simcontroloutline)
    configure -simcontrolcolor $simcbg

    $itk_component(simulate) configure -state disabled
}

# ----------------------------------------------------------------------
# USAGE: main_palette_source <type>
#
# Returns drag-n-drop data for the object <type>.  This data can be
# dragged between the filmstrip object palettes and the hierlist of
# nodes for the current tool.
# ----------------------------------------------------------------------
set ValueId 0
proc main_palette_source {type} {
    global ValueId
    if {[catch {Rappture::objects::get $type -attributes} attrs]} {
        return ""
    }
    set term [Rappture::objects::get $type -terminal]
    set vname "value[incr ValueId]"
    set type [string totitle $type]
    return [list node: id $vname type $type terminal $term \
        attributes [list label $type]]
}

# ----------------------------------------------------------------------
# USAGE: main_open ?-new|-file|filename?
#
# Handles the Open... operation, which opens a tool.xml file and
# loads the contents.  If there are any pending changes, the user
# is prompted to save the changes before continuing with the open.
# ----------------------------------------------------------------------
proc main_open {{what "-new"}} {
    global ToolXml LastToolXmlFile LastToolXmlLoaded ValueId

    if {![main_options_save -clear]} {
        return
    }
    set win [.func.build.options.panes pane 0]
    $win.scrl.skel select none

    main_generate_xml

    if {[string length $LastToolXmlLoaded] > 0 && [$ToolXml xml] ne $LastToolXmlLoaded} {
        set choice [tk_messageBox -icon warning -type yesno -title "Rappture: Save Changes?" -message "Changes to the current tool haven't been saved.\n\nSave changes?"]
        if {$choice == "yes" && ![main_saveas]} {
            return
        }
    }

    if {$what == "-new"} {
        set xmlobj [Rappture::LibraryObj ::#auto "<?xml version=\"1.0\"?><run><tool/></run>"]
    } else {
        if {$what == "-file"} {
            set fname [tk_getOpenFile -title "Rappture: Open Tool" -initialfile "tool.xml" -defaultextension .xml -filetypes { {{XML files} .xml} {{All files} *} }]
        } else {
            set fname $what
        }

        if {"" == $fname} {
            return
        }

        # save the current file name in the "Save As..." dialog for later
        set LastToolXmlFile $fname
        .saveas.opts.toolv.file configure -text $fname
        main_saveas update

        if {[catch {Rappture::library $fname} xmlobj]} {
            tk_messageBox -icon error -title "Rappture: Error" -message "Error loading tool description: $xmlobj"
            return
        }
    }

    # plug in the new tool definition
    set LastToolXmlLoaded [$xmlobj xml]
    set ValueId 0

    # clear the tree and load the new data
    set win [.func.build.options.panes pane 0]
    $win.scrl.skel tree delete all
    pack forget .func.build.options.errs

    set alist [main_open_import_attrs $xmlobj tool]
    $win.scrl.skel tree insert 0 end \
        type "Tool" terminal yes dragdrop no attributes $alist

    # all the Input section
    set n [$win.scrl.skel tree insert 0 end \
        type "Input" terminal no dragdrop no]
    main_open_children $win.scrl.skel $xmlobj $n

    # all the Output section
    set n [$win.scrl.skel tree insert 0 end \
        type "Output" terminal no dragdrop no]
    main_open_children $win.scrl.skel $xmlobj $n
}

# ----------------------------------------------------------------------
# USAGE: main_open_children <hierlist> <xmlobj> <node>
#
# Called by main_open to insert children in the <xml> at the specified
# <node> into the given <hierlist> widget.  The children must be
# recognized object types.
# ----------------------------------------------------------------------
proc main_open_children {hierlist xmlobj node} {
    global ValueId

    set path [$hierlist tree path $node "%lc:type(%id)"]
    regsub -all {\(%id\)} $path "" path

    foreach elem [$xmlobj children $path] {
        set type [$xmlobj element -as type $path.$elem]

        # make sure that this is a recognized object type
        if {[catch {Rappture::objects::get $type -image} result] == 0
              && [string length $result] > 0} {

            # get info about this object
            set id [$xmlobj element -as id $path.$elem]
            set term [Rappture::objects::get $type -terminal]
            set alist [main_open_import_attrs $xmlobj $path.$elem]

            # add it into the tree
            set newnode [$hierlist tree insert $node end \
                type [string totitle $type] id $id terminal $term \
                attributes $alist]

            # find the highest auto-generated node and start from there
            if {[regexp {^value([0-9]+)$} $id match num]} {
                if {$num > $ValueId} {
                    set ValueId $num
                }
            }

            if {!$term} {
                # if it can have children, then add any children
                main_open_children $hierlist $xmlobj $newnode
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: main_open_import_attrs <xml> <path>
#
# Called by main_open_children to import attributes from the place
# <path> in the <xml> object.
# ----------------------------------------------------------------------
proc main_open_import_attrs {xmlobj path} {
    set alist ""
    set type [$xmlobj element -as type $path]
    set side [lindex [split $path .] 0]

    if {[catch {Rappture::objects::get $type -attributes} attr] == 0} {
        # scan through and ingest all known attributes
        foreach rec $attr {
            set name [lindex $rec 0]
            catch {unset info}
            array set info [lrange $rec 1 end]

            # see if this attribute is excluded based on the -only option
            if {$info(-only) ne "" && [lsearch $info(-only) $side] < 0} {
                continue  ;# skip it!
            }

            set atype [lindex [split $info(-type) :] 0]
            set proc "Attr[string totitle $atype]::import"
            if {[catch {$proc $xmlobj $path.$info(-path)} result]} {
                puts "ERROR: $result"
                puts "  skipping $name attribute of $path"
            } else {
                lappend alist $name $result
            }
        }
    }
    return $alist
}

# ----------------------------------------------------------------------
# USAGE: main_errors ?-strict?
#
# Checks the values of all objects in the tree to look for errors
# before previewing and saving.  If any errors are found and the
# -strict flag is set, the user is forced to look at them before
# continuing on.  Otherwise, the user may choose to look at warnings
# and errors before continuing.  Usually called before previewing
# and saving the tool definition.  Returns 1 if there are errors that
# should be viewed/fixed, and 0 otherwise.
# ----------------------------------------------------------------------
proc main_errors {args} {
    global ErrList ErrListPos

    set strict no
    while {[llength $args] > 0} {
        set flag [lindex $args 0]
        set args [lrange $args 1 end]
        if {$flag eq "-strict"} {
            set strict yes
        } else {
            error "bad option \"$flag\": should be -strict"
        }
    }

    set ErrList ""
    set ErrListPos 0
    set ErrFocusAttr ""
    pack forget .func.build.options.errs

    set counter 0
    set win [.func.build.options.panes pane 0]
    set hlist $win.scrl.skel
    set nodelist [$hlist tree children 0]
    while {[llength $nodelist] > 0} {
        set node [lindex $nodelist 0]
        set nodelist [lrange $nodelist 1 end]

        # add children into the node traversal list
        set cnodes [$hlist tree children $node]
        if {[llength $cnodes] > 0} {
            set nodelist [eval linsert [list $nodelist] 0 $cnodes]
        }

        set type [string tolower [$hlist tree get $node type]]

        # make sure this object has a viewer
        if {[catch {Rappture::objects::get $type -palettes} pals]} {
            # must not have an object definition
            continue
        }
        set debug [list -node $node -counter [incr counter]]

        set path [$hlist tree path $node "%lc:type(%id)"]
        regsub -all {\(%id\)} $path "" path
        # convert input.foo to palette name "Inputs"
        set side [lindex [split $path .] 0]
        set pname "[string totitle $side]s"

        if {$side ne "tool" && [lsearch $pals $pname] < 0} {
            lappend ErrList [list error "This object doesn't yet have a visualizer that lets it be used as an $side to the program.  Support for this probably will be added in a future version of Rappture, but for now, it must be removed from the $side section." $debug]
            continue
        }

        # query attributes for this type and check their values
        if {[catch {Rappture::objects::get $type -attributes} attrdef]
             || [catch {$hlist tree get $node attributes} attrlist]} {
            # can't find any attributes for this node or node type
            continue
        }
        array set ainfo $attrlist

        foreach rec $attrdef {
            set name [lindex $rec 0]
            if {![info exists ainfo($name)]} {
                set ainfo($name) ""
            }
        }

        # perform all checks for this attribute type and append to ErrList
        eval lappend ErrList [Rappture::objects::check $type \
            $side [array get ainfo] $debug]
    }

    if {[llength $ErrList] > 0} {
        # arrange errors in order of severity
        set ErrList [lsort -decreasing -command main_errors_cmp $ErrList]

        # count the errors and warnings
        set nerrs 0
        set nwarn 0
        set nother 0
        foreach rec $ErrList {
            switch -- [lindex $rec 0] {
                error   { incr nerrs }
                warning { incr nwarn }
                default { incr nother }
            }
        }

        set phrases ""
        if {$nerrs == 1} {
            lappend phrases "1 error"
        } elseif {$nerrs > 1} {
            lappend phrases "$nerrs errors"
        }
        if {$nwarn == 1} {
            lappend phrases "1 warning"
        } elseif {$nwarn > 1} {
            lappend phrases "$nwarn warnings"
        }
        if {$nother == 1} {
            lappend phrases "1 other suggestion"
        } elseif {$nother > 1} {
            lappend phrases "$nother suggestions"
        }
        switch -- [llength $phrases] {
            1 { set phrases [lindex $phrases 0] }
            2 { set phrases [join $phrases " and "] }
            3 { set phrases [join $phrases "%"]
                regsub "%" $phrases ", " phrases
                regsub "%" $phrases ", and " phrases
            }
        }

        if {$nerrs+$nwarn+$nother == 1} {
            set thereis "There is"
            set problem "this problem"
            set errors "this error"
        } else {
            set thereis "There are"
            set problem "these problems"
            if {$nwarn+$nother == 0} {
                set errors "these errors"
            } else {
                set errors "at least the $nerrs error"
                if {$nerrs > 1} {lappend errors "s"}
            }
        }

        # if there are errors, we can't continue on
        if {$nerrs > 0} {
            if {$strict} {
                tk_messageBox -icon error -type ok -title "Rappture: Problems with your tool definition" -message "$thereis $phrases for your current tool definition.  You must resolve $errors before you continue with the preview."
                return 1
            } else {
                set resp [tk_messageBox -icon error -type yesno -title "Rappture: Problems with your tool definition" -message "$thereis $phrases for your current tool definition.  Examine and resolve $problem?"]
                return $resp
            }
        }

        # let the user decide whether to proceed
        set choice [tk_messageBox -icon error -type yesno -title "Rappture: Problems with your tool definition" -message "$thereis $phrases for your current tool definition.  Examine and resolve $problem?"]
        if {$choice == "yes"} {
            return 1
        } else {
            return 0
        }
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: main_errors_nav index|next|prev
#
# Navigates through the error messages generated by the last call
# to main_errors, putting up the next message in the sequence.
# ----------------------------------------------------------------------
proc main_errors_nav {where} {
    global ErrList ErrListPos ErrFocusAttr

    if {![main_options_save]} {
        return
    }

    if {$where == "next"} {
        incr ErrListPos
    } elseif {$where == "prev"} {
        incr ErrListPos -1
    } elseif {[string is integer -strict $where]} {
        set ErrListPos $where
    } else {
        error "bad option \"$where\": should be next or prev"
    }

    if {$ErrListPos < 0} {
        set ErrListPos [expr {[llength $ErrList]-1}]
    } elseif {$ErrListPos >= [llength $ErrList]} {
        set ErrListPos 0
    }

    set err [lindex $ErrList $ErrListPos]
    set class [string totitle [lindex $err 0]]
    set mesg [lindex $err 1]
    array set debug [lindex $err 2]

    .func.build.options.errs.info configure -text "$class: $mesg"

    if {$class == "Error"} {
        .func.build.options.errs.exclaim config -image [Rappture::icon err24]
    } else {
        .func.build.options.errs.exclaim config -image [Rappture::icon warn24]
    }

    set win [.func.build.options.panes pane 0]
    if {[info exists debug(-attribute)]} {
        set ErrFocusAttr $debug(-attribute)
    } else {
        set ErrFocusAttr ""
    }
    $win.scrl.skel select none -silent  ;# force a reload of options
    $win.scrl.skel select $debug(-node) ;# so we can highlight ErrFocusAttr

    # fix the navigation buttons for next/prev error
    .func.build.options.errs.nav.prev configure \
        -state [expr {($ErrListPos == 0) ? "disabled" : "normal"}]
    .func.build.options.errs.nav.next configure \
        -state [expr {($ErrListPos >= [llength $ErrList]-1) ? "disabled" : "normal"}]
}

# ----------------------------------------------------------------------
# USAGE: main_errors_cmp <err1> <err2>
#
# Used to compare two error records when sorting the main ErrList.
# Each record should have the following form:
#
#   error "this describes the error" {-node n -counter c -attribute a}
#
# Errors appear at the top, followed by warnings and suggestions.
# Each record has a counter value, which determines its position in
# the tree.  Serious errors near the top of the tree are shown first.
# ----------------------------------------------------------------------
proc main_errors_cmp {err1 err2} {
    array set severity {
        suggestion 0
        warning 1
        error 2
    }

    set eclass1 [lindex $err1 0]
    array set einfo1 [lindex $err1 2]

    set eclass2 [lindex $err2 0]
    array set einfo2 [lindex $err2 2]

    if {$severity($eclass1) > $severity($eclass2)} {
        return 1
    } elseif {$severity($eclass1) < $severity($eclass2)} {
        return -1
    }
    if {$einfo1(-counter) > $einfo2(-counter)} {
        return -1  ;# higher counters are further down in tree
    } elseif {$einfo1(-counter) < $einfo2(-counter)} {
        return 1   ;# put lower counters near the top
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: main_generate_xml
#
# Builds an XML representation of the current contents of the tool
# defintion tree.  Usually called just before previewing or saving
# the tool to build a fresh XML representation with all changes.
# Returns 1 if the XML was successfully generated, and 0 if something
# went wrong.
# ----------------------------------------------------------------------
proc main_generate_xml {} {
    global ToolXml

    # save any outstanding changes from the current options panel
    if {![main_options_save]} {
        return 0
    }

    # create a fresh XML object
    if {"" != $ToolXml} {
        itcl::delete object $ToolXml
    }
    set ToolXml [Rappture::LibraryObj ::#auto "<?xml version=\"1.0\"?><run><tool/></run>"]

    set win [.func.build.options.panes pane 0]
    set hlist $win.scrl.skel
    set nodelist [$hlist tree children 0]
    while {[llength $nodelist] > 0} {
        set node [lindex $nodelist 0]
        set nodelist [lrange $nodelist 1 end]

        # add children into the node traversal list
        set cnodes [$hlist tree children $node]
        if {[llength $cnodes] > 0} {
            set nodelist [eval linsert [list $nodelist] 0 $cnodes]
        }

        set path [$hlist tree path $node "%lc:type(%id)"]
        regsub -all {\(%id\)} $path "" path
        set type [string tolower [$hlist tree get $node type]]
        set side [lindex [split $path .] 0]

        if {[catch {Rappture::objects::get $type -attributes} attrdef]
             || [catch {$hlist tree get $node attributes} attrlist]} {
            # can't find any attributes for this node or node type
            continue
        }
        array set ainfo $attrlist

        foreach rec $attrdef {
            set name [lindex $rec 0]
            if {![info exists ainfo($name)]} {
                continue
            }
            array set info [lrange $rec 1 end]

            # see if this attribute is excluded based on the -only option
            if {$info(-only) ne "" && [lsearch $info(-only) $side] < 0} {
                continue  ;# skip it!
            }

            set atype [lindex [split $info(-type) :] 0]
            set proc "Attr[string totitle $atype]::export"
            if {[catch {$proc $ToolXml $path.$info(-path) $ainfo($name)} result]} {
                puts "ERROR: $result"
                puts "  skipping $name attribute of $path"
            }
        }
    }
    return 1
}

# ----------------------------------------------------------------------
# USAGE: main_saveas start|cancel|finish|update|gettoolfile|getprogfile
#
# Handles the Save As... operation, which saves the current tool
# information into a tool.xml file.  Prompts the user to determine
# the save file, and then saves the data--unless the user cancels.
# ----------------------------------------------------------------------
proc main_saveas {{option "start"}} {
    global tcl_platform SaveAs ToolXml LastToolXmlFile LastToolXmlLoaded

    switch -- $option {
        start {
            if {![main_options_save -clear]} {
                return
            }
            set win [.func.build.options.panes pane 0]
            $win.scrl.skel select none

            # freshen up the ToolXml
            if {![main_generate_xml]} {
                # something went wrong while saving the xml
                # pull up the build tab, so we can see the error
                .func select [.func index -name "Build"]
                return
            }

            if {[main_errors]} {
                .func select [.func index -name "Build"]
                pack .func.build.options.errs \
                    -before .func.build.options.panes \
                    -pady {10 0} -fill x
                main_errors_nav 0
                return
            }

            # get the language choice and update the skeleton program option
            set lang ""
            set cmd [$ToolXml get tool.command]
            if {[string index $cmd 0] == "!"} {
                set lang [string range $cmd 1 end]
            } else {
                # It's hard to reload a real command from a tool.xml file.
                # Scan through all languages and treat @@FILENAME@@ as a
                # wildcard.  See if any of the languages match.
                foreach name [RapptureBuilder::templates::languages] {
                    set pat [RapptureBuilder::templates::generate command \
                        -language $name -macros {@@FILENAME@@ * @@FILEROOT@@ *}]

                    if {[string match $pat $cmd]} {
                        set lang $name
                        break
                    }
                }
            }
            if {$lang ne ""} {
                .saveas.opts.prog configure -text "Skeleton program ($lang)" -state normal
                .saveas.opts.progv.getfile configure -state normal
                set ext [RapptureBuilder::templates::generate extension -language $lang]
                .saveas.opts.progv.file configure -text "main$ext"
                set SaveAs(lang) $lang
                set SaveAs(ext) $ext

                # see if there's a makefile for this program
                if {[catch {RapptureBuilder::templates::generate makefile -language $lang}]} {
                    set SaveAs(make) 0
                    .saveas.opts.make configure -state disabled
                } else {
                    .saveas.opts.make configure -state normal
                }
            } else {
                .saveas.opts.prog configure -text "Skeleton program (select language under Tool section)" -state disabled
                .saveas.opts.progv.file configure -text "select a file"
                .saveas.opts.progv.getfile configure -state disabled
                .saveas.opts.make configure -state disabled
                set SaveAs(lang) ?
                set SaveAs(ext) ?
                set SaveAs(prog) 0
                set SaveAs(make) 0
            }
            main_saveas update

            # make the saveas dialog big enough and center it on the main win
            set w 400
            set ho [winfo reqheight .saveas.opts]
            set hc [winfo reqheight .saveas.cntls]
            set h [expr {$ho+$hc+40}]
            set x0 [winfo rootx .]
            set y0 [winfo rooty .]
            set wmain [winfo width .]
            set hmain [winfo height .]
            set x0 [expr {$x0+($wmain-$w)/2}]
            set y0 [expr {$y0+($hmain-$h)/2}]

            wm geometry .saveas ${w}x${h}+${x0}+${y0}
            wm deiconify .saveas
            raise .saveas
            grab set .saveas

            # wait for the user to save/cancel and return 1/0
            vwait SaveAs(status)
            return [string equal $SaveAs(status) "ok"]
        }

        cancel {
            grab release .saveas
            wm withdraw .saveas
            after idle [list set SaveAs(status) "cancelled"]
        }

        finish {
            grab release .saveas
            wm withdraw .saveas
            set status "ok"

            set tfile [.saveas.opts.toolv.file cget -text]
            set pfile [.saveas.opts.progv.file cget -text]

            if {$SaveAs(tool) && $tfile ne "select a file"} {
                set tfile [file normalize $tfile]

                if {$SaveAs(prog) && $pfile ne "select a file"} {
                    # if we have a tool.xml file, put the program in same dir
                    set tail [file tail $pfile]
                    set pfile [file join [file dirname $tfile] $tail]
                }
            }

            if {$SaveAs(tool) && $tfile ne "select a file"} {
                # generate again now that we know the file name to get
                # relative file paths for things like <note>'s
                set LastToolXmlFile $tfile
                main_generate_xml

                # change any !Language for the command into a proper
                # invocation command.  Grab the XML before we do this
                # as the last official XML that's been saved.  We'll
                # know later if things changed and we need to save again.
                set xmlBeforeCmdSubst [$ToolXml xml]

                set cmd [$ToolXml get tool.command]
                if {[string index $cmd 0] eq "!"} {
                    if {$pfile eq "select a file"} {
                        set pfile "main$SaveAs(ext)"
                    }
                    set tail [file tail $pfile]
                    set cmd [RapptureBuilder::templates::generate command \
                        -language $SaveAs(lang) \
                        -macros [list @@FILENAME@@ $tail \
                                      @@FILEROOT@@ [file rootname $tail]]]
                    $ToolXml put tool.command $cmd
                }

                set cmds {
                    set fid [open $tfile w]
                    puts $fid "<?xml version=\"1.0\"?>"
                    puts -nonewline $fid [$ToolXml xml]
                    close $fid
                }
                if {[catch $cmds result]} {
                    tk_messageBox -icon error -title "Rappture: Error" -message "Error saving tool description: $result"
                    set status "error"
                } else {
                    set LastToolXmlLoaded $xmlBeforeCmdSubst
                }
            }

            if {$SaveAs(prog) && $pfile ne "select a file"} {
                if {[file exists $pfile]} {
                    set choice [tk_messageBox -icon warning -type yesno -title "Rappture: Confirm" -message "File \"$pfile\" already exists.\n\nOverwrite?"]
                    if {$choice == "no"} {
                        # pop the dialog back up
                        wm deiconify .saveas
                        raise .saveas
                        grab set .saveas
                        return
                    }
                }
                set code [RapptureBuilder::templates::generate main \
                    -language $SaveAs(lang) -xmlobj $ToolXml]

                set cmds {
                    set fid [open $pfile w]
                    puts -nonewline $fid $code
                    close $fid
                }
                if {[catch $cmds result]} {
                    tk_messageBox -icon error -title "Rappture: Error" -message "Error saving skeleton program file: $result"
                    set status "error"
                }
            }

            if {$SaveAs(make)} {
                if {$pfile eq "select a file"} {
                    set pfile "main$SaveAs(ext)"
                }
                set fname [file tail $pfile]
                set mfile [file join [file dirname $pfile] Makefile]
                if {[file exists $mfile]} {
                    set choice [tk_messageBox -icon warning -type yesno -title "Rappture: Confirm" -message "File \"$mfile\" already exists.\n\nOverwrite?"]
                    if {$choice == "no"} {
                        # pop the dialog back up
                        wm deiconify .saveas
                        raise .saveas
                        grab set .saveas
                        return
                    }
                }

                # /apps/rappture/current for 32-bit systems
                if {$tcl_platform(wordSize) == 8 } {
                     if { [file isdirectory /apps/share64/debian7/rappture/current]} {
                        set dir /apps/share64/debian7/rappture/current
                     } elseif { [file isdirectory /apps/share64/debian6/rappture/current]} {
                        set dir /apps/share64/debian6/rappture/current
                     } else {
                        set dir /apps/rappture/current
                     }
                } else {
                    set dir /apps/rappture/current
                }
                if {![file isdirectory $dir]} {
                    # if all else fails, ask the current Rappture for its install dir
                    set dir [file dirname [file dirname $::Rappture::installdir]]
                }

                set macros [list @@RAPPTUREDIR@@ $dir]
                lappend macros @@FILENAME@@ $fname
                lappend macros @@FILEROOT@@ [file rootname $fname]
                if {[catch {RapptureBuilder::templates::generate makefile -language $SaveAs(lang) -macros $macros} code] == 0} {
                    set cmds {
                        set fid [open $mfile w]
                        puts -nonewline $fid $code
                        close $fid
                    }
                    if {[catch $cmds result]} {
                        tk_messageBox -icon error -title "Rappture: Error" -message "Error saving Makefile: $result"
                        set status "error"
                    }
                }
            }

            after idle [list set SaveAs(status) $status]
        }

        update {
            set havefile 0
            set missingfile 0
            set tfile [.saveas.opts.toolv.file cget -text]
            if {$SaveAs(tool)} {
                if {$tfile ne "select a file"} {
                    .saveas.opts.toolv.file configure -fg black
                    set havefile 1
                } else {
                    set missingfile 1
                }
            } else {
                .saveas.opts.toolv.file configure -fg gray60
            }

            set pfile [.saveas.opts.progv.file cget -text]
            if {$SaveAs(prog)} {
                if {$pfile ne "select a file"} {
                    .saveas.opts.progv.file configure -fg black
                    set havefile 1
                } else {
                    set missingfile 1
                }
            } else {
                .saveas.opts.progv.file configure -fg gray60
            }

            if {$havefile && !$missingfile} {
                .saveas.cntls.save configure -state normal
            } else {
                .saveas.cntls.save configure -state disabled
            }
        }

        gettoolfile {
            set fname [tk_getSaveFile -title "Rappture: Save Tool" -parent .saveas -initialfile "tool.xml" -defaultextension .xml -filetypes { {{XML files} .xml} {{All files} *} }]

            if {"" != $fname} {
                .saveas.opts.toolv.file configure -text $fname
                set SaveAs(tool) 1
                main_saveas update
            }
        }

        getprogfile {
            set flist ""
            lappend flist [list "$SaveAs(lang) files" $SaveAs(ext)]
            lappend flist [list "All files" *]

            set pfile [.saveas.opts.progv.file cget -text]
            if {$pfile eq "select a file"} {
                set init ""
                set ext ""
            } else {
                set init $pfile
                set ext [file extension $pfile]
            }

            set fname [tk_getSaveFile -title "Rappture: Save Program Skeleton" -parent .saveas -initialfile $init -defaultextension $ext -filetypes $flist]

            if {"" != $fname} {
                .saveas.opts.progv.file configure -text $fname
                set SaveAs(prog) 1
                main_saveas update
            }
        }

        default {
            error "bad option \"$option\": should be start, cancel, finish, gettoolfile, getprogfile"
        }
    }

}

# ----------------------------------------------------------------------
# USAGE: main_exit
#
# Handles the Save As... operation, which saves the current tool
# information into a tool.xml file.  Prompts the user to determine
# the save file, and then saves the data--unless the user cancels.
# ----------------------------------------------------------------------
proc main_exit {} {
    global ToolXml LastToolXmlLoaded

    if {![main_options_save -clear]} {
        return
    }
    set win [.func.build.options.panes pane 0]
    $win.scrl.skel select none

    main_generate_xml

    if {[$ToolXml xml] ne $LastToolXmlLoaded} {
        set choice [tk_messageBox -icon warning -type yesnocancel -title "Rappture: Save Changes?" -message "Changes to the current tool haven't been saved.\n\nSave changes?"]
        if {$choice == "yes" && ![main_saveas]} {
            return
        } elseif {$choice == "cancel"} {
            return
        }
    }
    exit
}

# ----------------------------------------------------------------------
# USAGE: main_preview
#
# Handles the "Preview" tab.  Whenever a preview is needed, this
# function clears the current contents of the preview window and
# builds a new set of input controls based on the current XML.
# ----------------------------------------------------------------------
proc main_preview {} {
    global ToolXml ToolPreview

    # while we're checking for errors, put up a striped pattern over
    # the preview area
    set w [winfo width .func.preview]
    set h [winfo height .func.preview]
    place .func.preview.stripes -x 0 -y 0 -width $w -height $h
    raise .func.preview.stripes

    # freshen up the ToolXml
    if {![main_generate_xml]} {
        # something went wrong while saving the xml
        # pull up the build tab, so we can see the error
        .func select [.func index -name "Build"]
        return
    }

    if {[main_errors -strict]} {
        .func select [.func index -name "Build"]
        pack .func.build.options.errs -before .func.build.options.panes \
            -pady {10 0} -fill x
        main_errors_nav 0
        return
    }

    # clear all current widgets in the preview window
    place forget .func.preview.stripes
    set win .func.preview
    $win.pager delete 0 end

    if {"" != $ToolPreview} {
        itcl::delete object $ToolPreview
        set ToolPreview ""
    }
    set ToolPreview [Rappture::Tool ::#auto $ToolXml [pwd]]

    # add new widgets based on the phases
    set phases [$ToolXml children -type phase input]
    if {[llength $phases] > 0} {
        set plist ""
        foreach name $phases {
            lappend plist input.$name
        }
        set phases $plist
    } else {
        set phases input
    }

    if {[llength $phases] == 1} {
        $win.pager configure -arrangement side-by-side
    } else {
        $win.pager configure -arrangement pages
    }

    foreach comp $phases {
        if {"" == [$ToolXml element $comp]} {
            # empty element? then skip it
            continue
        }
        set title [$ToolXml get $comp.about.label]
        if {$title == ""} {
            set title "Input #auto"
        }
        $win.pager insert end -name $comp -title $title

        #
        # Build the page of input controls for this phase.
        #
        set f [$win.pager page $comp]
        Rappture::Page $f.cntls $ToolPreview $comp
        pack $f.cntls -expand yes -fill both
    }

    # add an analyzer for output widgets
    set simtxt [$ToolXml get tool.action.label]
    if {"" == $simtxt} {
        set simtxt "Simulate"
    }
    $win.pager insert end -name analyzer -title $simtxt
    set f [$win.pager page analyzer]
    # note: simcontrol on but disabled due to _simState code above
    Rappture::Analyzer $f.analyze $ToolPreview -simcontrol on \
        -notebookpage about
    pack $f.analyze -expand yes -fill both

    # copy the ToolXml object and pass to analyzer to show outputs
    set synthrun [Rappture::LibraryObj ::#auto "<?xml version=\"1.0\"?><run><tool/></run>"]
    $synthrun copy "" from $ToolXml ""
    $f.analyze load $synthrun
    $f.analyze configure -notebookpage analyze

    # turn off download options and clear button
    $f.analyze component download configure -state disabled
    $f.analyze component resultselector component clear configure -state disabled
    # remove the "---" and "Download..." options from the view selector
    $f.analyze component viewselector choices delete end
    $f.analyze component viewselector choices delete end
}

# ----------------------------------------------------------------------
# MAIN WINDOW
# ----------------------------------------------------------------------
wm geometry . 800x600
Rappture::Postern .postern
pack .postern -side bottom -fill x

blt::tabset .func -borderwidth 0 -relief flat -side top -tearoff 0 \
    -highlightthickness 0
pack .func -expand yes -fill both

# ----------------------------------------------------------------------
# BUILD AREA
# ----------------------------------------------------------------------
frame .func.build
.func insert end "Build" -window .func.build -fill both

frame .func.build.cntls
pack .func.build.cntls -fill x
button .func.build.cntls.new -text "New..." -command {main_open -new}
pack .func.build.cntls.new -side left -padx 2 -pady 4
button .func.build.cntls.open -text "Open..." -command {main_open -file}
pack .func.build.cntls.open -side left -padx 2 -pady 4
button .func.build.cntls.save -text "Save As..." -command main_saveas
pack .func.build.cntls.save -side left -padx 2 -pady 4

frame .func.build.side
pack .func.build.side -side left -fill y -pady 10
label .func.build.side.l -text "Object Types:"
pack .func.build.side.l -side top -anchor w

Rappture::Slideframes .func.build.side.palettes -animstartcommand {
    # turn scrollbars off during animation
    for {set i 0} {$i < [.func.build.side.palettes size]} {incr i} {
        set win [.func.build.side.palettes page @$i]
        $win.scrl configure -yscrollmode off
    }
} -animendcommand {
    # turn scrollbars back on when animation is over
    for {set i 0} {$i < [.func.build.side.palettes size]} {incr i} {
        set win [.func.build.side.palettes page @$i]
        $win.scrl configure -yscrollmode auto
    }
}
pack .func.build.side.palettes -expand yes -fill both

set plist [Rappture::objects::palettes]
set plist [linsert $plist 0 "All"]

set pnum 0
foreach ptitle $plist {
    set win [.func.build.side.palettes insert end $ptitle]

    Rappture::Scroller $win.scrl -xscrollmode off -yscrollmode auto
    pack $win.scrl -expand yes -fill both
    set iwin [$win.scrl contents frame]
    Rappture::Filmstrip $iwin.strip -orient vertical \
        -dragdropcommand main_palette_source
    pack $iwin.strip -expand yes -fill both

    foreach name [lsort [Rappture::objects::get]] {
        set imh [Rappture::objects::get $name -image]
        if {"" == $imh} {
            continue
        }
        set plist [Rappture::objects::get $name -palettes]
        if {$ptitle == "All" || [lsearch $plist $ptitle] >= 0} {
            $iwin.strip add $name -image $imh
        }
    }
}

# ----------------------------------------------------------------------
#  OPTIONS AREA
# ----------------------------------------------------------------------
frame .func.build.options
pack .func.build.options -expand yes -fill both -padx 10

frame .func.build.options.errs
frame .func.build.options.errs.nav
pack .func.build.options.errs.nav -side bottom -fill x -padx 2 -pady 2
button .func.build.options.errs.nav.next -text "Next >" -command {main_errors_nav next}
pack .func.build.options.errs.nav.next -side right
button .func.build.options.errs.nav.prev -text "< Prev" -command {main_errors_nav prev}
pack .func.build.options.errs.nav.prev -side right -padx 4

button .func.build.options.errs.x -bitmap [Rappture::icon dismiss] \
    -command {pack forget .func.build.options.errs}
pack .func.build.options.errs.x -side right -anchor n -padx 4 -pady 8
label .func.build.options.errs.exclaim -image [Rappture::icon err24]
pack .func.build.options.errs.exclaim -side left -anchor nw -padx 4 -pady 4
label .func.build.options.errs.info -text "" -anchor nw -justify left
pack .func.build.options.errs.info -side left -expand yes -fill both -pady {6 0}
bind .func.build.options.errs.info <Configure> {.func.build.options.errs.info configure -wraplength [expr {%w-4}]}

Rappture::Panes .func.build.options.panes -orientation vertical
pack .func.build.options.panes -expand yes -fill both -padx 10
.func.build.options.panes insert end -fraction 0.5

set win [.func.build.options.panes pane 0]
label $win.heading -text "Tool Interface:"
pack $win.heading -anchor w -pady {12 0}
Rappture::Scroller $win.scrl -xscrollmode auto -yscrollmode auto
pack $win.scrl -expand yes -fill both

Rappture::Hierlist $win.scrl.skel
$win.scrl contents $win.scrl.skel

bind $win.scrl.skel <<Selection>> main_options_load
bind $win.scrl.skel <<SelectionPath>> main_options_rename

set win [.func.build.options.panes pane 1]
Rappture::Scroller $win.vals -xscrollmode none -yscrollmode auto
pack $win.vals -expand yes -fill both

# ----------------------------------------------------------------------
# USAGE: main_options_load
#
# Invoked whenever the selection changes in the tree of all known
# objects.  Gets the current selection and pops up controls to edit
# the attributes.
# ----------------------------------------------------------------------
set OptionsPanelNode ""

proc main_options_load {} {
    global OptionsPanelNode OptionsPanelObjs ErrFocusAttr

    set win [.func.build.options.panes pane 0]
    set hlist $win.scrl.skel

    if {![main_options_save -clear]} {
        # something is wrong with the previous edits
        # show the last node as the current selection and fix the error
        if {"" != $OptionsPanelNode} {
            $hlist select $OptionsPanelNode -silent
        }
        return
    }

    # figure out the current selection
    set node [$hlist curselection]
    set type [$hlist curselection -field type]
    set path [$hlist curselection -path "%lc:type(%id)"]
    regsub -all {\(%id\)} $path "" path
    set side [lindex [split $path .] 0]

    # set the value column to expand
    set win [.func.build.options.panes pane 1]
    set frame [$win.vals contents frame]
    grid columnconfigure $frame 1 -weight 1

    # create new widgets for each object attribute
    if {"" != $type && [lsearch {Inputs Outputs} $type] < 0
          && [catch {Rappture::objects::get $type -attributes} attr] == 0} {

        frame $frame.cntls -borderwidth 4 -relief flat
        grid $frame.cntls -row 0 -column 0 -columnspan 2 -sticky nsew

        if {$type ne "Tool"} {
            # can't let people delete the tool info
            button $frame.cntls.del -text "Delete" -command main_options_delete
            pack $frame.cntls.del -side right
        }

        button $frame.cntls.help -text "Help" -command main_options_help
        pack $frame.cntls.help -side right

        # put this in last so it gets squeezed out
        Rappture::ObjPath $frame.cntls.path -label "Object" -pathtext $path \
            -renamecommand main_options_rename
        pack $frame.cntls.path -side left -expand yes -fill both

        if {$type eq "Tool"} {
            # if this is a tool, then lose the rename button
            pack forget [$frame.cntls.path component button]
        }

        array set ainfo [$hlist curselection -field attributes]

        set wnum 1
        foreach rec $attr {
            catch {unset info}
            set name [lindex $rec 0]
            array set info [lrange $rec 1 end]

            # see if this attribute is excluded based on the -only option
            if {$info(-only) ne "" && [lsearch $info(-only) $side] < 0} {
                continue  ;# skip it!
            }

            # create the widget to edit this attribute
            set atype [split $info(-type) :]
            set atname [lindex $atype 0]
            set atname "Attr[string totitle $atname]"
            set atargs ""
            foreach term [split [lindex $atype 1] ,] {
                set key [lindex [split $term =] 0]
                set val [lindex [split $term =] 1]
                if {$key == "validate"} {
                    set val "validate_$val"
                }
                lappend atargs -$key $val
            }

            # add any tooltip info for this attribute
            if {[info exists info(-tooltip)] && $info(-tooltip) ne ""} {
                lappend atargs -tooltip $info(-tooltip)
            }

            set win [frame $frame.val$wnum]
            if {[catch {eval $atname ::#auto $win $atargs} obj]} {
                puts stderr "attribute editor failed: $obj"
                destroy $win
                continue
            }
            if {[info exists ainfo($name)]} {
                $obj load $ainfo($name)
            }
            set OptionsPanelObjs($name) $obj

            # no title? then use the attribute name as a title
            if {[string length $info(-title)] == 0} {
                set info(-title) [string totitle $name]
            }
            label $frame.l$wnum -text "$info(-title):"
            grid $frame.l$wnum -row $wnum -column 0 -sticky e -padx 4 -pady 4
            grid $win -row $wnum -column 1 -sticky nsew -pady 4
            grid rowconfigure $frame $wnum -weight [expr {$info(-expand)? 1:0}]

            # if an error was found at this attribute, set focus there
            if {$name eq $ErrFocusAttr} {
                $obj edit
                set errbg [option get $frame errorHighlightColor Background]
                $frame.l$wnum configure -background $errbg
                set ErrFocusAttr ""
            }

            incr wnum
        }
    }

    # save this so we know later what node we're editing
    set OptionsPanelNode $node
}

# ----------------------------------------------------------------------
# USAGE: main_options_rename ?<name>?
#
# Used to change the id of the current selection to the given <name>.
# ----------------------------------------------------------------------
proc main_options_rename {args} {
    set win [.func.build.options.panes pane 0]
    set hlist $win.scrl.skel
    set node [$hlist curselection]

    if {[llength $args] > 0} {
        $hlist tree set $node id [lindex $args 0]
    }

    # get the updated path for this element and show in ObjPath widget
    set path [$hlist curselection -path "%lc:type(%id)"]
    regsub -all {\(%id\)} $path "" path

    set win [.func.build.options.panes pane 1]
    set frame [$win.vals contents frame]
    $frame.cntls.path configure -pathtext $path
}

# ----------------------------------------------------------------------
# USAGE: main_options_save ?-clear?
#
# Scans through all attribute widgets on the panel showing the current
# object and tells each one to save its value.  Returns 1 if successful,
# or 0 if errors are encountered.  Any errors are indicated by a popup
# cue under the widget with the bad value.
# ----------------------------------------------------------------------
proc main_options_save {{op -keep}} {
    global OptionsPanelNode OptionsPanelObjs
    set win [.func.build.options.panes pane 0]

    if {$OptionsPanelNode != "" && [catch {$win.scrl.skel tree get $OptionsPanelNode attributes} attrlist] == 0} {
        array set ainfo $attrlist

        # transfer values from the panel to the ainfo array
        foreach name [array names OptionsPanelObjs] {
            set obj $OptionsPanelObjs($name)
            if {![$obj save ainfo($name)]} {
                return 0
            }
        }

        # save current settings back in the tree
        $win.scrl.skel tree set $OptionsPanelNode attributes [array get ainfo]
    }

    if {$op == "-clear"} {
        foreach name [array names OptionsPanelObjs] {
            itcl::delete object $OptionsPanelObjs($name)
        }
        catch {unset OptionsPanelObjs}

        set bwin [.func.build.options.panes pane 1]
        set frame [$bwin.vals contents frame]
        foreach w [winfo children $frame] {
            destroy $w
        }
    }
    return 1
}

# ----------------------------------------------------------------------
# USAGE: main_options_help
#
# Handles the "Help" button for an object.  Pops up a help page
# explaining the object.
# ----------------------------------------------------------------------
proc main_options_help {} {
    # delete the node from the tree
    set win [.func.build.options.panes pane 0]
    set hlist $win.scrl.skel
    set node [$hlist curselection]
    set type [$hlist curselection -field type]
    if {"" != $node} {
        # look for a -help option for this node type
        set url [Rappture::objects::get $type -help]
        if {"" != $url} {
            Rappture::filexfer::webpage $url

            set win [.func.build.options.panes pane 1]
            set frame [$win.vals contents frame]
            Rappture::Tooltip::cue $frame.cntls.help "Popping up a web page with additional help information.  Making sure your web browser is not blocking pop-ups."
            after 2000 [list catch {Rappture::Tooltip::cue hide}]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: main_options_delete
#
# Handles the "Delete" button for an object.  Deletes the object and
# all of its children from the tree.
# ----------------------------------------------------------------------
proc main_options_delete {} {
    global OptionsPanelNode OptionsPanelObjs

    # delete the node from the tree
    set win [.func.build.options.panes pane 0]
    set hlist $win.scrl.skel
    set node [$hlist curselection]
    if {"" != $node} {
        # clear the current panel without saving
        foreach name [array names OptionsPanelObjs] {
            itcl::delete object $OptionsPanelObjs($name)
        }
        catch {unset OptionsPanelObjs}

        set win [.func.build.options.panes pane 1]
        set frame [$win.vals contents frame]
        foreach win [winfo children $frame] {
            destroy $win
        }

        # now delete the node itself
        $hlist tree delete $node
    }
}

# ----------------------------------------------------------------------
#  PREVIEW PANEL
# ----------------------------------------------------------------------
frame .func.preview
.func insert end "Preview" -window .func.preview -fill both
Rappture::Pager .func.preview.pager
pack .func.preview.pager -expand yes -fill both

bind .func.preview.pager <Map> main_preview

# use this frame to cover preview before it appears
blt::tile::frame .func.preview.stripes -tile [Rappture::icon diag]

# ----------------------------------------------------------------------
#  SAVE AS DIALOG
# ----------------------------------------------------------------------
toplevel .saveas
pack propagate .saveas off
wm title .saveas "Rappture: Save As..."
wm withdraw .saveas
wm protocol .saveas WM_DELETE_WINDOW {.saveas.cntls.cancel invoke}

frame .saveas.cntls
pack .saveas.cntls -side bottom -fill x -padx 4 -pady 4
button .saveas.cntls.cancel -text "Cancel" -underline 0 \
    -command {main_saveas cancel}
pack .saveas.cntls.cancel -side right -padx 4 -pady 4
button .saveas.cntls.save -text "Save" -underline 0 -default active \
    -command {main_saveas finish} -state disabled
pack .saveas.cntls.save -side right -padx 4 -pady 4

frame .saveas.opts
pack .saveas.opts -expand yes -fill both -padx 10 -pady 10
label .saveas.opts.l -text "What do you want to save?"
pack .saveas.opts.l -side top -anchor w

checkbutton .saveas.opts.tool -text "Tool definition file" -variable SaveAs(tool) -command {main_saveas update}
pack .saveas.opts.tool -side top -anchor w -pady {10 0}
frame .saveas.opts.toolv
pack .saveas.opts.toolv -anchor w
label .saveas.opts.toolv.filel -text "File:" -width 7 -anchor e
pack .saveas.opts.toolv.filel -side left
button .saveas.opts.toolv.getfile -text "Choose..." -command {main_saveas gettoolfile}
pack .saveas.opts.toolv.getfile -side right
label .saveas.opts.toolv.file -text "select a file" -anchor e -fg gray60
pack .saveas.opts.toolv.file -side left -expand yes -fill x
.saveas.opts.tool select

checkbutton .saveas.opts.prog -text "Skeleton program" -variable SaveAs(prog) -command {main_saveas update}
pack .saveas.opts.prog -side top -anchor w -pady {10 0}
frame .saveas.opts.progv
pack .saveas.opts.progv -anchor w
label .saveas.opts.progv.filel -text "File:" -width 7 -anchor e
pack .saveas.opts.progv.filel -side left
button .saveas.opts.progv.getfile -text "Choose..." -command {main_saveas getprogfile}
pack .saveas.opts.progv.getfile -side right
label .saveas.opts.progv.file -text "select a file" -anchor e -fg gray60
pack .saveas.opts.progv.file -side left -expand yes -fill x

checkbutton .saveas.opts.make -text "Makefile for building this program" -variable SaveAs(make) -command {main_saveas update}
pack .saveas.opts.make -side top -anchor w -pady {10 0}

# ----------------------------------------------------------------------
#  Open the given XML file or create a new one
# ----------------------------------------------------------------------
set ToolXml ""
set LastToolXmlLoaded ""
set LastToolXmlFile ""
set ToolPreview ""
set ErrFocusAttr ""

if {$params(-tool) ne ""} {
    if {![file exists $params(-tool)]} {
        puts stderr "can't find tool \"$params(-tool)\""
        exit 1
    }
    main_open $params(-tool)
} elseif {[file exists tool.xml]} {
    main_open tool.xml
} else {
    main_open -new
}

wm deiconify .
