% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2005  Purdue Research Foundation
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

%op = sprintf('%s:%s:%s', path, 'src', 'src/matlab//');
%op = path(op);

% before you run this script you need to manually start up matlab and
% add the src and src/matlab directories to your path
% 
% or do this:
%
% cd test
% matlab -nodisplay -r path\(path,\'src\'\),infile=\'rplib_test.xml\',RpMatlab_test

%path(path,src');
%path(path,'src/matlab');

lib = rpLib(infile);
err = test_element(lib,'input.number(min)');
err = test_element_comp(lib,'input.number(min)');
err = test_children(lib,'input');
err = test_children_bytype(lib,'input','number');
err = test_convert('1m','cm',0);
err = test_convert('1m','cm',1);
err = test_convert('1eV','J',1);
err = test_convert_dbl('1eV','J');
err = test_convert_str('1eV','J',0);
err = test_convert_str('1eV','J',1);
err = test_define_unit('We',0);
err = test_define_unit('We2',0);
err = test_find('We');
err = test_get(lib,'output.curve(result).about.label');
err = test_get_string(lib,'output.curve(result).about.label');
err = test_get_basis('cm');
err = test_get_basis('We');
err = test_get_double(lib,'input.number(max).current');
err = test_get_exponent('cm');
err = test_get_exponent('We2');
err = test_get_units('We2');
err = test_get_units_name('We2');
err = test_make_metric('We2');
err = test_node_comp(lib,'input.number(min)');
%err = test_node_comp(0,'input.nur(min)');
err = test_node_id(lib,'input.number(min)');
err = test_node_type(lib,'input.number(min)');
err = test_put(lib,'output.curve(result).xy(f12)','12 13',' ',1);
err = test_put_dbl(lib,'output.curve(result).xy(f13)',22,1);
err = test_put_dbl(lib,'output.curve(result).xy(f14)',0,1);
err = test_put_str(lib,'output.curve(result).xy(f15)','110 111',1);
err = test_xml(lib);
err = test_result(lib);

quit;
