
CXX ?= g++

all: board.o 

test: board_test
	./board_test

board.o: board.cpp board.hpp
	${CXX} board.cpp -c ${CXXFLAGS}

board_test: board.o board_test.cpp
	${CXX} board_test.cpp board.o -o board_test ${CXXFLAGS}


clean:
	rm -rf board.o board_test
