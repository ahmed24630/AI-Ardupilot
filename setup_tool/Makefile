CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRC := src/main.cpp src/board_detect.cpp src/device_scan.cpp src/config_writer.cpp src/vehicle_profile.cpp
TARGET := ai_pilot_setup

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) config.json
