CXX=g++
CXXFLAGS=-Wall -Wextra -pedantic -g

BUILD_DIR=build/

.PHONY: all clean src

all: $(BUILD_DIR) simulator

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

simulator: src
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(wildcard $(BUILD_DIR)*.o)

src:
	make -C src

clean:
	make -C src clean
	$(RM) simulator
