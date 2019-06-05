CXX=g++

BUILD_DIR=build/

.PHONY: all clean src

all: $(BUILD_DIR) simulator

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

simulator: src
	$(CXX) -o $@ $(wildcard $(BUILD_DIR)*.o)

src:
	make -C src

clean:
	make -C src clean
	$(RM) simulator
