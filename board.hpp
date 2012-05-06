
#ifndef BOARD_H
#define BOARD_H

typedef unsigned long long hashtype;

const int max_size = 19;
const int max_len = (max_size + 1) * (max_size + 2) + 1;

const char border = -1;
const char empty = 0;
const char black = 1;
const char white = 2;

typedef struct board_t {

    int size;
    int len;

    hashtype hash;

    hashtype prev_hash;
    int prev_pos;
    char prev_color;
    
    char color[max_len];

    int next_in_group[max_len];
    int group_info[max_len]; // [0, max_len) represents to base_of_group
                             // [max_len, inf) represents pseudo_liberties

} board;

#define base_of_group group_info
#define pseudo_liberties group_info

#define N(B, P) ((P) - LEN(B))
#define S(B, P) ((P) + LEN(B))
#define W(B, P) ((P) - 1)
#define E(B, P) ((P) + 1)
#define LEN(B) ((B)->size + 1)
#define POS(B, X, Y) (((X) + 1) * LEN(B) + ((Y) + 1))
#define GETX(B, P) ((P) / LEN(B) - 1)
#define GETY(B, P) ((P) % LEN(B) - 1)

void initialize();

void empty_board(board *b, int size);

void fork_board(const board *b, board *nb);

bool put_stone(board *b, int pos, char color);

bool check_board(board *b); // for test & debug

void calc_final_score(board *b, int *bs, int *ws, bool *score_vis, int *score_queue); // only Chinese rule now

void dump_board(const board *b);

#endif
