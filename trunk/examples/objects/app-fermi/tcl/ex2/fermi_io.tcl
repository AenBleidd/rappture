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

    # Most simple xy plots for output
    # Because it is a single plot, it gets its own view.
    # The plot is placed in the position 1,1 of the view.
    set plot [Rappture::Plot "fdfPlot"]
    $plot xaxis "Fermi-Dirac Factor" \
        "Plot of Fermi-Dirac Calculation"
    $plot yaxis "Energy"
        "Energy cooresponding to each fdf" \
        -hints {"units=eV"}

    # Declaring a second plot creates a new view.
    # The new plot will be placed in the position 1,1 of its view.
    # User can do simple plot grouping using the add function
    # in the science code.
    Rappture::Plot "fdfPlot2" \
        -xlabel "Fermi-Dirac Factor"\
        -xdesc "Plot of Fermi-Dirac Calculation" \
        -yaxis "Energy" \
        -ydesc "Energy cooresponding to each fdf" \
        -yhints {"units=eV"}
}
