function fermi_io()

    % assume global interface variable.
    % newly created objects are stored in the
    % global interface variable. if there is more
    % than one global interface variable, the
    % interface that should be used is specified
    % using the Rp_???InitX version of the funtion
    % where ??? represents the type of object the
    % user is trying to create.

    % describe the inputs
    % declare a number named 'temperature',
    % with units of Kelvin, default value 300,
    % and specify that it is only an input
    % the next number is named 'Ef', has units of
    % electron Volts, default value of -5.5 and
    % is also only an input value
    % RP_VAR_INPUT is a matlab function that returns
    % a constant value
    Rp_NumberInit('temperature','K',300,RP_VAR_INPUT);
    Rp_NumberInit('Ef','eV',-5.5,RP_VAR_INPUT);

    % describe the outputs
    % declare a table to store the outputs
    % table is named 'factorsTable' and is declared as
    % an output only object. two new columns are added to the
    % table "factorsTable". the first column is named
    % "Fermi-Dirac Factor", a description is provided,
    % and it has no units. the second column is named
    % "Energy", a description is provided, and has units
    % of electron Volts
    % RP_VAR_OUTPUT is a matlab function that returns
    % a constant value
    result = Rp_TableInit('factorsTable',RP_VAR_OUTPUT);
    Rp_TableNewColumn(result,'Fermi-Dirac Factor',
                      'Plot of Fermi-Dirac Calculation');
    Rp_TableNewColumn(result,'Energy',
                      'Energy cooresponding to each fdf',
                      'units=eV');

    % describe how the outputs shoud be displayed
    % initialize a view named "fdfView", specifying it
    % as an output only object with a 1x1 view layout.
    % next, add a plot named "fdfPlot" to the view fdfView".
    % populate the plot with data from the table
    % "fermifactors". Plot the column named
    % "Fermi-Dirac Factor" vs the column named "Energy".
    % place the plot, named fdfPlot, at position 1x1 within
    % the view's layout.
    % RP_VIEW_1X1 is a matlab function that returns
    % a constant value
    v = Rp_ViewInit('fdfView',RP_VAR_OUTPUT|RP_VIEW_1X1);
    Rp_ViewAddPlotFromTable(v,'fdfPlot','factorsTable',
                            'Fermi-Dirac Factor',
                            'Energy','position=1x1');

    % add another curve to the 'fdfPlot' plot.
    % plot the column named 'Fermi-Dirac Factor2' vs
    % the column named 'Energy'
    % Rp_ViewAddPlotFromTable(v,'fdfPlot','factorsTable',
    %                         'Fermi-Dirac Factor2',
    %                         'Energy','position=1x1');
}
