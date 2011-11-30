AC_DEFUN([RP_LANG_R],[
AC_ARG_WITH(
    [R],
    [AS_HELP_STRING([--with-R[=DIR]],
        [location of R interpreter @<:@default=yes@:>@])],
    [],
    [with_R=yes])

R=""
if test "$with_R" != "no" ; then
  if test "$with_R" = "yes" ; then
    AC_PATH_PROG(R, R)
  else
    AC_PATH_PROG(R, R, [], [${with_R}/bin:${with_R}])
  fi
fi

])
