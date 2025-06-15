HOME=../tests/YCSB-C
include $(HOME)/Defines.mk
SRCDIR=$(HOME)/src/

CC=g++
CFLAGS=-std=c++17 -c -g -Wall -fgnu-tm -msse4.1 -lmemkind -march=native -lssh -g
INCLUDES=-I. -I$(SRCDIR)
SOURCES=$(foreach tree,$(TREES),$(wildcard $(tree)/*.cc))
OBJECTS=$(SOURCES:.cc=.o)

all: $(SOURCES) $(OBJECTS)

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	$(RM) $(OBJECTS)

