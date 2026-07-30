CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
TARGET = test_linearizability
SOURCES = test_linearizability.cc

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

trans:
	java -cp /home/wfn/toolbox/tla2tools.jar pcal.trans help_update.tla
	cp help_update.cfg.bkp help_update.cfg

tlc:
	java -Xmx128g -cp /home/wfn/toolbox/tla2tools.jar tlc2.TLC -checkpoint 360 help_update.tla

.PHONY: all clean run
