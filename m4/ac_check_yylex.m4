# This macro check if flex function yylex_destroy is available. This function
# is generated by flex, so we have just to check flex version - 2.5.9 is
# the first one with yylex_destroy. On success HAVE_YYLEX_DESTROY is defined.
AC_DEFUN([AC_CHECK_YYLEX_DESTROY], [
	AC_MSG_CHECKING([for yylex_destroy])
	if test "x$LEX" = "xflex" ; then
		ac_flex_version_string=`$LEX --version | tr -d 'a-z '`
		ac_flex_version_major=`echo $ac_flex_version_string \
			| cut -f 1 -d .`
		ac_flex_version_minor=`echo $ac_flex_version_string \
			| cut -f 2 -d .`
		ac_flex_version_sub=`echo $ac_flex_version_string \
			| cut -f 3 -d .`
		if test \( "$ac_flex_version_major" -gt 2 \) ; then
			o1=yes
		else
			o1=false
		fi
		if test	\( "$ac_flex_version_major" = 2 \) -a \
			\( "$ac_flex_version_minor" -gt 5 \) ; then
			o2=yes
		else
			o2=false
		fi
		if test	\( "$ac_flex_version_major" = 2 \) -a  \
			\( "$ac_flex_version_minor" = 5 \) -a \
			\( "$ac_flex_version_sub"  -ge 9 \) ; then
			o3=yes
		else
			o3=false
		fi
			
		if test $o1 = yes -o $o2 = yes -o $o3 = yes ; then
			AC_MSG_RESULT([yes])
			AC_DEFINE(HAVE_YYLEX_DESTROY, 1, 
				[Define to 1 if flex generates yylex_destroy])
		else
			AC_MSG_RESULT([no])
		fi
	fi
])

