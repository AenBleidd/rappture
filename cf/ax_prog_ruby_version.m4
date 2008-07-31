
AC_DEFUN([AX_PROG_RUBY_VERSION],[
    AC_REQUIRE([AC_PROG_SED])
    AC_REQUIRE([AC_PROG_GREP])

    AS_IF([test -n "$RUBY"],[
        ax_ruby_version="$1"

        AC_MSG_CHECKING([for ruby version])
        changequote(<<,>>)
        ruby_version=`$RUBY --version 2>&1 | $GREP "^ruby " | $SED -e 's/^.* \([0-9]*\.[0-9]*\.[0-9]*\) .*/\1/'`
        changequote([,])
        AC_MSG_RESULT($ruby_version)

        AC_SUBST([RUBY_VERSION],[$ruby_version])

        AX_COMPARE_VERSION([$ax_ruby_version],[le],[$ruby_version],[
            :
            $2
        ],[
            :
            $3
        ])
    ],[
        AC_MSG_WARN([could not find the ruby interpreter])
        $3
    ])
])

