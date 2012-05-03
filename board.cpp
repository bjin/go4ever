
#include "board.hpp"

#include <cstring>
#include <cstdio>

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
    b->prev_pos = 0;
    b->prev_color = empty;

    memset(b->color, border, sizeof(*b->color) * b->len);
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            b->color[POS(b, i, j)] = empty;
        }
}

void fork_board(const board *b, board *nb)
{
    nb->size = b->size;
    nb->len = b->len;
    nb->white_captured = b->white_captured;
    nb->black_captured = b->black_captured;
    nb->hash = b->hash;
    nb->prev_hash = b->prev_hash;
    nb->prev_pos = b->prev_pos;
    nb->prev_color = b->prev_color;
    memcpy(nb->color, b->color, sizeof(*b->color)*b->len);
    memcpy(nb->next_in_group, b->next_in_group, sizeof(*b->next_in_group)*b->len);
    memcpy(nb->group_info, b->group_info, sizeof(*b->group_info)*b->len);
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

inline static void try_delete_group(board *b, int group)
{
    if (b->pseudo_liberties[group] != max_len)
        return;
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
        captured_other = b->pseudo_liberties[get_base(b, N(b, pos))] == max_len;
    }
    if (!captured_other && b->color[S(b, pos)] == oppocolor) {
        captured_other |= b->pseudo_liberties[get_base(b, S(b, pos))] == max_len;
    }
    if (!captured_other && b->color[W(b, pos)] == oppocolor) {
        captured_other |= b->pseudo_liberties[get_base(b, W(b, pos))] == max_len;
    }
    if (!captured_other && b->color[E(b, pos)] == oppocolor) {
        captured_other |= b->pseudo_liberties[get_base(b, E(b, pos))] == max_len;
    }
    if (!captured_other) {
        // check if it's a suicide or not. if so, revert back and return false
        int current_pseudo_liberties = b->pseudo_liberties[pos] - max_len;
        int baseN = -1, baseS = -1, baseW = -1, baseE = -1;
        if (b->color[N(b, pos)] == color) {
            baseN = get_base(b, pos);
            current_pseudo_liberties += b->pseudo_liberties[baseN] - max_len;
        }
        if (b->color[S(b, pos)] == color) {
            baseS = get_base(b, pos);
            if (baseS != baseN)
                current_pseudo_liberties += b->pseudo_liberties[baseS] - max_len;
        }
        if (b->color[W(b, pos)] == color) {
            baseW = get_base(b, pos);
            if (baseW != baseN && baseW != baseS)
                current_pseudo_liberties += b->pseudo_liberties[baseW] - max_len;
        }
        if (b->color[E(b, pos)] == color) {
            baseE = get_base(b, pos);
            if (baseE != baseN && baseE != baseS && baseE != baseW)
                current_pseudo_liberties += b->pseudo_liberties[baseE] - max_len;
        }
        if (current_pseudo_liberties == 0) {
            // surely it's a suicide, revert back
            delete_stone_update_pseudo_liberties(b, pos);
            b->color[pos] = empty;
            b->hash -= p4423[pos] * color;
            return false;
        }
    }

    if (b->color[N(b, pos)] == oppocolor) {
        try_delete_group(b, get_base(b, N(b, pos)));
    }
    if (b->color[S(b, pos)] == oppocolor) {
        try_delete_group(b, get_base(b, S(b, pos)));
    }
    if (b->color[W(b, pos)] == oppocolor) {
        try_delete_group(b, get_base(b, W(b, pos)));
    }
    if (b->color[E(b, pos)] == oppocolor) {
        try_delete_group(b, get_base(b, E(b, pos)));
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

    try_delete_group(b, get_base(b, pos));

    // check repetition
    
    if (b->prev_hash == b->hash) {
        // it's a repetition, revert back by redo last move

        b->prev_hash = -1; // to avoid infinite loops
        put_stone(b, b->prev_pos, b->prev_color);
        return false;
    }
    
    //record information
    b->prev_hash = recorded_hash;
    b->prev_pos = pos;
    b->prev_color = color;

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

void dump_board(const board *b)
{
    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            char color = b->color[POS(b, i, j)];
            if (color == empty)
                putchar('.');
            else if (color == black)
                putchar('X');
            else if (color == white)
                putchar('O');
            else
                putchar('E');
        }
        putchar('\n');
    }
}
