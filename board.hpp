
#ifndef BOARD_H
#define BOARD_H

#include <cstdio>
#include <cstring>

#define STATIC inline static __attribute__((always_inline))

typedef unsigned hash_t;
typedef int index_t;
enum stone_t {
    STONE_EMPTY = 0,
    STONE_BLACK = 1,
    STONE_WHITE = 2,
    STONE_BORDER = 3
};

STATIC char stone2char(stone_t type)
{
    static const char mapping[] = ".XO#";
    return mapping[type];
}

#define IS_STONE(S) (((S) == STONE_BLACK) || ((S) == STONE_WHITE))

const index_t max_size = 13;
const index_t max_len = (max_size + 1) * (max_size + 2) + 1;

#include "nbr3x3.hpp"

struct board_t {

    index_t size;
    index_t len;

    hash_t hash;
    index_t ko_pos;
    stone_t ko_color;

    stone_t stones[max_len];

    // cyclic linked list representation of a group
    index_t next_in_group[max_len];

    // [0, max_len) represents to base_of_group
    // [max_len, inf) represents pseudo_liberties
    index_t group_info[max_len];
#define base_of_group group_info
#define pseudo_liberties group_info

    // sum/square_sum of pseudo_liberties
    index_t group_liberties_sum[max_len];
    index_t group_liberties_sum_squared[max_len];

    // store all cells in a lists
    // [0, empty_ptr) are empty cells
    // [group_ptr, size^2) are group bases
    index_t list_pos[max_len];
    index_t list[max_len];
    index_t empty_ptr;
    index_t group_ptr;

    nbr3x3_t nbr3x3[max_len];
    index_t atari_of_group[max_len];

    // vis and queue
    index_t vis[max_len];
    index_t vis_cnt;
    index_t queue[max_len];
    index_t queue_cnt;

};

typedef const index_t *pindex_t;

#define LEN(B) ((B)->size + 1)
#define N(B, P) ((P) - LEN(B))
#define S(B, P) ((P) + LEN(B))
#define W(B, P) ((P) - 1)
#define E(B, P) ((P) + 1)
#define NE(B, P) ((P) - LEN(B) + 1)
#define NW(B, P) ((P) - LEN(B) - 1)
#define SE(B, P) ((P) + LEN(B) + 1)
#define SW(B, P) ((P) + LEN(B) - 1)
#define POS(B, X, Y) (((X) + 1) * LEN(B) + ((Y) + 1))
#define GETX(B, P) ((P) / LEN(B) - 1)
#define GETY(B, P) ((P) % LEN(B) - 1)

#define IN_GROUP(B, P, G) ((B)->stones[P] == (B)->stones[G] && get_base((B), (P)) == (G))

void initialize();

void empty_board(board_t *b, index_t size);

void fork_board(board_t *nb, const board_t *b);

index_t gen_move(const board_t *b, stone_t color, bool ko_rule=true);

index_t gen_moves(const board_t *b, stone_t color, index_t *moves, bool ko_rule=true);

void gen_moves_next(const board_t *b, stone_t color, const index_t *ptr, bool ko_rule=true);

bool is_legal_move(const board_t *b, index_t pos, stone_t color, bool ko_rule=true);

void put_stone(board_t *b, index_t pos, stone_t color);

bool check_board(board_t *b); // for testing & debug

void calc_final_score(board_t *b, int &bs, int &ws); // only Chinese rule now

void dump_board(const board_t *b);

#endif
