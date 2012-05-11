
#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>
#include "board.hpp"

using namespace std;

const index_t size = 9;

bool parse(board_t *b, char ch1, char ch2, index_t &pos, stone_t &color)
{
    assert(isalpha(ch1) && isalpha(ch2));
    assert(isupper(ch1) == isupper(ch2));
    if (ch1 == 't' || ch1 == 'T')
        return false;
    if (isupper(ch1)) {
        ch1 = tolower(ch1);
        ch2 = tolower(ch2);
        color = STONE_BLACK;
    } else {
        color = STONE_WHITE;
    }
    index_t x = ch1 - 'a';
    index_t y = ch2 - 'a';
    assert(0 <= x && x < size);
    assert(0 <= y && y < size);
    pos = POS(b, x, y);
    return true;
}

int main()
{
    initialize();
    int tests = 0, passed = 0;
    string s, t;
    int violated_ko_cnt = 0;
    while (cin >> s) {
        index_t pos;
        stone_t color;
        tests ++;
        for (int i = 0; i < 10; i++)
            cin >> s;
        board_t *b = new board_t;
        empty_board(b, size);
        bool failed = false;
        for (int i = 0; !failed && i + 1 < s.size(); i+=2) {
            if (parse(b, s[i], s[i+1], pos, color)) {
                hash_t prev = b->hash;
                put_stone(b, pos, color, true, true, false);
                if (!check_board || prev != b->hash) {
                    failed = true;
                    break;
                }
                bool res = put_stone(b, pos, color);
                bool checked = check_board(b);
                if (!checked || !res && prev != b->hash) {
                    failed = true;
                    break;
                }
                if (!res) {
                    violated_ko_cnt ++;
                }
                if (!res && !put_stone(b, pos, color, false)) {
                    failed = true;
                    break;
                }
            }
        }
        cin >> t;
        if (failed) {
            cout << "Failed on #" << tests << " " << s << " " << t << endl;
        } else {
            passed ++;
        }
    }
    printf("Violated Ko rules: %d\n", violated_ko_cnt);
    printf("Passed %d out of %d tests\n", passed, tests);
    return passed == tests ? 0 : 1;
}
