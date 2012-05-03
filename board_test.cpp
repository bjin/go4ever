

#include "board.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>


int passed_samples = 0;
int total_samples = 0;

void test_sample(const char *input, const char *expect)
{
    printf("testing [\n%s] [\n%s]\n", input, *expect ? "Failed" : expect);
    total_samples ++;
    board *b = new board;
    empty_board(b, 19);
    int px, py, pc;
    for (int x = 0, y = 0; *input; input++) {
        if (*input <= ' ') {
            x ++;
            y = -1;
        } else if (*input == 'X') {
            if (!put_stone(b, POS(b, x, y), black)) {
                printf("Failed PUT(%d, %d)\n", x, y);
                return;
            }
        } else if (*input == 'O') {
            if (!put_stone(b, POS(b, x, y), white)) {
                printf("Failed PUT(%d, %d)\n", x, y);
                return;
            }
        } else if (*input == 'x') {
            px = x;
            py = y;
            pc = black;
        } else if (*input == 'o') {
            px = x;
            py = y;
            pc = white;
        }
        y++;
    }
    bool expect_succ = *expect;
    bool succ = put_stone(b, POS(b, px, py), pc);
    if (succ != expect_succ) {
        printf("Failed\n");
        return;
    }
    if (!expect_succ) {
        passed_samples ++;
        return;
    }
    for (int x = 0, y = 0; *expect; expect++) {
        if (*input <= ' ') {
            x ++;
            y = -1;
        } else {
            char res = border;
            if (b->color[POS(b, x, y)] == white)
                res = 'O';
            else if (b->color[POS(b, x, y)] == black)
                res = 'X';
            else if (b->color[POS(b, x, y)] == empty)
                res = '.';
            if (res != *expect) {
                printf("Failed %d %d [%c] but expect [%c]\n", x, y, res, *expect);
                return;
            }
        }
        y++;
    }
    printf("Passed :)\n");
    passed_samples++;
    return;
}

void test_samples()
{
    passed_samples = 0;
    total_samples = 0;
    test_sample(
            ".X.\n"
            "XOX\n"
            ".x.\n"
        ,
            ".X.\n"
            "X.X\n"
            ".X.\n");
    test_sample(
            ".X.\n"
            "XoX\n"
            ".X.\n"
        , "");
    test_sample(
            ".XO\n"
            "XoXO\n"
            ".xO\n"
        ,
            ".XO.\n"
            "XO.O\n"
            ".XO.\n");
    test_sample(
            "oXO\n"
            "XO\n"
            "O\n"
        ,
            "O.O\n"
            ".O\n"
            "O\n");
}

unsigned randInt(int a)
{
    return (unsigned)rand() % a;
}

int main()
{
    initialize();
    test_samples();
    int tests = 1000;
    int passed = 0;
    srand(time(0));
    for (int i = 0; i < tests; i++) {
        board *b = new board;
        int size = 19 - randInt(5);
        empty_board(b, size);
        int steps = 0;
        bool failed = false;
        while (true) {
            int x, y;
            while (true) {
                x = randInt(size);
                y = randInt(size);
                if (b->color[POS(b, x, y)] == empty)
                    break;
            }
            if (!put_stone(b, POS(b, x, y), steps % 2 == 0 ? black : white))
                break;
            if (!check_board(b)) {
                failed = true;
                break;
            }
            steps++;
        }
        if (failed) {
            printf("[F]");
        } else {
            printf("[%d]", steps);
            passed++;
        }
    }
    printf("\n");
    printf("passed %d out of %d samples\n", passed_samples, total_samples);
    printf("passed %d out of %d random tests\n", passed, tests);
}
