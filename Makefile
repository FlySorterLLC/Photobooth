#

PYLON_ROOT = /opt/pylon5

LD         := $(CXX)
CPPFLAGS   := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags)
CXXFLAGS   := #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs)

all: Photobooth ServoTest CameraTest

Photobooth: Photobooth.o
	 $(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

Photobooth.o: Photobooth.cpp
	 $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

CameraTest: CameraTest.o
	 $(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

CameraTest.o: CameraTest.cpp
	 $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

ServoTest: ServoTest.o
	 $(LD) -o $@ $^

ServoTest.o: ServoTest.c
	 $(CXX) -c -o $@ $<

clean:
	 $(RM) *.o Photobooth ServoTest CameraTest
