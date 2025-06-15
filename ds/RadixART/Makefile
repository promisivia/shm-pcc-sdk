# Makefile for ARTSynchronized project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -march=native -g
LIBS = -ltbb -lmemkind -lnuma -pthread
CXXFLAGS += -I. -I../

# Source files
ART_FILES = OptimisticLockCoupling/Tree.cpp ROWEX/Tree.cpp ART/Tree.cpp \
            ../shm/mm.cc ../shm/memkind.cc ../shm/mm.cc \
            ../connection/establish.cc ../utils/init.cc
SOURCE_FILES = example.cpp

OBJ = $(ART_FILES:.cpp=.o) $(ART_FILES:.cc=.o) $(SOURCE_FILES:.cpp=.o)

# Output binary names
LIB_NAME = libARTSynchronized.a
EXEC_NAME = example

# Build rules
all: $(LIB_NAME) $(EXEC_NAME)

$(LIB_NAME): $(OBJ)
	ar rcs $@ $(OBJ)

$(EXEC_NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ -L. -lARTSynchronized $(LIBS)

# Clean rule
clean:
	rm -f $(LIB_NAME) $(EXEC_NAME) *.o

.PHONY: all clean
