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
    # Rappture::Number are assumed to be an input
    Rappture::Number "temperature" "K" 300
    Rappture::Number "Ef" "eV" -5.5

    # describe the outputs
    # declare a table to store the outputs
    # table is named "factorsTable".
    # two new columns are added to the
    # table "factorsTable". the first column is named
    # "Fermi-Dirac Factor", a description is provided,
    # and it has no units. the second column is named
    # "Energy", a description is provided, and has units
    # of electron Volts
    # Rappture::Table is assumed to be an output
    set results [Rappture::Table "factorsTable"]
    $results column "Fermi-Dirac Factor" \
                    "Plot of Fermi-Dirac Calculation"
    $results column "Energy" \
                    "Energy cooresponding to each fdf" \
                    -hints {"units=eV"}

    # describe how the outputs shoud be displayed
    # initialize a view named "fdfView", with a 1x1 layout.
    # next, add a plot named "fdfPlot" to the view "fdfView".
    # populate the plot with data from the table
    # "factorsTable". Plot the column named
    # "Fermi-Dirac Factor" vs the column named "Energy".
    # place the plot, named fdfPlot, at position 1,1 within
    # the view's layout.
    # Rappture::View is assumed to be an output
    set v [Rappture::View "fdfView" -rows 1 -cols 1]
    $v plot "fdfPlot" -at "1,1" -table "factorsTable" \
        {"Fermi-Dirac Factor" "Energy" "g:o"}
}
