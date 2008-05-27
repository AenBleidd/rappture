function fermi_main(infile)

try
    fermi(infile);
catch
    lasterr
    fid = fopen('errorDetected.txt','wt');
    fprintf(fid,'1\n');
    fclose(fid);
end
