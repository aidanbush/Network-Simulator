CXX=g++

.PHONY: all clean src

all: build src simulator

build:
	mkdir build

simulator: build/*.o src

src:
	make -C src

clean:
	$(RM) build/* simulator
