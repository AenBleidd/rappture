% ----------------------------------------------------------------------
%  EXAMPLE: Fermi-Dirac function in Matlab.
%
%  This script represents a legacy application that will be left
%  intact, but wrapped with a Rappture interface.
% ======================================================================
%  AUTHOR:  Michael McLennan, Purdue University
%  Copyright (c) 2004-2005
%  Purdue Research Foundation, West Lafayette, IN
% ======================================================================
disp('Enter the temperature (K):');
T = input(' T = ');

disp('Enter the Fermi level (eV):');
Ef = input(' Ef = ');

kT = 8.61734e-5 * T;
Emin = Ef - 10*kT;
Emax = Ef + 10*kT;

E = linspace(Emin,Emax,200);
f = 1.0 ./ (1.0 + exp((E - Ef)/kT));

fid = fopen('out.dat','w');
fprintf(fid,'FERMI-DIRAC FUNCTION F1/2\n\n');
fprintf(fid,'Energy (eV)   f1/2\n');
fprintf(fid,'------------  ------------\n');
for j=1:200
  fprintf(fid,'%12g  %12g\n', E(j), f(j));
end
fclose(fid);
