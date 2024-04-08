all: world

# requires libusb-compat

CXX?=g++
CXXFLAGS?=--std=c++23 -Os -Wall -fPIC -g
LDFLAGS?=-L/lib -L/usr/lib

INCLUDES+= -I./include

#INCLUDES+= -I./include -I./jsoncpp/include
#LIBS:=-lubox -lubus -lblobmsg_json

LIBS:=-lgd -lusb
#for ubuntu:

include cpu/Makefile.inc
include mem/Makefile.inc
include process/Makefile.inc
include netinfo/Makefile.inc
include uptime/Makefile.inc
include common/Makefile.inc
include logger/Makefile.inc
include throws/Makefile.inc
include signal/Makefile.inc
include expr/Makefile.inc

include Makefile.drivers
include Makefile.plugins
include Makefile.widgets
include Makefile.actions

OBJS:= \
	objs/fs_funcs.o \
	objs/rgb.o \
	objs/rect.o \
	objs/config.o \
	objs/properties.o \
	objs/display.o \
	objs/timer.o \
	objs/layout.o \
	objs/scheduler.o \
	objs/main.o

world: lcd2

$(shell mkdir -p objs)

objs/fs_funcs.o: src/fs_funcs.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/rgb.o: src/rgb.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/rect.o: src/rect.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/config.o: src/config.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/properties.o: src/properties.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/display.o: src/display.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/timer.o: src/timer.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/layout.o: src/layout.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/scheduler.o: src/scheduler.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

lcd2: $(COMMON_OBJS) $(LOGGER_OBJS) $(THROWS_OBJS) \
	$(NETINFO_OBJS) $(CPU_OBJS) $(MEM_OBJS) $(PROCESS_OBJS) \
	$(UPTIME_OBJS) $(SIGNAL_OBJS) $(EXPR_OBJS) \
	$(OBJS) $(DRIVERS) $(PLUGINS) $(WIDGETS) $(ACTIONS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) $^ -o $@;

.PHONY: clean
clean:
	@rm -rf objs
	@rm -f lcd2
