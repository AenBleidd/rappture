c ----------------------------------------------------------------------
c  TEST: Fortran Rappture Library Test Source.
c
c  Simple tests of the Rappture Library Fortran Bindings
c
c ======================================================================
c  AUTHOR:  Derrick S. Kearney, Purdue University
c  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

      SUBROUTINE  test_get(lib,path)
        integer lib
        character*100 path, retText

        print *,"TESTING GET       : path = ",path

        call rp_lib_get(lib, path, retText)

        print *,"retText = ",retText
      END SUBROUTINE  test_get

      SUBROUTINE  test_get_str(lib,path)
        integer lib
        character*100 path, retText

        print *,"TESTING GET STRING: path = ",path

        call rp_lib_get_str(lib, path, retText)

        print *,"retText = ",retText
      END SUBROUTINE  test_get_str

      SUBROUTINE  test_get_dbl(lib,path)
        integer lib
        double precision rslt, rp_lib_get_double
        character*100 path

        print *,"TESTING GET DOUBLE: path = ",path

        rslt = rp_lib_get_double(lib, path)

        print *,"rslt = ",rslt
      END SUBROUTINE  test_get_dbl

      SUBROUTINE  test_get_int(lib,path)
        integer lib
        integer rslt, rp_lib_get_integer
        character*100 path

        print *,"TESTING GET INTEGER: path = ",path

        rslt = rp_lib_get_integer(lib, path)

        print *,"rslt = ",rslt
      END SUBROUTINE  test_get_int

      SUBROUTINE  test_get_bool(lib,path)
        integer lib
        integer rslt, rp_lib_get_boolean
        character*100 path

        print *,"TESTING GET BOOLEAN: path = ",path

        rslt = rp_lib_get_boolean(lib, path)

        print *,"rslt = ",rslt
      END SUBROUTINE  test_get_bool

      program rplib_f_tests
        IMPLICIT NONE

        integer rp_lib

        integer driver
        CHARACTER*100 inFile, path

        call getarg(1,inFile)
        driver = rp_lib(inFile)
        ! print *,"dict key = ",driver

        ! TESTING ELEMENT
        path = "input.number(min)"
        call test_element(driver, path)

        ! TESTING GET
        path = "input.number(min).current"
        call test_get(driver, path)

        ! TESTING GET STRING
        path = "input.number(min).current"
        call test_get_str(driver, path)

        ! TESTING GET DOUBLE
        path = "input.number(min).current"
        call test_get_dbl(driver, path)

        ! TESTING GET INT
        path = "input.integer(try).current"
        call rp_lib_put_str(driver, path, "12",0)
        call test_get_int(driver, path)
        call rp_lib_put_str(driver, path, "15.043",0)
        call test_get_int(driver, path)

        ! TESTING GET BOOLEAN
        path = "input.boolean(b).current"
        call rp_lib_put_str(driver, path, "true",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "false",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "1",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "0",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "on",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "off",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "yes",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "no",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "t",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "f",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "ye",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "n",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "tr",0)
        call test_get_bool(driver, path)
        call rp_lib_put_str(driver, path, "of",0)
        call test_get_bool(driver, path)

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
