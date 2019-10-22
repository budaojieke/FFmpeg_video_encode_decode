CXX      = g++
AR       = ar
CXXFLAGS = -std=c++11 -Wall -fPIC
LDFLAGS  = -pthread
ROOT_DIR  = $(shell pwd)

TARGET      = encode
sources     = $(wildcard *.cpp)
objects     = $(sources:.cpp=.o)
dependence := $(sources:.cpp=.d)


%.d: %.cpp
	@set -e; rm -f $@; \
	echo -e $(NOTICE_COLOR)"Generating dependencies for $(shell pwd)/$< ..."$(RESTORE_COLOR); \
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

SHARED_LIBS = 
STATIC_LIBS = 

CXXFLAGS   +=  -I/FFmpeg
LDFLAGS    +=  -L lib
LDFLAGS    +=  -Bstatic -lavdevice -lavfilter -lavformat -lavcodec -lavutil -lswscale -lswresample -lz -llzma -lx264 -lx265

all: compile link

compile: $(objects)

link: $(objects)
	$(CXX) $^ $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(objects) $(dependence)
	rm -f *.o
.PHONY: clean
