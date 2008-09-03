function fermi_main(infile)

global lib;

try
    lib = rpLib(infile);
    fermi;
catch
    lastmsg = lasterr
    fid = fopen('errorDetected.txt','wt');
    fprintf(fid,'1\n');
    fprintf(fid,'%s\n',lastmsg);
    fclose(fid);
    append = 1;
    rpLibPutString(lib,'output.log',lastmsg,append)
    rpLibResult(lib,1);
end
