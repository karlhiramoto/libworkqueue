AC_PREREQ(2.59)
AC_INIT([libworkqueue priority workqueue scheduler], [0.1], [karl@hiramoto.org])

AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AC_PROG_CXX
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



AC_CONFIG_FILES([Makefile
		lib/Makefile
		examples/Makefile
		examples/high_load_test/Makefile
		examples/job_control/Makefile
		examples/serial_port_test/Makefile
		examples/simple_test/Makefile
		examples/simple_cpp/Makefile
		examples/periodic_test/Makefile

		])


AC_OUTPUT
