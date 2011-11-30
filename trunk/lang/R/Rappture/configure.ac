
AC_INIT([Rappture], 1.0, [rappture@nanohub.org])

dnl Select an optional include path, from a configure option
dnl or from an environment variable.
AC_ARG_WITH([rappture-include],
            AC_HELP_STRING([--with-rappture-include=INCLUDE_PATH],
                           [the location of Rappture header files]),
            [rappture_include_path=$withval])
RAPPTURE_CPPFLAGS="-I."
if test [ -n "$rappture_include_path" ] ; then
   RAPPTURE_CPPFLAGS="-I. -I${rappture_include_path}"
else
  if test [ -n "${RAPPTURE_INCLUDE}" ] ; then
     RAPPTURE_CPPFLAGS="-I. -I${RAPPTURE_INCLUDE}"
  fi
fi

dnl ditto for a library path
AC_ARG_WITH([rappture-lib],
            AC_HELP_STRING([--with-rappture-lib=LIB_PATH],
                           [the location of Rappture libraries]),
            [rappture_lib_path=$withval])
if test [ -n "$rappture_lib_path" ] ; then
   LIBS="-L${rappture_lib_path} ${LIBS}"
else 
  if test [ -n "${RAPPTURE_LIBS}" ] ; then
     LIBS="-L${RAPPTURE_LIBS} ${LIBS}"
  fi
fi

dnl Now find the compiler and compiler flags to use
: ${R_HOME=`R RHOME`}
if test -z "${R_HOME}"; then
  echo "could not determine R_HOME"
  exit 1
fi
CC=`"${R_HOME}/bin/R" CMD config CC`
CPP=`"${R_HOME}/bin/R" CMD config CPP`
CFLAGS=`"${R_HOME}/bin/R" CMD config CFLAGS`
CPPFLAGS=`"${R_HOME}/bin/R" CMD config CPPFLAGS`

CPPFLAGS="${CPPFLAGS} ${RAPPTURE_CPPFLAGS}"

dnl Check the headers can be found
AC_CHECK_HEADERS(RpLibrary.h RpUnits.h)
if test "${ac_cv_header_RpLibrary_h}" = no || 
   test "${ac_cv_header_RpUnits_h}" = no; then
   AC_MSG_ERROR("Rappture headers RpLibrary.h and RpUnits.h not found")
fi

dnl search for a library containing Rappture functions
AC_SEARCH_LIBS(rpLibrary, rappture, ,
         AC_MSG_ERROR("no librappture with RpLibrary found"))
AC_SEARCH_LIBS(rpConvert, rappture, ,
         AC_MSG_ERROR("no librappture with RpUnits found"))


AC_CHECK_SIZEOF(long, 4)

dnl substitute RODBC_CPPFLAGS and LIBS
AC_SUBST(RAPPTURE_CPPFLAGS)
AC_SUBST(LIBS)
dnl AC_CONFIG_HEADERS([src/config.h])
dnl and do subsitution in the src/Makevars.in and src/config.h
AC_CONFIG_FILES([src/Makevars])
AC_OUTPUT
