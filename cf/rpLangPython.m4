
AC_DEFUN([RP_LANG_PYTHON],[
PYTHON=""
PYTHON_CFLAGS=""
PYTHON_CPPFLAGS=""
PYTHON_DISTUTILS=""
PYTHON_INCLUDES=""
PYTHON_LDFLAGS=""
PYTHON_LIB=""
PYTHON_LIBDIR=""
PYTHON_SITE_DIR=""
PYTHON_SITE_PACKAGES=""
PYTHON_VERSION=""
pythondir=""
AC_ARG_WITH([python],
    [AS_HELP_STRING([--with-python[=DIR]],[location of python @<:@default=yes@:>@])],
    [],
    [with_python=yes])

if test "$with_python" != "no" ; then
  if test "$with_python" = "yes" ; then
    AC_PATH_PROG(PYTHON, python python2.5 python2.4 python2.3 python2.2 python2.1 python2.0 python1.6 python1.5)
  else
    AC_PATH_PROG(PYTHON, python, [], [${with_python}/bin:${with_python}])
  fi
  AC_MSG_CHECKING([for Python version])
  if test "x${PYTHON}" != "x"; then
    PYTHON_VERSION=`${PYTHON} -c "import sys; print sys.version[[0:3]]"`
  fi
  AC_MSG_RESULT([$PYTHON_VERSION])
fi

if test "x${PYTHON_VERSION}" != "x"; then
  PYTHON_INCLUDES=$incdir
  AC_MSG_CHECKING([for Python distutils])
  PYTHON_DISTUTILS=""
  ${PYTHON} -c "from distutils.core import setup; setup(name='test')" \
        build build_ext 2>&1 > /dev/null
  if test $? = 0 ; then
    PYTHON_DISTUTILS="yes"
  fi
  AC_MSG_RESULT([$PYTHON_DISTUTILS])
  if test "${PYTHON_DISTUTILS}" = "yes" ; then
    PYTHON_SITE_PACKAGES=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_lib()"`
    #
    # Check for Python include path
    #
    AC_MSG_CHECKING([path to Python headers])
    PYTHON_INCDIR=`${PYTHON} -c "import distutils.sysconfig; \
     print distutils.sysconfig.get_python_inc();"`
    AC_MSG_RESULT([$PYTHON_INCDIR])
    #
    # Python distutils found, get settings from python directly
    #
    PYTHON_SITE_DIR="`${PYTHON} -c 'from distutils import sysconfig; print sysconfig.get_python_lib(0);'`"
    
    PYTHON_CFLAGS="`$PYTHON -c 'from distutils import sysconfig; flags = [[\"-I\" + sysconfig.get_python_inc(0), \"-I\" + sysconfig.get_python_inc(1), \" \".join(sysconfig.get_config_var(\"CFLAGS\").split())]]; print \" \".join(flags);'`"
    PYTHON_LDFLAGS="`$PYTHON -c 'from distutils import sysconfig; libs = sysconfig.get_config_var(\"LIBS\").split() + sysconfig.get_config_var(\"SYSLIBS\").split(); libs.append(\"-lpython\"+sysconfig.get_config_var(\"VERSION\")); print \" \".join(libs);'`"
    PYTHON_LIB="`$PYTHON -c 'from distutils import sysconfig; print \"python\" + sysconfig.get_config_var(\"VERSION\");'`"
    PYTHON_LIBDIR="`$PYTHON -c 'from distutils import sysconfig; print sysconfig.get_config_var(\"LIBDIR\");'`"
    save_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS -I${PYTHON_INCDIR}"
    AC_CHECK_HEADERS([Python.h], [found=yes], [found=no])
    CPPFLAGS=$save_CPPFLAGS
    if test "$found" = "no" ; then
      PYTHON_DISTUTILS=""
    fi
  fi
fi
])
