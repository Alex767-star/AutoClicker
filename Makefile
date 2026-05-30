CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall
LDFLAGS = -lX11 -lXtst -pthread

TARGET = autoclicker
SRC = autoclicker.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: clean run
