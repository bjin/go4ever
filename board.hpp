
#ifndef BOARD_H
#define BOARD_H

typedef unsigned hash_t;
typedef int index_t;
enum stone_t {
    STONE_BORDER,
    STONE_EMPTY,
    STONE_BLACK,
    STONE_WHITE,
    STONE_MAX
};

const index_t max_size = 19;
const index_t max_len = (max_size + 1) * (max_size + 2) + 1;

struct board_t {

    index_t size;
    index_t len;

    hash_t hash;

    hash_t prev_hash;
    index_t prev_pos;
    stone_t prev_color;

    stone_t stones[max_len];

    // cyclic linked list representation of a group
    index_t next_in_group[max_len];

    // [0, max_len) represents to base_of_group
    // [max_len, inf) represents pseudo_liberties
    index_t group_info[max_len];
#define base_of_group group_info
#define pseudo_liberties group_info

    // xor sum of pseudo_liberties
    index_t group_liberties_xor[max_len];
};


#define N(B, P) ((P) - LEN(B))
#define S(B, P) ((P) + LEN(B))
#define W(B, P) ((P) - 1)
#define E(B, P) ((P) + 1)
#define LEN(B) ((B)->size + 1)
#define POS(B, X, Y) (((X) + 1) * LEN(B) + ((Y) + 1))
#define GETX(B, P) ((P) / LEN(B) - 1)
#define GETY(B, P) ((P) % LEN(B) - 1)

void initialize();

void empty_board(board_t *b, index_t size);

void fork_board(board_t *nb, const board_t *b);

bool put_stone(board_t *b, index_t pos, stone_t color);

bool check_board(board_t *b); // for testing & debug

void calc_final_score(board_t *b, int *bs, int *ws, bool *score_vis, int *score_queue); // only Chinese rule now

void dump_board(const board_t *b);

#endif
