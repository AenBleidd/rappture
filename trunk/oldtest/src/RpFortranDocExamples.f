c ----------------------------------------------------------------------
c  TEST: Fortran Documentation Examples.
c
c  Simple tests of Rappture's Fortran API found on our website
c https://developer.nanohub.org/projects/rappture/wiki/rappture_fortran_api
c
c ======================================================================
c  AUTHOR:  Derrick S. Kearney, Purdue University
c  Copyright (c) 2004-2012  HUBzero Foundation, LLC
c
c  See the file "license.terms" for information on usage and
c  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
c ======================================================================
      SUBROUTINE  test_rp_lib(filename)

        integer handle, rp_lib
        character*100 filename
        handle = rp_lib(filename)
        print *,handle

      END SUBROUTINE  test_rp_lib

      SUBROUTINE  test_rp_lib_element_comp(filename)

        character*100 filename
        character*100 path,retText
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        path = "input.number(temperature)"
        call rp_lib_element_comp(libHandle,path,retText)

        print *,retText

      END SUBROUTINE  test_rp_lib_element_comp

      SUBROUTINE  test_rp_lib_element_id(filename)

        character*100 filename
        character*100 path,retText
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        path = "input.number(temperature)"
        call rp_lib_element_id(libHandle,path,retText)

        print *,retText

      END SUBROUTINE  test_rp_lib_element_id

      SUBROUTINE  test_rp_lib_element_type(filename)

        character*100 filename
        character*100 path,retText
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        path = "input.number(temperature)"
        call rp_lib_element_type(libHandle,path,retText)

        print *,retText

      END SUBROUTINE  test_rp_lib_element_type

      SUBROUTINE  test_rp_lib_element_obj(filename)

        character*100 filename
        character*100 path
        integer libHandle, rp_lib, rp_lib_element_obj, newHandle

        libHandle = rp_lib(filename)

        path = "input.number(temperature)"
        newHandle =  rp_lib_element_obj(libHandle,path)

        print *,newHandle

      END SUBROUTINE  test_rp_lib_element_obj

      SUBROUTINE  test_rp_lib_children(filename)

        character*100 filename
        character*100 retText
        integer rp_lib_children, childHandle, prevChildHandle
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        prevChildHandle = 0
        childHandle = -1

 10     continue
            childHandle =
     +               rp_lib_children(libHandle,"input",prevChildHandle)
            if (childHandle .gt. 0) then
                call rp_lib_node_comp(childHandle,retText)
                print *,"component name: ",retText
           endif
            prevChildHandle = childHandle
        if (childHandle .gt. 0) goto 10

      END SUBROUTINE  test_rp_lib_children

      SUBROUTINE  test_rp_lib_get(filename)
        character*100 filename
        character*100 path,retText
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        path = "input.number(temperature).current"
        call rp_lib_get(libHandle,path,retText)

        print *,retText

      END SUBROUTINE  test_rp_lib_get

      SUBROUTINE  test_rp_lib_put_str(filename)
        character*100 filename
        character*500 path,retText
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        path = "input.number(temperature).min"
        call rp_lib_put_str(libHandle,path,"10",0)

        call rp_lib_xml(libHandle,retText)
        print *,retText

      END SUBROUTINE  test_rp_lib_put_str

      SUBROUTINE  test_rp_lib_node_comp(filename)
        character*100 filename
        character*100 retText,path
        integer rp_lib_element_obj, newHandle
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        path = "input.number(temperature)"
        newHandle = rp_lib_element_obj(libHandle,path)
        call rp_lib_node_comp(newHandle,retText)

        print *,retText

      END SUBROUTINE  test_rp_lib_node_comp

      SUBROUTINE  test_rp_lib_node_type(filename)
        character*100 filename
        character*100 retText,path
        integer rp_lib_element_obj, newHandle
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        path = "input.number(temperature)"
        newHandle = rp_lib_element_obj(libHandle,path)
        call rp_lib_node_type(newHandle,retText)

        print *,retText

      END SUBROUTINE  test_rp_lib_node_type

      SUBROUTINE  test_rp_lib_node_id(filename)
        character*100 filename
        character*100 retText,path
        integer rp_lib_element_obj, newHandle
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        path = "input.number(temperature)"
        newHandle = rp_lib_element_obj(libHandle,path)
        call rp_lib_node_id(newHandle,retText)

        print *,retText

      END SUBROUTINE  test_rp_lib_node_id

      SUBROUTINE  test_rp_lib_xml(filename)
        character*100 filename
        character*500 retText
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)

        call rp_lib_xml(libHandle,retText)
        print *,retText

      END SUBROUTINE  test_rp_lib_xml

      SUBROUTINE  test_rp_result(filename)
        character*100 filename
        integer libHandle, rp_lib

        libHandle = rp_lib(filename)
        call rp_result(libHandle)

      END SUBROUTINE  test_rp_result

      SUBROUTINE  test_rp_define_unit()

        integer unitHandle, rp_define_unit
        unitHandle = rp_define_unit('oir',0)
        unitHandle = rp_define_unit('oi4',0)
        print *,"unitHandle = ",unitHandle

      END SUBROUTINE  test_rp_define_unit

      SUBROUTINE  test_rp_find()

        integer unitHandle, rp_find
        unitHandle = rp_find('oir')
        print *,"unitHandle = ",unitHandle

      END SUBROUTINE  test_rp_find

      SUBROUTINE  test_rp_make_metric()

        integer unitHandle, rp_find, rp_make_metric, ok
        unitHandle = rp_find('oir')
        if (unitHandle .gt. 0) then
            ok = rp_make_metric(unitHandle)
            unitHandle = rp_find('coir')
            print *,"unitHandle = ",unitHandle
        else
            print *,"rp_make_metric FAILED!!!"
        endif
      END SUBROUTINE  test_rp_make_metric

      SUBROUTINE  test_rp_get_units()
        integer unitHandle, rp_find, rp_get_units, ok
        character*100 unitsName
        unitHandle = rp_find('oir')
        if (unitHandle .gt. 0) then
            ok = rp_get_units(unitHandle,unitsName)
            print *,"units = ",unitsName
        else
            print *,"rp_get_units FAILED!!!"
        endif
      END SUBROUTINE  test_rp_get_units

      SUBROUTINE  test_rp_get_units_name()
        integer unitHandle, rp_find, rp_get_units_name, ok
        character*100 unitsName
        unitHandle = rp_find('oir')
        if (unitHandle .gt. 0) then
            ok = rp_get_units_name(unitHandle,unitsName)
            print *,"units = ",unitsName
        else
            print *,"rp_get_units_name FAILED!!!"
        endif
      END SUBROUTINE  test_rp_get_units_name

      SUBROUTINE  test_rp_get_exponent()
        integer unitHandle, rp_find, ok
        double precision expon
        unitHandle = rp_find('oi4')
        if (unitHandle .gt. 0) then
            ok = rp_get_exponent(unitHandle,expon)
            print *,"exponent = ",expon
        else
            print *,"rp_get_exponent FAILED!!!"
        endif
      END SUBROUTINE  test_rp_get_exponent

      SUBROUTINE  test_rp_get_basis()
        integer unitHandle, basisHandle
        integer rp_get_units_name, rp_get_basis,rp_find
        character*100 unitName, basisName
        unitHandle = rp_find('oi4')
        if (unitHandle .gt. 0) then
            basisHandle = rp_get_basis(unitHandle)
            ok = rp_get_units_name(unitHandle,unitName)
            ok = rp_get_units_name(basisHandle,basisName)
            if (basisHandle .gt. 0) then
                print *,unitName,"'s basisName = ",basisName
            else
                print *,unitName," has no basis"
            endif
        else
            print *,"rp_get_basis FAILED!!!"
        endif

        unitHandle = rp_find('nm')
        if (unitHandle .gt. 0) then
            basisHandle = rp_get_basis(unitHandle)
            ok = rp_get_units_name(unitHandle,unitName)
            ok = rp_get_units_name(basisHandle,basisName)
            if (basisHandle .gt. 0) then
                print *,unitName,"'s basisName = ",basisName
            else
                print *,unitName," has no basis"
            endif
        else
            print *,"rp_get_basis FAILED!!!"
        endif
      END SUBROUTINE  test_rp_get_basis

      SUBROUTINE  test_rp_units_convert_dbl()
        integer rp_units_convert_dbl, ok
        double precision dblRslt
        ok = rp_units_convert_dbl("72F","C",dblRslt)

        if (ok .eq. 0) then
            print *, "72F = ",dblRslt,"C"
        else
            print *,"rp_units_convert_dbl FAILED!!!"
        endif
      END SUBROUTINE  test_rp_units_convert_dbl

      SUBROUTINE  test_rp_units_convert_str()
        integer rp_units_convert_str, ok
        character*100 strRslt
        ok = rp_units_convert_str("72F","C",strRslt)

        if (ok .eq. 0) then
            print *, "72F = ",strRslt
        else
            print *,"rp_units_convert_str FAILED!!!"
        endif
      END SUBROUTINE  test_rp_units_convert_str

      program rplib_f_tests
        IMPLICIT NONE

        CHARACTER*100 inFile

        call getarg(1,inFile)

        print *,"TESTING LIB"
        call test_rp_lib(inFile)

        print *,"TESTING ELEMENT COMP"
        call test_rp_lib_element_comp(inFile)

        print *,"TESTING ELEMENT ID"
        call test_rp_lib_element_id(inFile)

        print *,"TESTING ELEMENT TYPE"
        call test_rp_lib_element_type(inFile)

        print *,"TESTING ELEMENT OBJ"
        call test_rp_lib_element_obj(inFile)

        print *,"TESTING CHILDREN"
        call test_rp_lib_children(inFile)

        print *,"TESTING GET"
        call test_rp_lib_get(inFile)

        print *,"TESTING PUT STR"
        call test_rp_lib_put_str(inFile)

        print *,"TESTING NODE COMP"
        call test_rp_lib_node_comp(inFile)

        print *,"TESTING NODE TYPE"
        call test_rp_lib_node_type(inFile)

        print *,"TESTING NODE ID"
        call test_rp_lib_node_id(inFile)

        print *,"TESTING XML"
        call test_rp_lib_xml(inFile)

        print *,"TESTING RESULT"
        call test_rp_result(inFile)

        print *,"TESTING UNITS DEFINE UNIT"
        call test_rp_define_unit()

        print *,"TESTING UNITS FIND"
        call test_rp_find()

        print *,"TESTING UNITS MAKE METRIC"
        call test_rp_make_metric()

        print *,"TESTING UNITS GET UNITS"
        call test_rp_get_units()

        print *,"TESTING UNITS GET UNITS NAME"
        call test_rp_get_units_name()

        print *,"TESTING UNITS GET EXPONENT"
        call test_rp_get_exponent()

        print *,"TESTING UNITS GET BASIS"
        call test_rp_get_basis()

        print *,"TESTING UNITS CONVERT DOUBLE"
        call test_rp_units_convert_dbl()

        print *,"TESTING UNITS CONVERT STRING"
        call test_rp_units_convert_str()

      end program rplib_f_tests
