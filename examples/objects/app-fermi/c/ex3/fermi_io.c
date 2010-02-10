#include "rappture.h"

void fermi_io() {

    Rp_Table *result = NULL;
    Rp_TableColumn *x1 = NULL;
    Rp_TableColumn *y1 = NULL;
    Rp_TableColumn *x2 = NULL;
    Rp_TableColumn *y2 = NULL;
    Rp_View *v1 = NULL;
    Rp_View *v2 = NULL;
    Rp_View *v3 = NULL;

    // assume global interface variable.
    // newly created objects are stored in the
    // global interface variable. if there is more
    // than one global interface variable, the
    // interface that should be used is specified
    // using the Rp_???InitX version of the funtion
    // where ??? represents the type of object the
    // user is trying to create.

    // describe the inputs
    // declare a number named "temperature",
    // with units of Kelvin, default value 300,
    //
    // the next number is named "Ef", has units of
    // electron Volts, default value of -5.5
    //
    // Rp_Number is assumed to be an input
    Rp_NumberInit("temperature","K",300);
    Rp_NumberInit("Ef","eV",-5.5);

    // describe the outputs
    // declare a table to store the outputs
    // table is named "factorsTable".
    // two new columns are added to the default table
    // the first column is named "Fermi-Dirac Factor",
    // a description is provided, and it has no units.
    // the second column is named "Energy", a description
    // is provided, and has units of electron Volts
    x1 = Rp_TableColumnInit("Fermi-Dirac Factor",
            "Plot of Fermi-Dirac Calculation");
    y1 = Rp_TableColumnInit("Energy",
            "Energy cooresponding to each fdf");
    Rp_TableColumnHint(y1,"units","eV");

    x2 = Rp_TableColumnInit("Fermi-Dirac Factor * 2",
            "Plot of Fermi-Dirac Calculation multiplied by 2");
    y2 = Rp_TableColumnInit("Energy * 2",
            "Energy cooresponding to each fdf multiplied by 2");
    Rp_TableColumnHint(y2,"units","eV");

    // describe how the outputs should be displayed
    // initialize a view named "fdfView", with a 1x1 layout.
    // next, add a plot named "fdfPlot" to the view "fdfView".
    // populate the plot with data from the table
    // "factorsTable". Plot the column named
    // "Fermi-Dirac Factor" vs the column named "Energy".
    // place the plot, named fdfPlot, at position 1x1 (by default)
    // within the view's layout.
    // Rp_View is assumed to be an output
    v1 = Rp_ViewInit("fdfView");
    Rp_ViewPlot(v1,"fdfPlot",1,1,x1,y1,"g:o");

    // two stacked plots in the same view
    v2 = Rp_ViewInit("fdfView2");
    Rp_ViewPlot(v2,"fdfPlot2",1,1,x1,y1,"g:o");
    Rp_ViewPlot(v2,"fdfPlot3",2,1,x2,y2,"b-o");

    // two side by side plots in the same view
    v3 = Rp_ViewInit("fdfView3");
    Rp_ViewPlot(v3,"fdfPlot4",1,1,x1,y1,"g:o");
    Rp_ViewPlot(v3,"fdfPlot5",1,2,x2,y2,"b-o");

    // grouped curves in one plot in a view
    // plots hold same location in the view
    v4 = Rp_ViewInit("fdfView4");
    Rp_ViewPlot(v4,"fdfPlot6",1,1,x1,y1,"g:o");
    Rp_ViewPlot(v4,"fdfPlot7",1,1,x2,y2,"b-o");
}
