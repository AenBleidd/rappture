#!/bin/sh
#\
exec wish "$0" $*
# ----------------------------------------------------------------------
# wish executes everything from here on...

package require RapptureGUI
package require BLT
package require Img


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
set width 960
set height 630

set installdir [file dirname [ \
                Rappture::utils::expandPath [ \
                file normalize [info script]]]]



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



# html intro page

set intro [frame $f.intro]

Rappture::Scroller $intro.scroller -xscrollmode auto -yscrollmode auto
pack $intro.scroller -expand yes -fill both

Rappture::HTMLviewer $intro.scroller.html
$intro.scroller contents $intro.scroller.html

set fid [open "intro.html" r]
set html [read $fid]
close $fid
$intro.scroller.html load $html


# verticle divider
set div [frame $f.div -width 1 -background black]


set previewVar ""

# movie chooser

set chooser [frame $f.chooser_f]

set fid [open [file join $installdir images step1.png] r]
fconfigure $fid -translation binary -encoding binary
set data [read $fid]
close $fid
set imh [image create photo]
$imh put $data
label $chooser.step1 -image $imh

set vc [Rappture::VideoChooser $chooser.vc -variable ::previewVar]
#$vc load [glob "/home/derrick/projects/piv/video/*.mp4"]
$vc load [glob "/apps/video/*.mp4"]

pack $chooser.step1 -side top -anchor w -pady 8
pack $vc -side bottom -anchor center



# movie previewer

set preview [frame $f.preview_f]

set fid [open [file join $installdir images step2.png] r]
fconfigure $fid -translation binary -encoding binary
set data [read $fid]
close $fid
set imh [image create photo]
$imh put $data
label $preview.step2 -image $imh

set vp [Rappture::VideoPreview $preview.preview -variable ::previewVar]

pack $preview.step2 -side top -anchor w -pady 8
pack $vp -side bottom -anchor center



# analyze button gets us to the analyze page

set analyze [frame $f.analyze_f]

set fid [open [file join $installdir images step3.png] r]
fconfigure $fid -translation binary -encoding binary
set data [read $fid]
close $fid
set imh [image create photo]
$imh put $data
label $analyze.step3 -image $imh

button $analyze.go \
    -text "Analyze" \
    -command {
        $vp video stop
        $vs load file $previewVar
        $vs video seek [$vp query framenum]
        $nb current next>
        # FIXME: video loaded into memory twice
    }
pack $analyze.step3 -side left -anchor w
pack $analyze.go -anchor center


set reqwidth [expr round($width/2.0)]
blt::table $f \
    0,0 $intro -rowspan 3 -fill both -reqwidth $reqwidth\
    0,1 $div -rowspan 3 -fill y -pady 8 -padx 8\
    0,2 $chooser -fill x -padx {0 8}\
    1,2 $preview -fill x \
    2,2 $analyze -fill x

blt::table configure $f c0 -resize none
blt::table configure $f c1 -resize none
blt::table configure $f c2 -resize none
# blt::table configure $f r0 -pady 1

# ------------------------------------------------------------------
# VIDEO PAGE
# ------------------------------------------------------------------

set f [$nb insert end video]
set vs [Rappture::VideoScreen $f.viewer -fileopen {
            $vs video stop
            $nb current about}]
pack $vs -expand yes -fill both


# ------------------------------------------------------------------
# SHOW WINDOW
# ------------------------------------------------------------------


$nb current about
wm geometry . ${width}x${height}
wm deiconify .

#update idletasks
#image delete $imh
