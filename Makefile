
CXX ?= g++
CXXFLAGS ?= -O2

all: go4ever

go4ever: board.o random.o gamma.o gtp.o uct.o interface.cpp
	${CXX} board.o random.o gamma.o gtp.o uct.o interface.cpp -o go4ever

test: board_test board_test_olf terry.olf
	./board_test
	./board_test_olf < terry.olf

bench: board_bench
	./board_bench

board.o: board.cpp board.hpp nbr3x3.hpp gamma.hpp
	${CXX} board.cpp -c ${CXXFLAGS}

random.o: random.cpp random.hpp
	${CXX} random.cpp -c ${CXXFLAGS}

gamma.o: gamma.cpp gamma.hpp gamma.data
	${CXX} gamma.cpp -c ${CXXFLAGS}

uct.o: uct.cpp uct.hpp
	${CXX} uct.cpp -c ${CXXFLAGS}

gtp.o: gtp.cpp gtp.hpp
	${CXX} gtp.cpp -c ${CXXFLAGS}

board_test: board.o random.o gamma.o board_test.cpp board.hpp nbr3x3.hpp
	${CXX} board_test.cpp board.o random.o gamma.o -o board_test ${CXXFLAGS}

board_test_olf: board.o random.o gamma.o board_test_olf.cpp board.hpp nbr3x3.hpp
	${CXX} board_test_olf.cpp board.o random.o gamma.o -o board_test_olf ${CXXFLAGS}

board_bench: board.o random.o gamma.o board_bench.cpp board.hpp nbr3x3.hpp
	${CXX} board_bench.cpp board.o random.o gamma.o -o board_bench ${CXXFLAGS}

clean:
	rm -rf *.o board_test board_bench board_test_olf go4ever
