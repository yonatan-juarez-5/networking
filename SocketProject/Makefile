# compiler
CXX = g++

# compiler flags
CXXFLAGS = -Wall -w -g

# source files
SRCS = serverM.cpp serverC.cpp serverRTH.cpp serverEEB.cpp client.cpp

# object files
OBJS = $(SRCS:.cpp=.o)

# exe name
EXECS = serverM serverC serverRTH serverEEB client

# default target
all: $(EXECS)

# Rule to create each executable from its corresponding source file
serverM: serverM.o
	$(CXX) $(CXXFLAGS) -o $@ $<

serverC: serverC.o
	$(CXX) $(CXXFLAGS) -o $@ $<

serverRTH: serverRTH.o
	$(CXX) $(CXXFLAGS) -o $@ $<

serverEEB: serverEEB.o
	$(CXX) $(CXXFLAGS) -o $@ $<

client: client.o
	$(CXX) $(CXXFLAGS) -o $@ $<

# Rule to compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f *.o $(EXECS)

.PHONY: all clean