c ----------------------------------------------------------------------
c  TEST: Fortran Rappture Library Test Source.
c
c  Simple tests of the Rappture Library Fortran Bindings
c
c ======================================================================
c  AUTHOR:  Derrick S. Kearney, Purdue University
c  Copyright (c) 2004-2005  Purdue Research Foundation
c
c  See the file "license.terms" for information on usage and
c  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
c ======================================================================
      SUBROUTINE  test_element(lib,path)
        integer rp_lib_element_obj
        integer retVal, lib
        character*100 typ, comp, id, path

        print *,"TESTING ELEMENT: path = ",path

        retVal = rp_lib_element_obj(lib, path)

        print *,"dict key = ",retVal

        ! what happens when you get details for lib?
        call rp_lib_node_comp(retVal,comp)
        call rp_lib_node_type(retVal,typ)
        call rp_lib_node_id(retVal,id)

        print *,"comp = ",comp
        print *,"type = ",typ
        print *,"id = ",id
      END SUBROUTINE  test_element

      SUBROUTINE  test_get_str(lib,path)
        integer lib
        character*100 path, retText

        print *,"TESTING GET: path = ",path

        call rp_lib_get(lib, path, retText)

        print *,"retText = ",retText
      END SUBROUTINE  test_get_str

      SUBROUTINE  test_get_dbl(lib,path)
        integer lib
        double precision rslt, rp_lib_get_double
        character*100 path, retText

        print *,"TESTING GET: path = ",path

        rslt = rp_lib_get_double(lib, path)

        print *,"rslt = ",rslt
      END SUBROUTINE  test_get_dbl

      program rplib_f_tests
        IMPLICIT NONE

        integer rp_lib, rp_units_convert_dbl, rp_units_add_presets
        integer rp_lib_element_obj

        integer driver, ok
        double precision T, Ef, kT, Emin, Emax, dE, f, E
        CHARACTER*100 inFile, strVal, path
        character*40 xy

        call getarg(1,inFile)
        driver = rp_lib(inFile)
        ! print *,"dict key = ",driver

        ok = rp_units_add_presets("all")

        ! TESTING ELEMENT
        !call test_element(driver, "input.number(min)")
        path = "input.number(min)"
        call test_element(driver, path)
        !call rp_lib_get(driver, path, strVal)
        !print *,"strVal = ",strVal

        ! TESTING GET STRING
        path = "input.number(min).current"
        call test_get_str(driver, path)

        ! TESTING GET DOUBLE
        path = "input.number(min).current"
        call test_get_dbl(driver, path)


        call rp_result(driver)
      end program rplib_f_tests

!        call rp_lib_get(driver,
!     +        "input.number(min).current", strVal)
!        ok = rp_units_convert_dbl(strVal,"K",T)
!
!        call rp_lib_get(driver,
!     +        "input.number(Ef).current", strVal)
!        ok = rp_units_convert_dbl(strVal,"K",Ef)
!
!        kT = 8.61734e-5 * T
!        Emin = Ef - 10*kT
!        Emax = Ef + 10*kT
!
!        dE = 0.005*(Emax - Emin)
!
!        do 10 E=Emin,Emax,dE
!          f = 1.0/(1.0+exp((E-Ef)/kT))
!          write(xy,'(E20.12,F13.9,A)') f, E, char(10)
!          call rp_lib_put_str(driver,
!     +        "output.curve(f12).component.xy", xy, 1)
! 10     continue
!
!      end program rplib_f_tests
