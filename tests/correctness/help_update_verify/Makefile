CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
TARGET = test_linearizability
SOURCES = test_linearizability.cc
TLA2TOOLS_JAR ?=

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

trans:
	@test -n "$(TLA2TOOLS_JAR)" || (echo "Set TLA2TOOLS_JAR to tla2tools.jar"; exit 2)
	java -cp "$(TLA2TOOLS_JAR)" pcal.trans help_update.tla
	cp help_update.cfg.bkp help_update.cfg

tlc:
	@test -n "$(TLA2TOOLS_JAR)" || (echo "Set TLA2TOOLS_JAR to tla2tools.jar"; exit 2)
	java -Xmx4g -cp "$(TLA2TOOLS_JAR)" tlc2.TLC -checkpoint 360 help_update.tla

.PHONY: all clean run
