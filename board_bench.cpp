
#include <cstdio>
#include <cstring>
#include <sys/time.h>

#include "board.hpp"
#include "random.hpp"

const int times = 30000;
const int size = 13;

int main()
{
    initialize();
    timeval *start = new timeval;
    timeval *end = new timeval;
    gettimeofday(start, NULL);
    board_t *b = new board_t;
    fast_srandom(start->tv_sec);
    int bcnt = 0, wcnt = 0;
    long long moves = 0;
    for (int i = 0; i < times; i++) {
        empty_board(b, size);
        int steps = 0;
        bool black_passed = false;
        bool white_passed = false;
        while (!black_passed || !white_passed) {
            stone_t color = steps % 2 == 0 ? STONE_BLACK : STONE_WHITE;
            index_t pos = color == STONE_WHITE && white_passed || 
                color == STONE_BLACK && black_passed ? -1 : gen_move(b, color);
            if (pos >= 0)
                put_stone(b, pos, color);
            if (color == STONE_BLACK) {
                black_passed = pos < 0;
            } else if (color == STONE_WHITE) {
                white_passed = pos < 0;
            }
            steps ++;
        }
        int bs, ws;
        calc_final_score(b, bs, ws);
        if (bs > ws + 6.5)
            bcnt ++;
        else
            wcnt ++;
        moves += steps;
    }
    delete b;
    printf("%d : %d\n", bcnt * 10000 / times, wcnt * 10000 / times);
    gettimeofday(end, NULL);
    double used_time = (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec) * 0.000001;
    printf("%.10lf playouts per second\n", (double)times / used_time);
    printf("%.10lf moves per playout\n", (double)moves / times);
}
