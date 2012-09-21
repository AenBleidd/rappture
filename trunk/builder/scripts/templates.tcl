# ----------------------------------------------------------------------
#  COMPONENT: templates
#
#  This file contains routines to parse language definition files in
#  the "templates" directory.  This defines all of the programming
#  languages that the Builder can build main programs for.  Each
#  language is specified as follows:
#
#    language NAME {
#        main { ...template... }
#        input pattern { script }
#        output pattern { script }
#    }
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require RapptureBuilder

namespace eval RapptureBuilder::templates {
    #
    # Set up a safe interpreter for loading language defn files...
    #
    variable tmplParser [interp create -safe]
    foreach cmd [$tmplParser eval {info commands}] {
        $tmplParser hide $cmd
    }
    $tmplParser alias language RapptureBuilder::templates::parse_language
    $tmplParser alias unknown RapptureBuilder::templates::parse_tmpl_unknown
    proc ::RapptureBuilder::templates::parse_tmpl_unknown {args} {
        error "bad command \"[lindex $args 0]\": should be language"
    }

    #
    # Set up a safe interpreter for loading the language definition...
    #
    variable langParser [interp create -safe]
    foreach cmd [$langParser eval {info commands}] {
        $langParser hide $cmd
    }
    $langParser alias extension RapptureBuilder::templates::parse_template extension
    $langParser alias command RapptureBuilder::templates::parse_template command
    $langParser alias makefile RapptureBuilder::templates::parse_template makefile

    $langParser alias main RapptureBuilder::templates::parse_template main
    $langParser alias input RapptureBuilder::templates::parse_pattern input
    $langParser alias output RapptureBuilder::templates::parse_pattern output

    $langParser alias unknown RapptureBuilder::templates::parse_lang_unknown
    proc ::RapptureBuilder::templates::parse_lang_unknown {args} {
        error "bad option \"[lindex $args 0]\": should be main, input, output"
    }

    # this variable will hold LangDef object as it is being built
    variable currLangDef ""
}

# ----------------------------------------------------------------------
# USAGE: RapptureBuilder::templates::init
#
# Called at the beginning of the Rappture builder to initialize the
# template language system.  Loads all languages in the "templates"
# directory within the builder to define all known languages.  After
# this, the main program can be built by calling templates::generate.
# ----------------------------------------------------------------------
proc RapptureBuilder::templates::init {} {
    # load supporting type definitions
    set dir [file join $RapptureBuilder::library scripts templates]
    foreach fname [glob [file join $dir *.tl]] {
        RapptureBuilder::templates::load $fname
    }

    # if anyone tries to load again, do nothing
    proc ::RapptureBuilder::templates::init {} { # already loaded }
}

# ----------------------------------------------------------------------
# USAGE: RapptureBuilder::templates::load <fileName>
#
# Used internally to load language definition files when this package
# is first initialized by the builder.  Processes the <fileName>
# within the template parser.  As a side-effect of executing this
# file, a new template language is defined.
# ----------------------------------------------------------------------
proc RapptureBuilder::templates::load {fname} {
    variable tmplParser
    variable langDefs

    set fid [open $fname r]
    set info [read $fid]
    close $fid

    if {[catch {$tmplParser eval $info} err] != 0} {
        error $err "$err\n    (while loading object definition from file \"$fname\")"
    }
}

# ----------------------------------------------------------------------
# USAGE: RapptureBuilder::templates::languages
#
# Returns a list of languages with templates loaded by the "init"
# procedure.  These names can be passed in to "generate" to generate
# code for these languages.
# ----------------------------------------------------------------------
proc RapptureBuilder::templates::languages {} {
    variable langDefs
    return [array names langDefs]
}

# ----------------------------------------------------------------------
# USAGE: RapptureBuilder::templates::generate <what> \
#           ?-language name? ?-xmlobj obj? ?-macros <string-map>?
#
# Builds a string of generated information starting with <what> as
# the main template.  This string may contain other @@NAME@@ fields,
# which are substituted recursively until all substitutions have
# been made.  The -language option specifies which language template
# should be used.  The -xmlobj option gives data for the @@NAME@@
# substitutions.
#
# Returns the generated string with all possible substitutions.
# ----------------------------------------------------------------------
proc RapptureBuilder::templates::generate {what args} {
    variable langDefs

    Rappture::getopts args params {
        value -language ""
        value -xmlobj ""
        value -macros ""
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"generate what ?-language name? ?-xmlobj obj?\""
    }
    if {$params(-language) eq ""} {
        error "must specify a language for the code to be generated"
    }

    set lang $params(-language)
    if {![info exists langDefs($lang)]} {
        error "bad -language \"$lang\": should be one of [join [lsort [array names langDefs]] {, }]"
    }

    set info [$langDefs($lang) template $what]
    if {$info eq ""} {
        error "no such template \"$what\""
    }

    set xmlobj $params(-xmlobj)

    # produce the @@INPUTS@@ section...
    set inputs ""
    if {$xmlobj ne ""} {
        foreach path [Rappture::entities -as path $xmlobj input] {
            append inputs [$langDefs($lang) generate $xmlobj $path -code]
        }
    }

    # produce the @@OUTPUTS@@ section...
    set outputs ""
    if {$xmlobj ne ""} {
        foreach path [Rappture::entities -as path $xmlobj output] {
            append outputs [$langDefs($lang) generate $xmlobj $path -code]
        }
    }

    # produce the @@DECLARATIONS@@ section...
    set decls ""
    if {$xmlobj ne ""} {
        foreach path [Rappture::entities -as path $xmlobj input] {
            append decls [$langDefs($lang) generate $xmlobj $path -decl]
        }
        foreach path [Rappture::entities -as path $xmlobj output] {
            append decls [$langDefs($lang) generate $xmlobj $path -decl]
        }
    }

    # handle the @@INPUTS@@ / @@OUTPUTS@@ / @@DECLARATIONS@@ specially
    # preserve the indent at the beginning of each line
    foreach {macro var} {
        @@INPUTS@@ inputs
        @@OUTPUTS@@ outputs
        @@DECLARATIONS@@ decls
    } {
        set re [format {(?:^|\n)([ \t]*)%s} $macro]
        while {[regexp -indices $re $info match indent]} {
            set code [set $var]
            foreach {m0 m1} $match break
            foreach {i0 i1} $indent break
            if {$i1-$i0 > 0} {
                set indent [string range $info $i0 $i1]
                set newcode ""
                foreach line [split $code \n] {
                    append newcode $indent $line "\n"
                }
                set code [string trimright $newcode \n]
            }
            if {[string index $info $m0] eq "\n"} { set code "\n$code" }
            set info [string replace $info $m0 $m1 $code]
        }
    }

    # make substitutions for any remaining macros
    set info [string map $params(-macros) $info]

    # return the generated code
    return $info
}

# ----------------------------------------------------------------------
# PARSER:  RapptureBuilder::templates::parse_language
#
# Used internally to parse the definition of a language file:
#
#   language NAME {
#       main { ...body of code... }
#       inputs { template }
#       outputs { template }
#   }
#
# Builds an object in currLangDef and then registers the completed
# object in the langDefs array.  This defines a known language that
# can be used to generate a template.
# ----------------------------------------------------------------------
proc RapptureBuilder::templates::parse_language {name body} {
    variable currLangDef
    variable langDefs
    variable langParser

    set currLangDef [RapptureBuilder::templates::LangDef ::#auto $name]

    if {[catch {$langParser eval $body} err] != 0} {
        itcl::delete object $currLangDef
        set currLangDef ""
        error $err "\n    (while loading language definition for \"$name\")"
    }

    set langDefs($name) $currLangDef
    set currLangDef ""
}

# ----------------------------------------------------------------------
# PARSER:  RapptureBuilder::templates::parse_template <what> <body>
#
# Used internally to add the definition of program templates to the
# current language definition:
#
#   main { ...template... }
#
# ----------------------------------------------------------------------
proc RapptureBuilder::templates::parse_template {what body} {
    variable currLangDef
    $currLangDef template $what $body
}

# ----------------------------------------------------------------------
# PARSER:  RapptureBuilder::templates::parse_pattern <which> <pattern> <script>
#
# Used internally to add the definition of how to input/output
# specific objects to the current language definition:
#
#   input number {...script...}
#   output * {...script...}
#
# Each script is executed in a context that has commands and built-in
# variables for various bits of the current object:
#   [attr get name] ... returns the value of attribute name
#   $path ............. path for current object
#   $id ............... id (name at end of path) for current object
# ----------------------------------------------------------------------
proc RapptureBuilder::templates::parse_pattern {which pattern body} {
    variable currLangDef
    $currLangDef handler $which:$pattern $body
}


# ----------------------------------------------------------------------
#  CLASS: LangDef
# ----------------------------------------------------------------------
itcl::class RapptureBuilder::templates::LangDef {
    constructor {name args} {
        set _lang $name
        eval configure $args
    }

    # used to query the name of this language
    public method language {} {
        return $_lang
    }

    # used to register/query a template with a specific name
    public method template {what args} {
        if {[llength $args] == 0} {
            if {[info exists _frag2txt($what)]} {
                return $_frag2txt($what)
            }
            return ""
        } elseif {[llength $args] != 1} {
            error "wrong # args: should be \"template what ?text?\""
        }
        set _frag2txt($what) [lindex $args 0]
    }

    # used to register a script that processes an input/output element
    public method handler {pattern body} {
        set i [lsearch -exact $_patterns $pattern]
        if {$i < 0} { lappend _patterns $pattern }
        set _pat2code($pattern) $body
    }

    # used to generate the output for a specific Rappture object in this lang
    public method generate {xmlobj path {what "-code"}} {
        variable genParser
        variable genOutput ""
        variable genDecls ""
        variable genXmlobj $xmlobj
        variable genPath $path

        set which [lindex [split $path .] 0]
        set type [$xmlobj element -as type $path]
        set elem $which:$type

        set code ""
        foreach pat $_patterns {
            if {[string match $pat $elem]} {
                set code $_pat2code($pat)
                break
            }
        }
        if {$code eq ""} {
            error "can't find production code for $elem"
        }

        $genParser eval [list set path $path]
        $genParser eval [list set type $type]
        $genParser eval [list set id [$xmlobj element -as id $path]]
        $genParser eval $code

        switch -- $what {
            -code   { return $genOutput }
            -decl   { return $genDecls }
            default { error "bad option \"$what\": should be -code, -decl" }
        }
    }

    private variable _lang ""     ;# name of this language
    private variable _frag2txt    ;# maps fragment name => template text
    private variable _patterns "" ;# order of patterns for _pat2code
    private variable _pat2code    ;# maps glob pattern => script to gen output

    #
    # Set up a safe interpreter for the "generate" method...
    #
    private common genParser [interp create -safe]
    $genParser alias code RapptureBuilder::templates::LangDef::cmd_code
    $genParser alias decl RapptureBuilder::templates::LangDef::cmd_decl
    $genParser alias attr RapptureBuilder::templates::LangDef::cmd_attr

    private common genOutput ""  ;# gathers code output from genParser
    private common genDecls ""   ;# gathers declarations from genParser
    private common genXmlobj ""  ;# Rappture tool spec
    private common genPath ""    ;# current path in the genXmlobj data

    # like the std "puts" command, but adds output to the generated code
    proc cmd_code {args} {
        set nl "\n"
        while {[llength $args] > 1} {
            set opt [lindex $args 0]
            set args [lrange $args 1 end]
            if {$opt eq "-nonewline"} {
                set nl ""
            } else {
                error "bad option \"$opt\": should be -nonewline"
            }
        }
        if {[llength $args] != 1} {
            error "wrong # args: should be \"puts ?-nonewline? string\""
        }
        append genOutput [lindex $args 0] $nl
    }

    # like the std "puts" command, but adds output to code declarations
    proc cmd_decl {args} {
        set nl "\n"
        while {[llength $args] > 1} {
            set opt [lindex $args 0]
            set args [lrange $args 1 end]
            if {$opt eq "-nonewline"} {
                set nl ""
            } else {
                error "bad option \"$opt\": should be -nonewline"
            }
        }
        if {[llength $args] != 1} {
            error "wrong # args: should be \"puts ?-nonewline? string\""
        }
        append genDecls [lindex $args 0] $nl
    }

    # used to query attribute info for the current object
    proc cmd_attr {option name} {
        set type [$genXmlobj element -as type $genPath]
        set ainfo [Rappture::objects::get $type -attributes]
        if {$ainfo eq ""} {
            error "don't know how to generate code for type \"$type\""
        }

        set found 0
        foreach rec $ainfo {
            if {[lindex $rec 0] eq $name} {
                set found 1
                break
            }
        }
        if {!$found} {
            error "bad attribute name \"$name\" for type \"$type\""
        }

        switch -- $option {
            get {
                array set attr [lrange $rec 1 end]
                return [$genXmlobj get $genPath.$attr(-path)]
            }
            info {
                return $rec
            }
            default {
                error "bad option \"$option\": should be get, info"
            }
        }
    }
}
