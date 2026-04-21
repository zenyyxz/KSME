CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3 -Iinclude -Isrc

SRC = src/main.cpp src/core/framebuffer.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = math_visualizer

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
