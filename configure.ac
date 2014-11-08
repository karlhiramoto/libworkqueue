AC_PREREQ(2.59)
AC_INIT([libworkqueue priority workqueue scheduler], [0.1], [karl@hiramoto.org])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

######################################################################
# DOXYGEN SUPPORT
######################################################################

DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)

DX_INIT_DOXYGEN([$PACKAGE_NAME],[Doxyfile])

######################################################################

AC_CONFIG_HEADERS([libworkqueue-config.h])


AC_LANG([C])
# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string])


AC_ARG_ENABLE([examples],
	AS_HELP_STRING([--disable-examples], [don't build example programs]),
	[enable_examples="$enableval"], [enable_examples="yes"])

AM_CONDITIONAL([ENABLE_EXAMPLES], [test "$enable_examples" = "yes"])


AC_CONFIG_FILES([Makefile
		lib/Makefile
		examples/Makefile
		examples/high_load_test/Makefile
		examples/job_control/Makefile
		examples/serial_port_test/Makefile
		examples/simple_test/Makefile
		examples/simple_cpp/Makefile
		examples/time_sort/Makefile
		examples/periodic_test/Makefile
		examples/pthread_wrap_ex/Makefile
		])


AC_OUTPUT
