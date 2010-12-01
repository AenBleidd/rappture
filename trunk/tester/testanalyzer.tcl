# Provides a "display" method to the analyzer widget, which allows you to pass
# in a xml file, as opposed to the load method which only accepts a library
# object.  What I really need here is the analyze page of the analyzer's
# notebook as a separate widget.

package require Itk

namespace eval Rappture::Tester::TestAnalyzer { # forward declaration}

option add *TestAnalyzer.width 3.5i widgetDefault
option add *TestAnalyzer.height 4i widgetDefault
option add *TestAnalyzer.simControl "auto" widgetDefault
option add *TestAnalyzer.simControlBackground "" widgetDefault
option add *TestAnalyzer.simControlOutline gray widgetDefault
option add *TestAnalyzer.simControlActiveBackground #ffffcc widgetDefault
option add *TestAnalyzer.simControlActiveOutline black widgetDefault
option add *TestAnalyzer.notebookpage "about" widgetDefault

option add *TestAnalyzer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *TestAnalyzer.codeFont \
    -*-courier-medium-r-normal-*-12-* widgetDefault
option add *TestAnalyzer.textFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *TestAnalyzer.boldTextFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

itcl::class Rappture::Tester::TestAnalyzer {
    inherit Rappture::Analyzer

    constructor {tool args} {
        Rappture::Analyzer::constructor $tool \
            -simcontrol off -notebookpage analyze
    } {
        # defined later
    }

    public method display {xmlobj}
}

itk::usual TestAnalyzer {
    keep -background -cursor -foreground -font
}

itcl::body Rappture::Tester::TestAnalyzer::constructor {tool args} {
    eval itk_initialize $args
}

itcl::body Rappture::Tester::TestAnalyzer::display {runfile} {
    clear
    load [Rappture::library $runfile]
    $itk_component(notebook) current analyze
}
