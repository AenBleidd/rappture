def fermi_io():

    # assume global interface variable.
    # newly created objects are stored in the
    # global interface variable. if there is more
    # than one global interface variable, the
    # interface that should be used is specified
    # using the Rp_???InitX version of the funtion
    # where ??? represents the type of object the
    # user is trying to create.

    # describe the inputs
    # declare a number named "temperature",
    # with units of Kelvin, default value 300,
    #
    # the next number is named "Ef", has units of
    # electron Volts, default value of -5.5
    #
    # Rappture::Number is assumed to be an input

    Rappture.Number(name="temperature",units="K",default=300)
    Rappture.Number(name="Ef",units="eV",default=-5.5)

    # describe the outputs
    # two new columns are added to the default table
    # the first column is named "Fermi-Dirac Factor",
    # a description is provided, and it has no units.
    # the second column is named "Energy", a description
    # is provided, and has units of electron Volts.
    # Rappture::Table is assumed to be an output
    x1 = Rappture.Table.column(name="Fermi-Dirac Factor",
            desc="Plot of Fermi-Dirac Calculation")
    y1 = Rappture.Table.column(name="Energy",
            desc="Energy cooresponding to each fdf",
            hints=["units=eV"])
    x2 = results.column(name="Fermi-Dirac Factor * 2",
            desc="Plot of Fermi-Dirac Calculation multiplied by 2")
    y2 = results.column(name="Energy * 2",
            desc="Energy cooresponding to each fdf multiplied by 2",
            hints=["units=eV"])


    # describe how the outputs shoud be displayed
    # initialize a view named "fdfView", with a 1x1 layout.
    # next, add a plot named "fdfPlot" to the view fdfView".
    # populate the plot with data from the table
    # "factorsTable". Plot the column named
    # "Fermi-Dirac Factor" vs the column named "Energy".
    # place the plot, named fdfPlot, at position 1x1 (by default)
    # within the view's layout.
    # Rappture.View is assumed to be an output
    # v.plot(xdata, ydata, format, at, name)
    v = Rappture.View(name="fdfView")
    v.plot(x1, y1, "g:o", [1,1], "fdfPlot")

    # two stacked plots in the same view
    v2 = Rappture.View(name="fdfView2")
    v2.plot(x1, y1, "g:o", [1,1], "fdfPlot2")
    v2.plot(x2, y2, "b-o", [2,1], "fdfPlot3")

    # two side by side plots in the same view
    v3 = Rappture.View(name="fdfView3")
    v3.plot(x1, y1, "g:o", [1,1], "fdfPlot4")
    v3.plot(x2, y2, "b-o", [1,2], "fdfPlot5")

    # grouped curves in one plot in a view
    v4 = Rappture.View(name="fdfView1")
    v4.plot(x1, y1, "g:o", [1,1], "fdfPlot6")
    v4.plot(x2, y2, "b-o", [1,1], "fdfPlot7")
