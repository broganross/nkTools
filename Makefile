# Compiler Info ('g++-4.1 --version')
# g++-4.1 (GCC) 4.1.2 20080704 (Red Hat 4.1.2-46)
# Copyright (C) 2006 Free Software Foundation, Inc.
# This is free software; see the source for copying conditions.  There is NO
# warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# End Compiler Info Output

# Change this to Nuke's location
NDKDIR ?= /usr/local/Nuke7.0v8
MYCXX ?= g++
LINK ?= g++
CXXFLAGS ?= -g -c -DUSE_GLEW -I$(NDKDIR)/include -fPIC -msse 
LINKFLAGS ?= -L$(NDKDIR) 
LIBS ?= -lDDImage
LINKFLAGS += -shared
VPATH = src
OBJS = DrivenDilate.so Ramp2.so

BUILDDIR = ./build
INSTALLDIR = ~/.nuke
PYTHONDIR = ./python

.PHONY: all 

all: post-build

pre-build:
	@test -d $(BUILDDIR) || mkdir $(BUILDDIR);
	@test -d $(BUILDDIR)/nix || mkdir $(BUILDDIR)/nix;

post-build: main-build
	@echo "post-build";
	@$(MAKE) add-python

main-build: pre-build
	@$(MAKE) --no-print-directory target

target: $(OBJS)

.PRECIOUS : %.os
$(BUILDDIR)/%.os: %.cpp
	$(MYCXX) $(CXXFLAGS) -o $(@) $<

%.so: $(BUILDDIR)/%.os
	$(LINK) $(LINKFLAGS) $(LIBS) -o ./build/nix/$@ $<

add-python:
	@cp $(PYTHONDIR)/init.py $(BUILDDIR)/init.py
	@cp $(PYTHONDIR)/menu.py $(BUILDDIR)/menu.py

install:
	@test -d $(INSTALLDIR) || mkdir $(INSTALLDIR)
	@test -d $(INSTALLDIR)/plugins || mkdir $(INSTALLDIR)/plugins

	@if [ -f $(INSTALLDIR)/menu.py ]; \
	then \
		cat $(PYTHONDIR)/menu.py >> $(INSTALLDIR)/menu.py; \
		echo "WARNING: menu.py contents updated"; \
	else \
		cp $(PYTHONDIR)/menu.py $(INSTALLDIR)/menu.py; \
	fi; \

	@if [ -f $(INSTALLDIR)/init.py ]; \
	then \
		cat $(PYTHONDIR)/init.py >> $(INSTALLDIR)/init.py; \
		echo "WARNING: init.py contents updated"; \
	else \
		cp $(PYTHONDIR)/init.py $(INSTALLDIR)/init.py; \
	fi; 

	@for plugin in $(OBJS); do \
		cp $(BUILDDIR)/nix/$$plugin $(INSTALLDIR)/plugins/$$plugin ; \
	done


clean:
	rm -rf ./src/*.os ./build/*.so
	test 	rm $(BUILDDIR)/init.py
	rm $(BUILDDIR)/menu.py
