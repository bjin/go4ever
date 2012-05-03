
CXX ?= g++
CXXFLAGS ?= -O2

all: board.o

test: board_test
	./board_test

bench: board_bench
	./board_bench

board.o: board.cpp board.hpp
	${CXX} board.cpp -c ${CXXFLAGS}

board_test: board.o board_test.cpp
	${CXX} board_test.cpp board.o -o board_test ${CXXFLAGS}

board_bench: board.o board_bench.cpp
	${CXX} board_bench.cpp board.o -o board_bench ${CXXFLAGS}

clean:
	rm -rf board.o board_test
