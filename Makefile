ifeq ($(strip $(CXX)),)
CXX=g++
endif
CXXFLAGS=-std=c++2a -Wall -Wextra -Wconversion -pedantic -g

BUILD_DIR=build/

.PHONY: all clean src test

all: $(BUILD_DIR) simulator

test:
	make -C src test

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

simulator: src
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(wildcard $(BUILD_DIR)*.o)

src:
	make -C src CXXFLAGS="$(CXXFLAGS)"

clean:
	make -C src clean
	make -C src test_clean
	$(RM) simulator
	$(RM) simulator.dSYM
