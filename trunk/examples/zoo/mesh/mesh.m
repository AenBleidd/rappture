
% Open an XML run file to write into
lib = rpLib(infile);

meshtype = rpLibGetString(lib, 'input.choice(mesh).current');
contour = rpLibGetString(lib, 'input.boolean(contour).current')
switch contour 
  case 'yes'
    view = 'contour';
  otherwise
    view = 'heightmap';
end

switch meshtype 
  case 'cloud' 
    mesh = 'output.mesh';
    rpLibPutString(lib, 'output.mesh.about.label', \
		   'cloud in unstructured mesh', 0);
    rpLibPutString(lib, 'output.mesh.dim', '2', 0);
    rpLibPutString(lib, 'output.mesh.units', 'm', 0);
    rpLibPutString(lib, 'output.mesh.hide', 'yes', 0);

    xv = linspace(0, 1, 50)
    for i = 1:50
      for j = 1:50
        string = sprintf('%12g  %12g\n', xv(i), xv(j));
	rpLibPutString(lib, 'output.mesh.unstructured.points', string, 1);
      endfor
    endfor
    rpLibPutString(lib, 'output.mesh.unstructured.celltypes', '', 0);

  case 'regular' 
    mesh = 'output.mesh';
    rpLibPutString(lib, 'output.mesh.about.label', 'uniform grid mesh', 0);
    rpLibPutString(lib, 'output.mesh.dim', '2', 0);
    rpLibPutString(lib, 'output.mesh.units', 'm', 0);
    rpLibPutString(lib, 'output.mesh.hide', 'yes', 0);

    % Specify the x-axis of the mesh
    rpLibPutString(lib, 'output.mesh.grid.xaxis.min', '0.0', 0);
    rpLibPutString(lib, 'output.mesh.grid.xaxis.max', '1.0', 0);
    rpLibPutString(lib, 'output.mesh.grid.xaxis.numpoints', '50', 0);
    
    % Specify the y-axis of the mesh
    rpLibPutString(lib, 'output.mesh.grid.yaxis.min', '0.0', 0);
    rpLibPutString(lib, 'output.mesh.grid.yaxis.max', '1.0', 0);
    rpLibPutString(lib, 'output.mesh.grid.yaxis.numpoints', '50', 0);

  case 'irregular' 
    mesh = 'output.mesh';
    rpLibPutString(lib, 'output.mesh.about.label', 'irregular grid mesh', 0);
    rpLibPutString(lib, 'output.mesh.dim', '2', 0);
    rpLibPutString(lib, 'output.mesh.units', 'm', 0);
    rpLibPutString(lib, 'output.mesh.hide', 'yes', 0);

    xv = linspace(0, 1, 50)
    string = sprintf('%12g\n', xv);
    rpLibPutString(lib, 'output.mesh.grid.xcoords', string, 0);
    rpLibPutString(lib, 'output.mesh.grid.ycoords', string, 0);

  case 'hybrid' 
    mesh = 'output.mesh';
    rpLibPutString(lib, 'output.mesh.about.label', \
		   'hybrid regular and irregular grid mesh', 0);
    rpLibPutString(lib, 'output.mesh.dim', '2', 0);
    rpLibPutString(lib, 'output.mesh.units', 'm', 0);
    rpLibPutString(lib, 'output.mesh.hide', 'yes', 0);

    xv = linspace(0, 1, 50)
    string = sprintf('%g\n', xv);
    rpLibPutString(lib, 'output.mesh.grid.xcoords', string, 0);

    % Specify the y-axis of the mesh
    rpLibPutString(lib, 'output.mesh.grid.yaxis.min', '0.0', 0);
    rpLibPutString(lib, 'output.mesh.grid.yaxis.max', '1.0', 0);
    rpLibPutString(lib, 'output.mesh.grid.yaxis.numpoints', '50', 0);

  case 'triangular'
    mesh = 'output.mesh';
    rpLibPutString(lib, 'output.mesh.about.label', \
		   'triangles in unstructured mesh', 0);
    rpLibPutString(lib, 'output.mesh.dim', '2', 0);
    rpLibPutString(lib, 'output.mesh.units', 'm', 0);
    rpLibPutString(lib, 'output.mesh.hide', 'yes', 0);

    f = fopen('points.txt');
    points = fscanf(f, '%g');
    fclose(f);
    string = sprintf('%g %g\n', points);
    rpLibPutString(lib, 'output.mesh.unstructured.points', string, 0);

    rpLibPutFile(lib, 'output.mesh.unstructured.triangles', 'triangles.txt', \
		 0, 0);

  case 'unstructured' 
    mesh = 'output.mesh';
    rpLibPutString(lib, 'output.mesh.about.label', 'Unstructured Grid', 0);
    rpLibPutString(lib, 'output.mesh.dim', '2', 0);
    rpLibPutString(lib, 'output.mesh.units', 'm', 0);
    rpLibPutString(lib, 'output.mesh.hide', 'yes', 0);
    
    f = fopen('points.txt');
    points = fscanf(f, '%g');
    fclose(f);
    string = sprintf('%g %g\n', points);
    rpLibPutString(lib, 'output.mesh.unstructured.points', string, 0);

    rpLibPutFile(lib, 'output.mesh.unstructured.cells', 'triangles.txt', 0, 0);
    rpLibPutString(lib, 'output.mesh.unstructured.celltypes', 'triangle', 0);

  case 'cells' 
    mesh = 'output.mesh';
    rpLibPutString(lib, 'output.mesh.about.label', \
		   'unstructured grid with heterogeneous cells', 0);
    rpLibPutString(lib, 'output.mesh.dim', '2', 0);
    rpLibPutString(lib, 'output.mesh.units', 'm', 0);
    rpLibPutString(lib, 'output.mesh.hide', 'yes', 0);

    f = fopen('points.txt');
    points = fscanf(f, '%g');
    fclose(f);
    string = sprintf('%g %g\n', points);
    rpLibPutString(lib, 'output.mesh.unstructured.points', string, 0);

    rpLibPutFile(lib, 'output.mesh.unstructured.cells', 'triangles.txt', 0, 0);

    % TO BE COMPLETED
    rpLibPutString(lib, 'output.mesh.unstructured.celltypes', 'triangle', 0);

  case 'vtkmesh'
    mesh = 'output.mesh';
    rpLibPutString(lib, 'output.mesh.about.label', 'vtk mesh', 0);
    rpLibPutString(lib, 'output.mesh.dim', '2', 0);
    rpLibPutString(lib, 'output.mesh.units', 'm', 0);
    rpLibPutString(lib, 'output.mesh.hide', 'yes', 0);

    rpLibPutFile(lib, 'output.mesh.vtk', 'mesh.vtk', 0, 0);

  case 'vtkfield' 

    rpLibPutString(lib, 'output.field(substrate).about.label', \
	       'Substrate Surface', 0);
    rpLibPutFile(lib, 'output.field(substrate).component.vtk', 'file.vtk', \
		 0, 0);
    rpLibPutString(lib, 'output.string.current', '', 0);
    rpLibResult(lib);
    quit;

  otherwise
    error 'unknown mesh type $meshtype'
end

% Read in the data since we're not simulating anything...
f = fopen('substrate_data.txt');
substrate_values = fscanf(f, '%g');
fclose(f);
f = fopen('particle_data.txt');
particle_values = fscanf(f, '%g');
fclose(f);

rpLibPutString(lib, 'output.field(substrate).about.label', \
	       'Substrate Surface', 0);
rpLibPutString(lib, 'output.field(substrate).component.mesh', mesh, 0);
string = sprintf('%12g\n', substrate_values);
rpLibPutString(lib, 'output.field(substrate).component.values', string, 0);
rpLibPutString(lib, 'output.field(substrate).about.view', view, 0);

rpLibPutString(lib, 'output.field(particle).about.label', \
	       'Particle Surface', 0);
rpLibPutString(lib, 'output.field(particle).component.mesh', mesh, 0);
string = sprintf('%12g\n', particle_values);
rpLibPutString(lib, 'output.field(particle).component.values', string, 0);
rpLibPutString(lib, 'output.field(particle).about.view', view, 0);

rpLibPutString(lib, 'output.string.about.label', 'Mesh XML definition', 0);
xml = rpLibXml(lib);
rpLibPutString(lib, 'output.string.current', xml, 0);

% save the updated XML describing the run...
rpLibResult(lib);
quit;
