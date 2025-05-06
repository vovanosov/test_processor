# Makefile for Test Processor
SYSTEMC_HOME ?= /opt/systemc/

# Compiler and Linker settings
CXX ?= g++
CXXFLAGS ?= -I$(SYSTEMC_HOME)/include -std=c++17
LDFLAGS ?= -L$(SYSTEMC_HOME)/lib -lsystemc

# Source files
SRCS = src/test_processor.cpp
OBJS = $(SRCS:.cpp=.o)

# Target executable
TARGET = test_processor

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean