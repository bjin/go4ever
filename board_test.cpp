

#include "board.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>

unsigned randInt(int a)
{
    return (unsigned)rand() % a;
}

int main()
{
    initialize();
    int tests = 1000;
    int passed = 0;
    srand(time(0));
    for (int i = 0; i < tests; i++) {
        board_t *b = new board_t;
        int size = randInt(max_size - 4) + 5;
        empty_board(b, size);
        int steps = 0;
        bool failed = false;
        for (int it = 0; it < size * size; it++) {
            int tries = 10;
            while (tries > 0) {
                stone_t color = steps % 2 == 0 ? STONE_BLACK : STONE_WHITE;
                index_t pos = gen_move(b, color);
                hash_t hashv = b->hash;
                if (!put_stone(b, pos, color)) {
                    tries --;
                    if (!check_board(b) || hashv != b->hash) {
                        failed = true;
                        break;
                    }
                } else
                    break;
            }
            if (failed || !check_board(b)) {
                failed = true;
                break;
            }
            if (tries <= 0)
                break;
            steps++;
        }
        delete b;
        if (failed) {
            printf("[F]");
        } else {
            printf("[%d]", steps);
            passed++;
        }
    }
    printf("\n");
    printf("passed %d out of %d random tests\n", passed, tests);

    return passed == tests ? 0 : 1;
}
