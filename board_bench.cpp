
#include <cstdio>
#include <cstring>
#include <sys/time.h>

#include "board.hpp"
#include "random.hpp"

const int times = 100000;
const int size = 13;

int main()
{
    timeval *start = new timeval;
    timeval *end = new timeval;
    gettimeofday(start, NULL);
    board_t *b = new board_t;
    fast_srandom(4423);
    int bcnt = 0, wcnt = 0;
    for (int i = 0; i < times; i++) {
        empty_board(b, size);
        int steps = 0;
        while (true) {
            int tries = 10;
            while (tries > 0) {
                stone_t color = steps % 2 == 0 ? STONE_BLACK : STONE_WHITE;
                index_t pos = gen_move(b, color);
                if (put_stone(b, pos, color)) {
                    break;
                }
                tries --;
            }
            if (tries <= 0)
                break;
            steps ++;
        }
        int bs, ws;
        calc_final_score(b, bs, ws);
        if (bs > ws + 2.5)
            bcnt ++;
        else
            wcnt ++;
    }
    delete b;
    printf("%d : %d\n", bcnt, wcnt);
    gettimeofday(end, NULL);
    double used_time = (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec) * 0.000001;
    printf("%.10lf playouts per second\n", 1.0 / (used_time / times));
}
