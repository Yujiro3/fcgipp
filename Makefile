PROGRAM  = fcgicli
TARGETS  = main fcgipp
DEP      = .depend
SRCS     = $(TARGETS:%=%.cpp)
OBJS     = $(addsuffix .o, $(basename $(SRCS)))

CXXFLAGS = -Wall -g
CXXLIBS  = -lfcgi
CXX      = g++ -O2

all: dep $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(CXXLIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

dep:
ifeq ($(DEP),$(wildcard $(DEP)))
-include $(DEP)
else
	$(CXX) -MM $(CXXFLAGS) $(SRCS) > $(DEP)
endif

clean:
	rm -f $(PROGRAM) $(OBJS) $(DEP)

