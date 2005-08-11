c   file fermi_example.f

        program fermi_example
            IMPLICIT NONE

            integer rp_lib
            real*8 rp_lib_get_double
            
            CHARACTER*100 inFile, outFile
            character*100 T_path, Ef_path, outPath, outPath2,outPath3
            character*40 xy
            integer handle
            real*8 T,Ef,kT,Emin,Emax,dE,f,E,kTconstExp

            ! inFile = "driver.xml"
            call getarg(1,inFile)
            outFile = "run.xml"

            handle = 0

            handle = rp_lib(inFile)

            T_path = "input.(ambient).(temperature).current"
            Ef_path = "input.number(Ef).default"

            T = rp_lib_get_double(handle,T_path)
            Ef = rp_lib_get_double(handle,Ef_path)


c            kT = 8.61734e-5 * T
            kTconstExp = (10.0)**(-5.0)
            kT = (8.61734*kTconstExp) * T 

            Emin = (Ef - (10*kT))
            Emax = (Ef + (10*kT))

            dE = (0.005)*(Emax - Emin)

            outPath = "output.curve(f12).xaxis.label"
            call rp_lib_put_str(handle,outPath,"f12 x label",1)
            outPath = "output.curve(f12).yaxis.label"
            call rp_lib_put_str(handle,outPath,"f12 y label",1)

            outPath = "output.curve(f12).component.xy"

            do 15,E = Emin,Emax,dE
                f = 1.0/(1.0+exp((E-Ef)/kT))
                write(xy,'(E20.12,F13.9,A)')f,E,char(10)
                call rp_lib_put_str(handle,outPath,xy,1)
 15         continue

            outPath = "output.curve(f13).about.group"
            call rp_lib_put_str(handle,outPath,"group1",1)
            outPath = "output.curve(f13).xaxis.label"
            call rp_lib_put_str(handle,outPath,"f13 x label",1)
            outPath = "output.curve(f13).yaxis.label"
            call rp_lib_put_str(handle,outPath,"f13 y label",1)

            outPath = "output.curve(f13).component.xy"

            outPath2 = "output.curve(f14).about.group"
            call rp_lib_put_str(handle,outPath2,"group1",1)
            outPath2 = "output.curve(f14).component.xy"

            outPath3 = "output.curve(f15).about.group"
            call rp_lib_put_str(handle,outPath3,"group1",1)
            outPath3 = "output.curve(f15).component.xy"

            do 16,E = Emin,Emax,dE
                f = -1.0/(1.0+exp((E-Ef)/kT))
                write(xy,'(E20.12,F13.9,A)')f,E,char(10)
                call rp_lib_put_str(handle,outPath,xy,1)
                f = -4.0/(1.0+exp((E-Ef)/kT))
                write(xy,'(E20.12,F13.9,A)')f,E,char(10)
                call rp_lib_put_str(handle,outPath2,xy,1)
                f = 1.0/(1.0+exp((E-Ef)/kT))
                write(xy,'(E20.12,F13.9,A)')f,E,char(10)
                call rp_lib_put_str(handle,outPath3,xy,1)
 16         continue

            
            call rp_lib_write_xml(handle, outFile)
            call rp_quit()

            print *,"=RAPPTURE-RUN=>"//outFile

        end program fermi_example
