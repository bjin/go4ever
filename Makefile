
CXX ?= g++
CXXFLAGS ?= -O2

all: board.o random.o gamma.o

test: board_test board_test_olf terry.olf
	./board_test
	./board_test_olf < terry.olf

bench: board_bench
	./board_bench

board.o: board.cpp board.hpp nbr3x3.hpp
	${CXX} board.cpp -c ${CXXFLAGS}

random.o: random.cpp random.hpp
	${CXX} random.cpp -c ${CXXFLAGS}

gamma.o: gamma.cpp gamma.hpp gamma_data.hpp
	${CXX} gamma.cpp -c ${CXXFLAGS}

board_test: board.o random.o board_test.cpp board.hpp nbr3x3.hpp
	${CXX} board_test.cpp board.o random.o -o board_test ${CXXFLAGS}

board_test_olf: board.o random.o board_test_olf.cpp board.hpp nbr3x3.hpp
	${CXX} board_test_olf.cpp board.o random.o -o board_test_olf ${CXXFLAGS}

board_bench: board.o random.o board_bench.cpp board.hpp nbr3x3.hpp
	${CXX} board_bench.cpp board.o random.o -o board_bench ${CXXFLAGS}

clean:
	rm -rf *.o board_test board_bench board_test_olf
