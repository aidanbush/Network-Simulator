CXX=g++
CXXFLAGS=-Wall -Wextra -pedantic -g -D_TEST

.PHONY: all test_prep clean

all: test
	valgrind ./test

test_prep:
	make -C ../ BUILD_DIR=tests/ CXXFLAGS="$(CXXFLAGS)"

test.o: test.cpp  ../*.h

test: test.o test_prep
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ *.o

clean:
	$(RM) test *.o
