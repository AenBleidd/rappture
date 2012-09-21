c ----------------------------------------------------------------------
c  TEST: Fortran's interface to RpUnits.
c
c  Basic units conversion tests for the RpUnits portion of Rappture
c  written in Fortran.
c ======================================================================
c  AUTHOR:  Derrick Kearney, Purdue University
c  Copyright (c) 2004-2012  HUBzero Foundation, LLC
c
c  See the file "license.terms" for information on usage and
c  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
c ======================================================================
      program units_test
        IMPLICIT NONE

        integer rp_units_convert_str
        integer rp_units_convert_dbl

        double precision dblVal
        character*40 retStr
        integer retVal

        retVal = rp_units_convert_str("72F","C",retStr)
        print *,"72F = ",retStr
        print *,"correct retVal = 22.222C",retVal
        print *,"retVal = ",retVal

        retVal = rp_units_convert_dbl("72F","C",dblVal)
        print *,"72F = ",dblVal, " (no units)"
        print *,"correct retVal = 22.222",retVal
        print *,"retVal = ",retVal

      end program units_test

