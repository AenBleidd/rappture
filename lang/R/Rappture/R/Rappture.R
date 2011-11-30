.packageName <- 'Rappture'

rp_lib <- function(lib) {
    .Call("RPRLib",lib)
}

rp_lib_get_string <- function(handle, path) {
    .Call("RPRLibGetString",handle,path)
}

rp_lib_get_double <- function(handle, path) {
    .Call("RPRLibGetDouble",handle,path)
}

rp_lib_get_integer <- function(handle, path) {
    .Call("RPRLibGetInteger",handle,path)
}

rp_lib_get_boolean <- function(handle, path) {
    .Call("RPRLibGetBoolean",handle,path)
}

rp_lib_get_file <- function(handle, path, fname) {
    .Call("RPRLibGetFile",handle,path,fname)
}

rp_lib_put_string <- function(handle, path, value, append) {
    .Call("RPRLibPutString",handle,path,value,append)
}

rp_lib_put_double <- function(handle, path, value, append) {
    .Call("RPRLibPutDouble",handle,path,value,append)
}

rp_lib_put_file <- function(handle, path, fname, compress, append) {
    .Call("RPRLibPutFile",handle,path,fname,compress,append)
}

rp_lib_result <- function(handle) {
    .Call("RPRLibResult",handle)
}

rp_units_convert_double <- function(from,to) {
    .Call("RPRUnitsConvertDouble",from,to)
}

rp_units_convert_string <- function(from,to,showUnits) {
    .Call("RPRUnitsConvertString",from,to,showUnits)
}

.onLoad <- function(lib,pkg) {
}
