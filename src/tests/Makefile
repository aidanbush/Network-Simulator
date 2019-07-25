CXX=g++
CXXFLAGS=-std=c++2a -Wall -Wextra -Wconversion -pedantic -g
DEBUG_FLAGS=-D_TEST

.PHONY: all test_prep clean

all: test
	valgrind ./test

test_prep:
	make -C ../ BUILD_DIR=tests/ CXXFLAGS="$(CXXFLAGS) $(DEBUG_FLAGS)"

throwAssert.o: throwAssert.cpp throwAssert.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) -o $@ -c $<

test.o: test.cpp ../*.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) -o $@ -c $<

test: test_prep test.o throwAssert.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) -o $@ *.o

clean:
	$(RM) test *.o && $(RM) -r test.dSYM
