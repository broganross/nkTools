# Compiler Info ('g++-4.1 --version')
# g++-4.1 (GCC) 4.1.2 20080704 (Red Hat 4.1.2-46)
# Copyright (C) 2006 Free Software Foundation, Inc.
# This is free software; see the source for copying conditions.  There is NO
# warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# End Compiler Info Output
NDKDIR ?= /usr/local/Nuke7.0v6# Change this to Nuke's location
MYCXX ?= g++
LINK ?= g++
CXXFLAGS ?= -g -c -DUSE_GLEW -I$(NDKDIR)/include -fPIC -msse 
LINKFLAGS ?= -L$(NDKDIR) 
LIBS ?= -lDDImage
LINKFLAGS += -shared
all: DrivenDilate.so
.PRECIOUS : %.os
%.os: %.cpp
	$(MYCXX) $(CXXFLAGS) -o $(@) $<
%.so: %.os
	$(LINK) $(LINKFLAGS) $(LIBS) -o ~/.nuke/$(@) $<
clean:
	rm -rf *.os DepthErode.so stdRamp.so EdgeErode.so PointColor.so depthDistort.so Normalise.so