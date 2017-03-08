#

PYLON_ROOT = /opt/pylon5

LD         := $(CXX)
CPPFLAGS   := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags)
CXXFLAGS   := #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs)
WPLFLAGS   := -lwiringPi -lpthread
CVLFLAGS   := -lopencv_core -lopencv_imgproc -lopencv_highgui 

all: Photobooth ServoTest CameraTest GPIOTest ArduinoTest DispenserTest OpenCVTest

Photobooth: Photobooth.o
	 $(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

Photobooth.o: Photobooth.cpp
	 $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

CameraTest: CameraTest.o
	 $(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(CVLFLAGS)

CameraTest.o: CameraTest.cpp
	 $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

ServoTest: ServoTest.o
	 $(LD) -o $@ $^

ServoTest.o: ServoTest.c
	 $(CXX) -c -o $@ $<

DispenserTest: DispenserTest.o
	$(LD) -o $@ $^

DispenserTest.o: DispenserTest.c
	$(CXX) -c -o $@ $<

ArduinoTest: ArduinoTest.o
	$(LD) -o $@ $^

ArduinoTest.o: ArduinoTest.c
	$(CXX) -c -o $@ $<

OpenCVTest: OpenCVTest.o
	$(LD) -o $@ $^ $(CVLFLAGS)

OpenCVTest.o: OpenCVTest.cpp
	$(CXX) -c -o $@ $<

GPIOTest: GPIOTest.o
	$(LD) -o $@ $^ $(WPLFLAGS)

GPIOTest.o: GPIOTest.c
	$(CXX) -c -o $@ $<

clean:
	 $(RM) *.o Photobooth ServoTest CameraTest GPIOTest ArduinoTest DispenserTest
