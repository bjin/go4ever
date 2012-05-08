
CXX ?= g++
CXXFLAGS ?= -O2

all: board.o random.o

test: board_test
	./board_test

bench: board_bench
	./board_bench

board.o: board.cpp board.hpp
	${CXX} board.cpp -c ${CXXFLAGS}

random.o: random.cpp random.hpp
	${CXX} random.cpp -c ${CXXFLAGS}

board_test: board.o random.o board_test.cpp
	${CXX} board_test.cpp board.o random.o -o board_test ${CXXFLAGS}

board_bench: board.o random.o board_bench.cpp
	${CXX} board_bench.cpp board.o random.o -o board_bench ${CXXFLAGS}

clean:
	rm -rf *.o board_test board_bench
