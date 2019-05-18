CXX = g++
STRIP = strip
CXXFLAGS = -Wall -Wextra -Wconversion -std=c++17 -O2 -g
BINARY = wav2agb
LIBS =

SRC_FILES = $(wildcard *.cpp)
OBJ_FILES = $(SRC_FILES:.cpp=.o)


all: $(BINARY)
	

.PHONY: clean
clean:
	rm -f $(OBJ_FILES)

$(BINARY): $(OBJ_FILES)
	$(CXX) -o $@ $^ $(LIBS)
	#$(STRIP) -s $@

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)
