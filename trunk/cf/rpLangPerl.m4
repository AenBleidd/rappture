
AC_DEFUN([RP_LANG_PERL],[
AC_ARG_WITH(
    [perl],
    [AS_HELP_STRING([--with-perl[=DIR]], [location of perl @<:@default=yes@:>@])],
    [],
    [with_perl=yes])

PERL=
PERL_INCLUDES=
PERL_ARCHLIB=
PERL_ARCHLIBEXP=
PERL_VERSION=
PERL_VENDORLIB=
PERL_PRIVLIB=
PERL_CPPFLAGS=
PERL_CCFlAGS=
PERL_VERSION_RV=
XSUBPP=
PERL_LIBSPEC=
if test "$with_perl" != "no" ; then
  if test "$with_perl" != "yes" ; then 
    AC_PATH_PROG(PERL, perl, [], [$with_perl/bin:$with_perl])
    AC_PATH_PROG(XSUBPP, xsubpp, [], [$with_perl/bin:$with_perl])
  else
    AC_PATH_PROG(PERL, perl)
    AC_PATH_PROG(XSUBPP, xsubpp)
  fi
  if test "x${PERL}" != "x" ; then 
    PERL_ARCHLIB=`${PERL} -MConfig -e 'print $Config{archlib}'`
    PERL_VERSION=`${PERL} -MConfig -e 'print $Config{version}'`
    PERL_CCFLAGS=`${PERL} -MConfig -e 'print $Config{ccflags}'`
    PERL_CPPFLAGS=`${PERL} -MConfig -e 'print $Config{cppflags}'`
    PERL_VENDORLIB=`${PERL} -MConfig -e 'print $Config{vendorlib}'`
    PERL_PRIVLIB=`${PERL} -MConfig -e 'print $Config{privlib}'`
    PERL_INSTALLARCHLIB=`${PERL} -MConfig -e 'print $Config{installarchlib}'`
    PERL_ARCHLIBEXP=`${PERL} -MConfig -e 'print $Config{archlibexp}'`
    PERL_VERSION_RV=`echo ${PERL_VERSION} | cut -d'.' -f1-2`
    # libperl may or may not be installed.  Check for its existence.
    if test -f "${PERL_ARCHLIBEXP}/CORE/libperl${SHLIB_SUFFIX}" ; then 
      PERL_LIBSPEC="-L${PERL_ARCHLIBEXP}/CORE -lperl"
    fi
  fi
fi
])