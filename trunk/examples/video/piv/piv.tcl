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

# about page controls
set aboutcntls [frame $f.aboutcntls]
button $aboutcntls.next -text "Mark Particles" -command {$nb current next>}
pack $aboutcntls.next -side left -expand yes -padx 4 -pady 1

blt::table $f \
    0,0 $intro -rowspan 3 -fill both \
    1,1 $loader -fill x \
    2,1 $aboutcntls -fill x

blt::table configure $f c* -resize both
# blt::table configure $f r0 -pady 1

# ------------------------------------------------------------------
# VIDEO PAGE
# ------------------------------------------------------------------

set f [$nb insert end main]
$f configure -width 830 -height 530
Rappture::VideoScreen $f.viewer -width 830 -height 530
$loader.go configure -command {$nb current next>; $f.viewer loadcb}

#Rappture::VideoViewer $f.viewer
#$f.viewer load "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-05-55.mp4"
#$f.viewer load "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-09-56.mp4"
#$f.viewer load "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-13-56.mp4"
#$f.viewer load "/home/derrick/junk/coa360download56Kbps240x160.mpg"
pack $f.viewer -expand yes -fill both

# ------------------------------------------------------------------
# SHOW WINDOW
# ------------------------------------------------------------------

$f configure -width 2 -height 2
$nb current about
wm deiconify .
