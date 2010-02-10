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
    %
    % the next number is named 'Ef', has units of
    % electron Volts, default value of -5.5
    %
    % Rp_Number is assumed to be an input
    Rp_NumberInit('temperature','K',300);
    Rp_NumberInit('Ef','eV',-5.5);

    % Most simple xy plots fir output
    % Because it is a single plot, it gets its own view.
    % The plot is placed in the position 1,1 of the view.
    p1 = Rp_PlotInit('fdfPlot');
    Rp_PlotXAxis(p1,'Fermi-Dirac Factor',
        'Plot of Fermi-Dirac Calculation');
    Rp_PlotYAxis(p1,'Energy','Energy cooresponding to each fdf');
    Rp_PlotYAxisHint(p1,'units','eV');

    % Declaring a second plot creates a new view.
    % The new plot will be placed in the position 1,1 of its view.
    % User can do simple plot grouping using the add function
    % in the science code.
    p2 = Rp_PlotInit('fdfPlot2');
    Rp_PlotXAxis(p2,'Fermi-Dirac Factor',
        'Plot of Fermi-Dirac Calculation');
    Rp_PlotYAxis(p2,'Energy','Energy cooresponding to each fdf');
    Rp_PlotYAxisHint(p2,'units','eV');
}
