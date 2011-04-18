AC_DEFUN([RP_LANG_JAVA],[
rp_with_java="yes"
AC_ARG_WITH([java],
    [AS_HELP_STRING([--with-java[=DIR]],[location of java @<:@default=yes@:>@])],
    [rp_with_java=$withval],
    [rp_with_java=yes])
JAVA=""
JAVAH=""
JAVAC=""
JAVA_DEV_PKG="no"
AC_ARG_WITH(
    [java],
    [AS_HELP_STRING([--with-java=DIR], [location of java @<:@default=yes@:>@])],
    [rp_with_java=$withval])

if test "${rp_with_java}" != "no" ; then
  if test "${rp_with_java}" = "yes" ; then
    AC_PATH_PROG(JAVA,  java)
    AC_PATH_PROG(JAVAC, javac)
    AC_PATH_PROG(JAVAH, javah)
  else
    AC_PATH_PROG(JAVA,  java,  [], [${rp_with_java}/bin:${rp_with_java}])
    AC_PATH_PROG(JAVAC, javac, [], [${rp_with_java}/bin:${rp_with_java}])
    AC_PATH_PROG(JAVAH, javah, [], [${rp_with_java}/bin:${rp_with_java}])
  fi
fi
JDK=
JAVA_INC_DIR=
JAVA_INC_SPEC=

# If java exists, let's look for the jni.h file.
if test "x${JAVA}" != "x" ; then
  for d in \
   ${rp_with_java} \
   ${JAVA_HOME} \
   /apps/java/jdk1.6* \
   /usr/lib/jvm/*sun-1.6* \
   /opt/sun-jdk-1.6* \
   /opt/icedtea6-* \
   /opt/sun-jdk-1.5* \
   /usr/lib/jvm/*sun-1.5*
  do
    if test -r "${d}/include/jni.h" ; then
      JDK=${d}
      JAVA_HOME=$JDK
      JAVA_INC_DIR=${JDK}/include
      JAVA_INC_SPEC="-I${JDK}/include -I${JDK}/include/linux"
      break;
    fi
    if test -r "${d}/Headers/jni.h" ; then
      JDK=${d}
      JAVA_HOME=$JDK
      JAVA_INC_DIR=${JDK}/Headers
      JAVA_INC_SPEC="-I${JDK}/Headers -I${JDK}/Headers/macos"
      break;
    fi
  done
fi
])