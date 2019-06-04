CXX=g++

.PHONY: all clean

all: build simulator

build:
	mkdir build

simulator: build/simulator.o
	g++ -o simulator build/simulator.o

build/simulator.o: src/simulator.cpp
	g++ src/simulator.cpp -o build/simulator.o -c

clean:
	rm -f build/* simulator
