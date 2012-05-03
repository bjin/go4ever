
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/time.h>

#include "board.hpp"

const int times = 10000;
const int size = 13;

unsigned randInt(int a)
{
    return (unsigned)rand() % a;
}

int main()
{
    timeval *start = new timeval;
    timeval *end = new timeval;
    gettimeofday(start, NULL);
    board *b = new board;
    srand(4423);
    for (int i = 0; i < times; i++) {
        empty_board(b, size);
        int steps = 0;
        while (true) {
            int tries = 100;
            while (tries > 0) {
                int x = randInt(size);
                int y = randInt(size);
                if (put_stone(b, POS(b, x, y), steps % 2 == 0 ? black : white)) {
                    break;
                }
                tries --;
            }
            if (tries <= 0)
                break;
            steps ++;
        }
    }
    delete b;
    gettimeofday(end, NULL);
    double used_time = (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec) * 0.000001;
    printf("%.10lf game simulation per second\n", 1.0 / (used_time / times));
}
