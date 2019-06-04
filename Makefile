all:
	g++ src/simulator.cpp -o simulator

clean:
	rm -f build/* simulator
