#!/bin/sh
#\
exec wish "$0" $*
# ----------------------------------------------------------------------
# wish executes everything from here on...

package require RapptureGUI
package require BLT


option add *font -*-helvetica-medium-r-normal-*-12-*
option add *background #d9d9d9
option add *Tooltip.background white
option add *BugReport*banner*foreground white
option add *BugReport*banner*background #a9a9a9
option add *BugReport*banner*highlightBackground #a9a9a9
option add *BugReport*banner*font -*-helvetica-bold-r-normal-*-18-*
option add *upload*Label*font -*-helvetica-medium-r-normal-*-12-*
option add *download*Label*font -*-helvetica-medium-r-normal-*-12-*

switch $tcl_platform(platform) {
    unix - windows {
        event add <<PopupMenu>> <ButtonPress-3>
    }
    macintosh {
        event add <<PopupMenu>> <Control-ButtonPress-1>
    }
}

# install some Rappture support stuff...
Rappture::bugreport::install
Rappture::filexfer::init
Rappture::grab::init


wm title . "particle velocity estimate"
wm withdraw .


# ------------------------------------------------------------------
# Main Window
# ------------------------------------------------------------------

# create a notebook to hold the different pages

set nb [Rappture::Notebook .nb]
pack .nb -expand yes -fill both

# ------------------------------------------------------------------
# About Page
# ------------------------------------------------------------------

set f [$nb insert end about]

# loader for videos

set loader [frame $f.loader]

label $loader.icon -image [Rappture::icon folder]
pack $loader.icon -side left
label $loader.name -borderwidth 1 -relief solid -background white -anchor w
pack $loader.name -side left -expand yes -fill x -padx 2
button $loader.go -text "Upload..."
pack $loader.go -side right

# create an html intro page

set intro [frame $f.intro -borderwidth 1 -width 300 -height 300]

Rappture::Scroller $intro.scroller -xscrollmode auto -yscrollmode auto
pack $intro.scroller -expand yes -fill both

Rappture::HTMLviewer $intro.scroller.html
$intro.scroller contents $intro.scroller.html

set fid [open "intro.html" r]
set html [read $fid]
close $fid
$intro.scroller.html load $html

set demo [frame $f.demo]

blt::table $f \
    0,0 $intro -rowspan 3 -fill both \
    0,1 $demo -fill x \
    1,1 $loader -fill x

blt::table configure $f c* -resize both
# blt::table configure $f r0 -pady 1

# ------------------------------------------------------------------
# VIDEO PAGE
# ------------------------------------------------------------------

set f [$nb insert end video]
set movieViewer [Rappture::VideoScreen $f.viewer]
$loader.go configure -command {$nb current next>; $movieViewer loadcb}

pack $movieViewer -expand yes -fill both

# ------------------------------------------------------------------
# SHOW WINDOW
# ------------------------------------------------------------------

$nb current about
wm geometry . 900x600
wm deiconify .
update idletasks

# for testing we automatically load a video
array set videoFiles [list \
    1 "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-05-55.mp4" \
    2 "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-09-56.mp4" \
    3 "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-13-56.mp4" \
    4 "/home/derrick/junk/coa360download56Kbps240x160.mpg" \
]
$nb current video
update idletasks
$movieViewer load file $videoFiles(1)
