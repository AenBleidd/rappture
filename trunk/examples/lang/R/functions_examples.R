require(Rappture)


lib = rp_lib("driver.xml")
cat("lib = ",lib,"\n")

ee = rp_lib_get_string(lib,"input.string(ee).current")
cat("ee = ",ee," is.character = ",is.character(ee),"\n")

dd = rp_lib_get_double(lib,"input.number(temperature).current")
cat("dd = ",dd," is.real = ",is.real(dd),"\n")

ii = rp_lib_get_integer(lib,"input.integer(ii).current")
cat("ii = ",ii," is.integer = ",is.integer(ii),"\n")

bb = rp_lib_get_boolean(lib,"input.boolean(bb).current")
cat("bb = ",bb," is.logical = ",is.logical(bb),"\n")

nbytes = rp_lib_get_file(lib,"input.string(ee).current","myoutfile")
cat("nbytes = ",nbytes," is.integer = ",is.integer(nbytes),"\n")

err = rp_lib_put_string(lib,"output.string(ps).current","voodoo",TRUE)
cat("err = ",err," is.integer = ",is.integer(err),"\n")

d = as.real(12.45)
err = rp_lib_put_double(lib,"output.number(ps).current",d,TRUE)
cat("err = ",err," is.integer = ",is.integer(err),"\n")

pf = rp_lib_put_file(lib,"output.string(pf).current","myoutfile",FALSE,FALSE)
cat("err = ",err," is.integer = ",is.integer(err),"\n")

result = rp_lib_result(lib)
cat("result = ",result," is.integer = ",is.integer(result),"\n")

result = rp_units_convert_double("0C","F")
cat("result = ",result," is.real = ",is.real(result),"\n")

show = TRUE
result = rp_units_convert_string("100mm","m",show)
cat("result = ",result," is.character = ",is.character(result),"\n")

show = FALSE
result = rp_units_convert_string("100mm","m",show)
cat("result = ",result," is.character = ",is.character(result),"\n")

percent = as.integer(56)
result = rp_utils_progress(percent,"almost done...")
cat("result = ",result," is.integer = ",is.integer(result),"\n")

result = rp_utils_progress(76,"almost done...")
cat("result = ",result," is.integer = ",is.integer(result),"\n")

result = rp_utils_progress(36.546,"almost done...")
cat("result = ",result," is.integer = ",is.integer(result),"\n")
