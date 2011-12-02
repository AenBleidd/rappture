require(Rappture)


lib = rp_lib("driver.xml")
lib

ee = rp_lib_get_string(lib,"input.string(ee).current")
is.character(ee)
ee

dd = rp_lib_get_double(lib,"input.number(temperature).current")
is.real(dd)
dd

ii = rp_lib_get_integer(lib,"input.integer(ii).current")
is.integer(ii)
ii

bb = rp_lib_get_boolean(lib,"input.boolean(bb).current")
is.logical(bb)
bb

ff = rp_lib_get_file(lib,"input.string(ee).current","myoutfile")
is.integer(ff)
ff

ps = rp_lib_put_string(lib,"output.string(ps).current","voodoo",TRUE)
is.integer(ps)
ps

d = as.real(12.45)
pd = rp_lib_put_double(lib,"output.number(ps).current",d,TRUE)
is.integer(pd)
pd

pf = rp_lib_put_file(lib,"output.string(pf).current","myoutfile",FALSE,FALSE)
is.integer(pf)
pf

result = rp_lib_result(lib)
is.integer(result)
result

result = rp_units_convert_double("0C","F")
is.real(result)
result

show = TRUE
result = rp_units_convert_string("100mm","m",show)
is.character(result)
result

show = FALSE
result = rp_units_convert_string("100mm","m",show)
is.character(result)
result
