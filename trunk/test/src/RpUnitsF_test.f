c ----------------------------------------------------------------------
c  TEST: Fortran's interface to RpUnits.
c
c  Basic units conversion tests for the RpUnits portion of Rappture
c  written in Fortran.
c ======================================================================
c  AUTHOR:  Derrick Kearney, Purdue University
c  Copyright (c) 2004-2005
c  Purdue Research Foundation, West Lafayette, IN
c ======================================================================
      program units_test
        IMPLICIT NONE

        integer rp_units_convert_str, rp_units_add_presets
        integer rp_units_convert_dbl

        double precision dblVal
        character*40 retStr
        integer retVal

        retVal = rp_units_add_presets("all")

        retVal = rp_units_convert_str("72F","C",retStr)
        print *,"72F = ",retStr
        print *,"retVal = ",retVal

        retVal = rp_units_convert_dbl("72F","C",dblVal)
        print *,"72F = ",dblVal, " (no units)"
        print *,"retVal = ",retVal
        
      end program units_test

