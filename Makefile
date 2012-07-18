##############################################################################
#
#    file                 : Makefile
#    created              : Fri Dec 10 14:05:25 CET 2010
#    copyright            : (C) 2002 Christoph Schwering
#
##############################################################################

include /home/chs/Documents/Mercury/prGolog/Mercury/Make.common

COMPILFLAGS := -I$(LIB_DIR)/inc
LIBS        = -lboost_system -lboost_thread-mt
#LIBS	    = -lpr

ROBOT       = chs
MODULE      = ${ROBOT}.so
MODULEDIR   = drivers/${ROBOT}
SOURCES     = ${ROBOT}.cpp driver.cpp trackprofile.cpp simpledriver.cpp transmission.cpp autothrottle.cpp minithrottle.cpp abs.cpp worldmodel.cpp image.cpp pnmimage.cpp delay.cpp

SHIPDIR     = drivers/${ROBOT}
SHIP        = ${ROBOT}.xml logo.rgb

PKGSUBDIRS  = ${SHIPSUBDIRS}
src-robots-chs_PKGFILES = $(shell find * -maxdepth 0 -type f -print)
src-robots-chs_PKGDIR   = ${PACKAGE}-${VERSION}/$(subst ${TORCS_BASE},,$(shell pwd))

include ${MAKE_DEFAULT}

