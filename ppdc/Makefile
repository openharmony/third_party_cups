#
# Makefile for the CUPS PPD Compiler.
#
# Copyright © 2020-2024 by OpenPrinting.
# Copyright © 2007-2019 by Apple Inc.
# Copyright © 2002-2006 by Easy Software Products.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

#
# Include standard definitions...
#

include ../Makedefs


#
# Object files...
#

LIBOBJS =	\
		ppdc-array.o \
		ppdc-attr.o \
		ppdc-catalog.o \
		ppdc-choice.o \
		ppdc-constraint.o \
		ppdc-driver.o \
		ppdc-file.o \
		ppdc-filter.o \
		ppdc-font.o \
		ppdc-group.o \
		ppdc-import.o \
		ppdc-mediasize.o \
		ppdc-message.o \
		ppdc-option.o \
		ppdc-profile.o \
		ppdc-shared.o \
		ppdc-source.o \
		ppdc-string.o \
		ppdc-variable.o
OBJS =		\
		$(LIBOBJS) \
		genstrings.o \
		ppdc.o \
		ppdhtml.o \
		ppdi.o \
		ppdmerge.o \
		ppdpo.o \
		testcatalog.o
LIBTARGETS =	\
		libcupsppdc.a
UNITTARGETS =	\
		ppdc-static \
		ppdi-static \
		testcatalog
EXECTARGETS =	\
		ppdc \
		ppdhtml \
		ppdi \
		ppdmerge \
		ppdpo

TARGETS	=	\
		$(LIBTARGETS) \
		$(EXECTARGETS) \
		$(LOCALTARGET)


#
# Make everything...
#

all:		$(TARGETS)


#
# Make library targets...
#

libs:


#
# Make unit tests...
#

unittests:	$(UNITTARGETS)


#
# Clean everything...
#

clean:
	$(RM) $(OBJS) core
	$(RM) *.bak *.bck core.*
	$(RM) $(TARGETS) $(UNITTARGETS) genstrings
	$(RM) -r ppd ppd2
	$(RM) sample-import.drv sample.c test.drv


#
# Update dependencies...
#

depend:
	$(CXX) -MM $(ALL_CXXFLAGS) $(OBJS:.o=.cxx) >Dependencies


#
# Install all targets...
#

install:	all install-data install-headers install-libs install-exec


#
# Install data files...
#

install-data:
	$(INSTALL_DIR) $(DATADIR)/drv
	$(INSTALL_DATA) sample.drv $(DATADIR)/drv


#
# Install programs...
#

install-exec:
	echo Installing PPD compiler programs...
	$(INSTALL_DIR) $(BINDIR)
	for file in $(EXECTARGETS); do \
		$(INSTALL_BIN) $$file $(BINDIR); \
	done
	if test "x$(SYMROOT)" != "x"; then \
		$(INSTALL_DIR) $(SYMROOT); \
		for file in $(EXECTARGETS); do \
			cp $$file $(SYMROOT); \
			dsymutil $(SYMROOT)/$$file; \
		done \
	fi


#
# Install headers...
#

install-headers:


#
# Install libraries...
#

install-libs:


#
# Uninstall...
#

uninstall:
	for file in $(EXECTARGETS); do \
		$(RM) $(BINDIR)/$$file; \
	done
	$(RM) $(DATADIR)/drv/sample.drv
	$(RMDIR) $(DATADIR)/drv


#
# Local programs (not built when cross-compiling...)
#

local:	genstrings


#
# genstrings - generate GNU gettext strings.
#

genstrings:		genstrings.o libcupsppdc.a ../cups/$(LIBCUPSSTATIC) \
			sample.drv ../data/media.defs
	echo Linking $@...
	$(LD_CXX) $(ARCHFLAGS) $(ALL_LDFLAGS) -o genstrings genstrings.o \
		libcupsppdc.a $(LINKCUPSSTATIC)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@
	echo Generating localization strings...
	./genstrings >sample.c


#
# ppdc, the PPD compiler.
#

ppdc:			ppdc.o libcupsppdc.a ../cups/$(LIBCUPS)
	echo Linking $@...
	$(LD_CXX) $(ALL_LDFLAGS) -o $@ ppdc.o libcupsppdc.a $(COMMONLIBS) $(LINKCUPS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


ppdc-static:		ppdc.o libcupsppdc.a ../cups/$(LIBCUPSSTATIC) foo.drv foo-fr.po
	echo Linking $@...
	$(LD_CXX) $(ARCHFLAGS) $(ALL_LDFLAGS) -o ppdc-static ppdc.o libcupsppdc.a \
		$(LINKCUPSSTATIC)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@
	echo Testing PPD compiler...
	./ppdc-static -l en,fr -I ../data foo.drv
	./ppdc-static -l en,fr -z -I ../data foo.drv


#
# ppdhtml, the PPD to HTML utility.
#

ppdhtml:			ppdhtml.o libcupsppdc.a ../cups/$(LIBCUPS)
	echo Linking $@...
	$(LD_CXX) $(ALL_LDFLAGS) -o $@ ppdhtml.o libcupsppdc.a $(COMMONLIBS) $(LINKCUPS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# ppdi, import PPD files.
#

ppdi:			ppdi.o libcupsppdc.a ../cups/$(LIBCUPS)
	echo Linking $@...
	$(LD_CXX) $(ALL_LDFLAGS) -o $@ ppdi.o libcupsppdc.a $(COMMONLIBS) $(LINKCUPS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


ppdi-static:		ppdc-static ppdi.o libcupsppdc.a ../cups/$(LIBCUPSSTATIC)
	echo Linking $@...
	$(LD_CXX) $(ARCHFLAGS) $(ALL_LDFLAGS) -o ppdi-static ppdi.o libcupsppdc.a \
		$(LINKCUPSSTATIC)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@
	echo Testing PPD importer...
	$(RM) -r ppd ppd2 sample-import.drv
	./ppdc-static -l en -I ../data sample.drv
	./ppdi-static -I ../data -o sample-import.drv ppd/*
	./ppdc-static -l en -I ../data -d ppd2 sample-import.drv
	if diff -r ppd ppd2 >/dev/null; then \
		echo PPD import OK; \
	else \
		echo PPD import FAILED; \
		exit 1; \
	fi


#
# ppdmerge, merge PPD files.
#

ppdmerge:			ppdmerge.o ../cups/$(LIBCUPS)
	echo Linking $@...
	$(LD_CXX) $(ALL_LDFLAGS) -o $@ ppdmerge.o $(LINKCUPS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# ppdpo, create message catalog files.
#

ppdpo:			ppdpo.o libcupsppdc.a ../cups/$(LIBCUPS)
	echo Linking $@...
	$(LD_CXX) $(ALL_LDFLAGS) -o $@ ppdpo.o libcupsppdc.a $(COMMONLIBS) $(LINKCUPS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# testcatalog, test ppdcCatalog class.
#

testcatalog:		testcatalog.o libcupsppdc.a ../cups/$(LIBCUPSSTATIC)
	echo Linking $@...
	$(LD_CXX) $(ALL_LDFLAGS) -o $@ testcatalog.o libcupsppdc.a \
		$(LINKCUPSSTATIC)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# libcupsppdc.a
#

libcupsppdc.a:	$(LIBOBJS)
	echo Archiving $@...
	$(RM) $@
	$(AR) $(ARFLAGS) $@ $(LIBOBJS)
	$(RANLIB) $@


#
# Include dependencies...
#

include Dependencies
