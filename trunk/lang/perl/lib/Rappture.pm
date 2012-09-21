package Rappture;

use 5.008;
use strict;
use warnings;

require Exporter;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Rappture ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.01';

require XSLoader;
XSLoader::load('Rappture', $VERSION);

# Preloaded methods go here.

1;
__END__

=head1 NAME

Rappture - Rappture/Perl API, Bindings to the Rappture Development Toolkit.

=head1 SYNOPSIS

  use Rappture;

  # open the XML file containing the run parameters

  $driver = Rappture::RpLibrary->new($ARGV[0]);

  $Tstr = $driver->get("input.(temperature).current");
  $T = Rappture::RpUnits::convert($Tstr, "K", "off");

  $Efstr = $driver->get("input.(Ef).current");
  $Ef = Rappture::RpUnits::convert($Efstr, "eV", "off");

  $kT = 8.61734e-5 * $T;
  $Emin = $Ef - 10 * $kT;
  $Emax = $Ef + 10 * $kT;

  $E = $Emin;
  $dE = 0.005*($Emax - $Emin);

  while( $E < $Emax ) {
    $f = 1.0 / ( 1.0 + exp(($E - $Ef) / $kT));
    $driver->put("output.curve(f12).component.xy", "$f $E\n", 1);
    $E = $E + $dE;
  }

  $driver->result(); 

=head1 DESCRIPTION

This module provides an interface to the RAPPTURE (Rapid APPlication 
infrastrucTURE) Toolkit. 

=head2 EXPORT

None by default.



=head1 SEE ALSO

See 
http://developer.nanohub.org/projects/rappture
http://developer.nanohub.org/projects/rappture/wiki/Documentation
http://developer.nanohub.org/projects/rappture/wiki/rappture_perl_api

=head1 AUTHOR

Nicholas J. Kisseberth

=head1 COPYRIGHT AND LICENSE

Copyright (c) 2004-2012  HUBzero Foundation, LLC
All rights reserved.

Developed by:  Network for Computational Nanotechnology
               Purdue University, West Lafayette, Indiana
               http://www.rappture.org

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal with the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following
conditions:

    * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimers.

    * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers
        in the documentation and/or other materials provided with the
        distribution.

    * Neither the names of the Network for Computational Nanotechnology,
        Purdue University, nor the names of its contributors may be used
        to endorse or promote products derived from this Software without
        specific prior written permission. 

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.

=cut
