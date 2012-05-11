
#include "board.hpp"
#include "random.hpp"

#include <cstdio>
#include <ctime>

int main()
{
    initialize();
    int tests = 1000;
    int passed = 0;
    fast_srandom(time(0));
    for (int i = 0; i < tests; i++) {
        board_t *b = new board_t;
        int size = fast_random(max_size - 4) + 5;
        empty_board(b, size);
        int steps = 0;
        bool failed = false;
        while (true) {
            stone_t color = steps % 2 == 0 ? STONE_BLACK : STONE_WHITE;
            index_t pos = gen_move(b, color);
            if (pos < 0)
                break;
            if (!is_legal_move(b, pos, color)) {
                failed = true;
                break;
            }
            if (pos >= 0) {
                put_stone(b, pos, color);
                if (!check_board(b)) {
                    failed = true;
                    break;
                }
            }
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
    printf("Passed %d out of %d random tests\n", passed, tests);

    return passed == tests ? 0 : 1;
}
