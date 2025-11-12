CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
TARGET = test_help_update
SOURCES = test_help_update.cc

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run



