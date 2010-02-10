proc fermi_io {

    # assume global interface variable.
    # newly created objects are stored in the
    # global interface variable. if there is more
    # than one global interface variable, the
    # interface that should be used is specified
    # using an alternative version of the object constructor
    # that supports an interface argument.

    # describe the inputs
    # declare a number named "temperature",
    # with units of Kelvin, default value 300,
    #
    # the next number is named "Ef", has units of
    # electron Volts, default value of -5.5
    #
    # Rappture::Number is assumed to be an input
    Rappture::Number "temperature" "K" 300
    Rappture::Number "Ef" "eV" -5.5

    # describe the outputs
    # two new columns are added to the default table
    # the first column is named "Fermi-Dirac Factor",
    # a description is provided, and it has no units.
    # the second column is named "Energy", a description
    # is provided, and has units of electron Volts.
    # Rappture::Table is assumed to be an output
    set x1 [Rappture::Table::column "Fermi-Dirac Factor" \
            "Plot of Fermi-Dirac Calculation"]
    set y1 [Rappture::Table::column "Energy" \
            "Energy cooresponding to each fdf" \
            -hints {"units=eV"}]
    set x2 [Rappture::Table::column "Fermi-Dirac Factor * 2" \
            "Plot of Fermi-Dirac Calculation multiplied by 2"]
    set y2 [Rappture::Table::column "Energy * 2" \
            "Energy cooresponding to each fdf multiplied by 2" \
            -hints {"units=eV"}]

    # describe how the outputs should be displayed
    # initialize a view named "fdfView", with a 1x1 layout.
    # next, add a plot named "fdfPlot" to the view "fdfView".
    # populate the plot with data from the table
    # "factorsTable". Plot the column named
    # "Fermi-Dirac Factor" vs the column named "Energy".
    # place the plot, named fdfPlot, at position 1,1 (by default)
    # within the view's layout.
    # Rappture::View is assumed to be an output
    set v [Rappture::View "fdfView"]
    $v plot "fdfPlot" {$x1 $y1 "g:o"}

    # two stacked plots in the same view
    set v2 [Rappture::View "fdfView2"]
    $v2 plot "fdfPlot2" -at {1 1} {$x1 $y1 "g:o"}
    $v2 plot "fdfPlot3" -at {2 1} {$x2 $y2 "b-o"}

    # two side by side plots in the same view
    set v3 [Rappture::View "fdfView3"]
    $v3 plot "fdfPlot4" -at {1 1} {$x1 $y1 "g:o"}
    $v3 plot "fdfPlot5" -at {1 2} {$x2 $y2 "b-o"}

    # grouped curves in one plot in a view
    set v4 [Rappture::View "fdfView4"]
    $v4 plot "fdfPlot6" -at {1 1} {$x1 $y1 "g:o"}
    $v4 plot "fdfPlot7" -at {1 1} {$x2 $y2 "b-o"}

    # grouped curves in one plot in a view
    set v5 [Rappture::View "fdfView5"]
    $v5 plot "fdfPlot8" {$x1 $y1 "g:o" $x2 $y2 "b-o"}
