
#include "board.hpp"

#include <cstring>

static hashtype p4423[max_len];

void initialize()
{
    p4423[0] = 1;
    for (int i = 1; i < max_len; i++) {
        p4423[i] = p4423[i - 1] * 4423;
    }
}

void empty_board(board *b, int size)
{
    b->size = size;
    b->len = (size + 1) * (size + 2) + 1;
    b->white_captured = 0;
    b->black_captured = 0;
    b->hash = 0;
    b->prev_hash = -1;

    memset(b->color, border, sizeof(*b->color) * b->len);
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            b->color[POS(b, i, j)] = empty;
        }
}

void fork_board(const board *b, board *nb)
{
    memcpy(nb, b, sizeof(board));
}

// {{{ help function for put_stone()

inline static int get_base(board *b, int pos)
{
    if (b->base_of_group[pos] >= max_len)
        return pos;
    int r = pos;
    while (b->base_of_group[r] < max_len) {
        r = b->base_of_group[r];
    }
    while (pos != r) {
        int fpos = b->base_of_group[pos];
        b->base_of_group[pos] = r;
        pos = fpos;
    }
    return r;
}

inline static int get_empty_neighbour(board *b, int pos)
{
    int ret = 0;
    if (b->color[N(b, pos)] == empty) {
        ret ++;
    }
    if (b->color[S(b, pos)] == empty) {
        ret ++;
    }
    if (b->color[W(b, pos)] == empty) {
        ret ++;
    }
    if (b->color[E(b, pos)] == empty) {
        ret ++;
    }
    return ret;
}

inline static void add_stone_update_pseudo_liberties(board *b, int pos)
{
    if (b->color[N(b, pos)] > empty) {
        b->pseudo_liberties[get_base(b, N(b, pos))] --;
    }
    if (b->color[S(b, pos)] > empty) {
        b->pseudo_liberties[get_base(b, S(b, pos))] --;
    }
    if (b->color[W(b, pos)] > empty) {
        b->pseudo_liberties[get_base(b, W(b, pos))] --;
    }
    if (b->color[E(b, pos)] > empty) {
        b->pseudo_liberties[get_base(b, E(b, pos))] --;
    }
}

inline static void delete_stone_update_pseudo_liberties(board *b, int pos)
{
    if (b->color[N(b, pos)] > empty) {
        b->pseudo_liberties[get_base(b, N(b, pos))] ++;
    }
    if (b->color[S(b, pos)] > empty) {
        b->pseudo_liberties[get_base(b, S(b, pos))] ++;
    }
    if (b->color[W(b, pos)] > empty) {
        b->pseudo_liberties[get_base(b, W(b, pos))] ++;
    }
    if (b->color[E(b, pos)] > empty) {
        b->pseudo_liberties[get_base(b, E(b, pos))] ++;
    }
}

inline static bool try_delete_group(board *b, int group)
{
    if (b->pseudo_liberties[group] != max_len)
        return false;
    int ptr = group;
    do {
        if (b->color[ptr] == white) {
            b->white_captured ++;
        } else {
            b->black_captured ++;
        }
        b->hash -= b->color[ptr] * p4423[ptr];
        b->color[ptr] = empty;
        delete_stone_update_pseudo_liberties(b, ptr);
        ptr = b->next_in_group[ptr];
    } while (ptr != group);
    return true;
}

inline static void try_merge_group(board *b, int p, int q)
{
    if (p == q)
        return;
    if (b->color[p] != b->color[q])
        return;
    //merge cyclic linked list
    int tmp = b->next_in_group[p];
    b->next_in_group[p] = b->next_in_group[q];
    b->next_in_group[q] = tmp;
    //link q into p
    b->pseudo_liberties[p] += b->pseudo_liberties[q] - max_len;
    b->base_of_group[q] = p;
}
// }}}

bool put_stone(board *b, int pos, char color)
{
    // basic checks

    if (pos < 0 || pos >= b->len) {
        return false;
    }
    if (b->color[pos] != empty) {
        return false;
    }

    hashtype recorded_hash = b->hash;

    // put a stone 

    b->color[pos] = color;
    b->next_in_group[pos] = pos;
    b->group_info[pos] = max_len + get_empty_neighbour(b, pos);
    b->hash += p4423[pos] * color;

    // update neighbour group's pseudo_liberties

    add_stone_update_pseudo_liberties(b, pos);

    // try to capture others
    bool captured_other = false;

    int oppocolor = color == white ? black : white;
    if (b->color[N(b, pos)] == oppocolor) {
        captured_other |= try_delete_group(b, get_base(b, N(b, pos)));
    }
    if (b->color[S(b, pos)] == oppocolor) {
        captured_other |= try_delete_group(b, get_base(b, S(b, pos)));
    }
    if (b->color[W(b, pos)] == oppocolor) {
        captured_other |= try_delete_group(b, get_base(b, W(b, pos)));
    }
    if (b->color[E(b, pos)] == oppocolor) {
        captured_other |= try_delete_group(b, get_base(b, E(b, pos)));
    }

    // merge with friendly neighboour stones's group

    if (b->color[N(b, pos)] == color) {
        try_merge_group(b, get_base(b, N(b, pos)), get_base(b, pos));
    }
    if (b->color[S(b, pos)] == color) {
        try_merge_group(b, get_base(b, S(b, pos)), get_base(b, pos));
    }
    if (b->color[W(b, pos)] == color) {
        try_merge_group(b, get_base(b, W(b, pos)), get_base(b, pos));
    }
    if (b->color[E(b, pos)] == color) {
        try_merge_group(b, get_base(b, E(b, pos)), get_base(b, pos));
    }

    // check suicide

    if (try_delete_group(b, get_base(b, pos)) && !captured_other)
        return false;

    // check repetition
    
    if (b->prev_hash == b->hash)
        return false;
    b->prev_hash = recorded_hash;

    return true;
}

bool check_board(board *b)
{
    static bool flag[max_len];
    memset(flag, false, sizeof(bool) * max_len);
    for (int i = 0; i < b->len; i++) {
        if (b->color[i] > empty) {
            flag[b->next_in_group[i]] = true;
            //check next[i] in the same group
            if (get_base(b, i) != get_base(b, b->next_in_group[i]))
                return false;
            //check same color
            if (b->color[i] != b->color[b->next_in_group[i]])
                return false;
        }
    }
    // check uniqueness of next[i]
    for (int i = 0; i < b->len; i++) {
        if ((b->color[i] > empty) != flag[i]) {
            return false;
        }
    }
    static int recalc_ps_lib[max_len];
    memset(recalc_ps_lib, 0, sizeof(int) * max_len);
    for (int i = 0; i < b->len; i++) {
        if (b->color[i] > empty) {
            recalc_ps_lib[get_base(b, i)] += get_empty_neighbour(b, i);
        }
    }
    // check pseudo_liberties count
    for (int i = 0; i < b->len; i++) {
        if (b->color[i] > empty && b->group_info[i] >= max_len) {
            if (b->pseudo_liberties[i] - max_len != recalc_ps_lib[i])
                return false;
        }
    }
    // check hash value
    hashtype nhash = 0;
    for (int i = 0; i < b->len; i++) {
        if (b->color[i] > empty)
            nhash += b->color[i] * p4423[i];
    }
    if (nhash != b->hash)
        return false;
    return true;
}

void calc_final_score(board *b, int *bs, int *ws, bool *score_vis, int *score_queue)
{
    *bs = 0;
    *ws = 0;
    int head, tail;
    memset(score_vis, false, sizeof(bool) * b->len);
    for (int i = S(b, 0); S(b, i) < b->len; i++) {
        if (b->color[i] == border)
            continue;
        if (score_vis[i])
            continue;
        if (b->color[i] == black) {
            *bs++;
            continue;
        } else if (b->color[i] == white) {
            *ws++;
            continue;
        }
        // use BFS to check the color of reachable 
        head = tail = 0;
        score_queue[tail++] = i;
        score_vis[i] = true;
        bool reachB = false;
        bool reachW = false;
        while (head < tail) {
            int p = score_queue[head++];
#define EXPAND(Q) \
            if (b->color[Q] == black) { \
                reachB = true; \
            } else if (b->color[Q] == white) { \
                reachW = true; \
            } else if (b->color[Q] == empty && !score_vis[Q]) { \
                score_vis[Q] = true; \
                score_queue[tail++] = Q; \
            }
            EXPAND(N(b, p));
            EXPAND(S(b, p));
            EXPAND(W(b, p));
            EXPAND(E(b, p));
        }
        if (reachB && !reachW) {
            *bs += tail;
        } else if (!reachB && reachW) {
            *ws += tail;
        }
    }
}
