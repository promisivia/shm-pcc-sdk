# Makefile for atomic load benchmark

CXX = g++
CXXFLAGS = -std=c++11 -O0 -pthread -msse4.1
INCLUDES=-I../../../src/shm/include
SRC = $(wildcard *.cc) $(wildcard ../../../src/shm/mempool.cc)
TARGETS = $(SRC:.cc=) 

all: $(TARGETS)

%: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean