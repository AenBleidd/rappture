      program showdata
        IMPLICIT NONE

        integer rp_lib, rp_lib_get_file
        integer driver, nbytes
        CHARACTER*100 inFile, strVal

        call getarg(1,inFile)
        driver = rp_lib(inFile)
        if (driver .eq. 0) then
            write(0,*) "Error while opening ",inFile
            stop
        endif

        nbytes = rp_lib_get_file(driver,
     +        "input.string(data).current", "mydata.dat")
        if (nbytes .le. 0) then
            write(strVal,*) "Error while writing data file"
            write(0,*) strVal
            call rp_lib_put_str (driver,"output.log",strVal,0)
            call rp_result(driver)
            stop
        endif

        OPEN (UNIT=5, FILE="mydata.dat", STATUS="OLD")

        call rp_lib_put_file (driver,"output.string(data).current",
     +          "mydata.dat",0,0)

        CLOSE (UNIT=5, STATUS="DELETE")

        call rp_result(driver)
      end program showdata
