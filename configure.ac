#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([clboss], [0.3A], [ZmnSCPxj@protonmail.com])
AC_CONFIG_AUX_DIR([auxdir])
AM_INIT_AUTOMAKE([subdir-objects tar-ustar])
AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
LT_INIT([disable-shared])

# Checks for programs.
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11
AC_LANG([C++])

AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AX_PTHREAD([:],[
	AC_MSG_ERROR([Need pthread.])
])
AX_LIB_EV
PKG_CHECK_MODULES([SQLITE3], [sqlite3 >= 3.0.0])
AX_LIB_CURL(,[:],[
	AC_MSG_ERROR([Need libcurl.])
])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile external/bitcoin-ripemd160/Makefile])
AC_CONFIG_SUBDIRS([external/libsodium
		   external/secp256k1
		  ])

AC_OUTPUT
