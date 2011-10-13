
#
# Octave variables:
#
#	OCTAVE			Path to default octave executable.  Will use
#				to determine the version.
#	OCTAVE_VERSION		Full version of octave (X.X.X).  Used in 
#				Makefiles to indicate if we are building 
#				the octave language bindings.  The empty
#				string indicates octave is not available.
#	OCTAVE_VERSION_MAJOR	Single verion number of octave.  Used in 
#				rappture.env scripts to indicate how to set 
#				OCTAVE_PATH and OCTAVE_LOADPATH variables.
#	MKOCTFILE		Path to the default octave compiler.  This
#				variable isn't used directly.  Either 
#				MKOCTFILE2 or MKOCTFILE3 will be set to its
#				value.
#	MKOCTFILE2		Path to octave version 2 compiler.
#	MKOCTFILE3		Path to octave version 3 compiler.
#

# Standard octave search (use the installed version of octave)
AC_DEFUN([RP_LANG_OCTAVE],[
AC_ARG_WITH(
    [octave],
    [AS_HELP_STRING([--with-octave[=DIR]],
        [path of default octave compiler `mkoctfile' @<:@default=yes@:>@])],
    [],
    [with_octave=yes])

OCTAVE=
OCTAVE_VERSION=
OCTAVE_VERSION_MAJOR=
MKOCTFILE2=
MKOCTFILE3=

if test "$with_octave" != "no" ; then
  if test "$with_octave" = "yes" ; then
    AC_PATH_PROG(OCTAVE, octave)
    AC_PATH_PROG(MKOCTFILE, mkoctfile)
  else
    OCTAVE=$with_octave
    MKOCTFILE=`dirname $with_octave`/mkoctfile
  fi
fi

if test "x${OCTAVE}" != "x" ; then 
  OCTAVE_VERSION=`${OCTAVE} -v | grep version | cut -d' ' -f4`
  OCTAVE_VERSION_MAJOR=`echo ${OCTAVE_VERSION} | cut -d'.' -f1`
  if test "${OCTAVE_VERSION_MAJOR}" == "3" ; then
    OCTAVE3=$OCTAVE
    MKOCTFILE3=$MKOCTFILE
  fi
  if test "${OCTAVE_VERSION_MAJOR}" == "2" ; then
    OCTAVE2=$OCTAVE
    MKOCTFILE2=$MKOCTFILE
  fi
fi

#
# Check for octave3 before octave2 so that we can override the 
# OCTAVE_VERSION variables.  
#
# Rappture doesn't normally really support both octave2 and octave3 
# simultaneously.  NANOhub has a hacked version of octave3 that 
# looks for OCTAVE_LOADPATH before looking at OCTAVE_PATH (i.e. 
# OCTAVE_PATH tells octave2 where to load the rappture bindings 
# and OCTAVE_LOADPATH tells octave3 where to load).
#
# Usually you will have installed either octave2 or octave3, but 
# not both.  
#

# Check if octave3 was designated *in addition* to the installed version.
# This can override the default version if they are the same versions.

AC_ARG_WITH(
    [mkoctfile3],
    [AS_HELP_STRING([--with-mkoctfile3[=DIR]],
        [path of octave compiler `mkoctfile' @<:@default=no@:>@])],
    [],
    [with_mkoctfile3=no])

if test "$with_mkoctfile3" != "no" ; then
  if test "$with_mkoctfile3" = "yes" ; then
    AC_PATH_PROG(mkoctfile3, mkoctfile)
  else 
    MKOCTFILE3=$with_mkoctfile3
  fi
  OCTAVE_VERSION=`${MKOCTFILE3} --version 2>&1 | cut -d' ' -f3`
  OCTAVE_VERSION_MAJOR=`echo ${OCTAVE_VERSION} | cut -d'.' -f1`
fi

# Check if mkoctfile2 was designated *in addition* to the installed version.
# This can override the default version if they are the same versions.
AC_ARG_WITH(
    [mkoctfile2],
    [AS_HELP_STRING([--with-mkoctfile2[=DIR]],
        [path of octave compiler `mkoctfile' @<:@default=no@:>@])],
    [],
    [with_mkoctfile2=no])

if test "$with_mkoctfile2" != "no" ; then
  if test "$with_mkoctfile2" = "yes" ; then
    AC_PATH_PROG(mkoctfile2, mkoctfile)
  else 
    MKOCTFILE2=$with_mkoctfile2
  fi
  # Have to use octave to get a version, instead of mkoctfile.
  octave=`dirname $MKOCTFILE2`/octave
  OCTAVE_VERSION=`${octave} -v | grep version | cut -d' ' -f4`
  OCTAVE_VERSION_MAJOR=`echo ${OCTAVE_VERSION} | cut -d'.' -f1`
fi
])