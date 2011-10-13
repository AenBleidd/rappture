
AC_DEFUN([AX_RUBY_DEV_FLAGS],[
    #
    # Check for Ruby include path
    #
    AC_MSG_CHECKING([for Ruby include path])
    if test -z "$RUBY_CPPFLAGS"; then
        ruby_path=`$RUBY -rmkmf -e 'print Config::CONFIG[["archdir"]]'`
        if test -n "${ruby_path}"; then
                ruby_path="-I$ruby_path"
        fi
        RUBY_CPPFLAGS=$ruby_path
    fi
    AC_MSG_RESULT([$RUBY_CPPFLAGS])
    AC_SUBST([RUBY_CPPFLAGS])

    #
    # Check for Ruby library path
    #
    AC_MSG_CHECKING([for Ruby library path])
    if test -z "$RUBY_LDFLAGS"; then
        RUBY_LDFLAGS=`$RUBY -rmkmf -e 'print Config::CONFIG[["LIBRUBYARG_SHARED"]]'`
    fi
    AC_MSG_RESULT([$RUBY_LDFLAGS])
    AC_SUBST([RUBY_LDFLAGS])

    #
    # Check for site packages
    #
    AC_MSG_CHECKING([for Ruby site-packages path])
    if test -z "$RUBY_SITE_PKG"; then
        RUBY_SITE_PKG=`$RUBY -rmkmf -e 'print Config::CONFIG[["sitearchdir"]]'`
    fi
    AC_MSG_RESULT([$RUBY_SITE_PKG])
    AC_SUBST([RUBY_SITE_PKG])

    #
    # libraries which must be linked in when embedding
    #
    AC_MSG_CHECKING(ruby extra libraries)
    if test -z "$RUBY_EXTRA_LIBS"; then
       RUBY_EXTRA_LIBS=`$RUBY -rmkmf -e 'print Config::CONFIG[["SOLIBS"]]'`
    fi
    AC_MSG_RESULT([$RUBY_EXTRA_LIBS])
    AC_SUBST(RUBY_EXTRA_LIBS)

    #
    # linking flags needed when embedding
    # (is it even needed for Ruby?)
    #
    # AC_MSG_CHECKING(ruby extra linking flags)
    # if test -z "$RUBY_EXTRA_LDFLAGS"; then
    # RUBY_EXTRA_LDFLAGS=`$RUBY -rmkmf -e 'print Config::CONFIG[["LINKFORSHARED"]]'`
    # fi
    # AC_MSG_RESULT([$RUBY_EXTRA_LDFLAGS])
    # AC_SUBST(RUBY_EXTRA_LDFLAGS)

    # this flags breaks ruby.h, and is sometimes defined by KDE m4 macros
    CFLAGS="`echo "$CFLAGS" | sed -e 's/-std=iso9899:1990//g;'`"
    #
    # final check to see if everything compiles alright
    #
    AC_MSG_CHECKING([consistency of all components of ruby development environment])
    AC_LANG_PUSH([C])
    # save current global flags
    ac_save_LIBS="$LIBS"
    LIBS="$ac_save_LIBS $RUBY_LDFLAGS"
    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$ac_save_CPPFLAGS $RUBY_CPPFLAGS"
    AC_TRY_LINK([
        #include <ruby.h>
    ],[
        ruby_init();
    ],[rubyexists=yes],[rubyexists=no])

    AC_MSG_RESULT([$rubyexists])

    if test "$rubyexists" = "no"; then
      HAVE_RUBY_DEVEL="no"
    fi
    AC_LANG_POP
    # turn back to default flags
    CPPFLAGS="$ac_save_CPPFLAGS"
    LIBS="$ac_save_LIBS"

    #
    # all done!
    #
])
