AC_DEFUN([RP_LANG_MATLAB],[
AC_ARG_WITH(
    [matlab],
    [AS_HELP_STRING([--with-matlab[=DIR]],
        [location of matlab and mex compiler @<:@default=yes@:>@])],
    [],
    [with_matlab=yes])

MCC=""
MEX=""
MEX_ARCH=""
MEXEXT=""
MATLAB=
if test "$with_matlab" != "no" ; then
  if test "$with_matlab" = "yes" ; then
    AC_PATH_PROG(MATLAB, matlab)
  else
    AC_PATH_PROG(MATLAB, matlab, [], [${with_matlab}/bin:${with_matlab}])
  fi
fi

if test "x$MATLAB" != "x" ; then
  # Found matlab.  May be a symlink to the real binary.  Run "matlab -e"
  # to tell where matlab is installed.

  matlab_bindir=`${MATLAB} -e | grep "MATLAB=" | sed s/MATLAB=//`/bin

  # Now see if we can find "mex" or "mexext" there.
  AC_PATH_PROG(MEX, mex, [], [${matlab_bindir}])
  AC_PATH_PROG(MEXEXT, mexext, [], [${matlab_bindir}])

  # Run "mexext" to get the expected module extension for this platform.
  AC_MSG_CHECKING([for mex extension])
  if test "x$MEXEXT" != "x" ; then
    MEXEXT=`$MEXEXT`
  else
    MEXEXT="mexglx"
  fi
  AC_MSG_RESULT([$MEXEXT])
  AC_PATH_PROG(MCC, mcc, [], [${matlab_bindir}])
  AC_MSG_CHECKING([for mcc extension])
fi
])
