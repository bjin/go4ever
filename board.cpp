
#include "board.hpp"

#include <cstring>

static hashtype p4423[max_len];

void initialize()
{
    p4423[0] = 1;
    for (int i = 0; i < max_len; i++) {
        p4423[i + 1] = p4423[i] * 4423;
    }
}

void empty_board(board *b, int size)
{
    b->size = size;
    b->len = (size + 1) * (size + 2) + 1;
    b->white_captured = 0;
    b->black_captured = 0;
    b->hash = 0;

    memset(b->color, border, sizeof(*b->color) * b->len);
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            b->color[POS(b, i, j)] = empty;
        }

    memset(b->next_in_group, -1, sizeof(*b->next_in_group) * b->len);
    memset(b->base_of_group, -1, sizeof(*b->base_of_group) * b->len);
    b->group_count = 0;
    b->groups = new group[1];
}

void fork_board(const board *b, board *nb)
{
    memcpy(nb, b, sizeof(board));
    nb->groups = new group[b->group_count + 1];
    memcpy(nb->groups, b->groups, sizeof(group) * nb->group_count);
}

void free_board(board *b)
{
    delete[] b->groups;
    delete b;
}

static int group_index[max_size]; // will break in multi-thread environment

inline static int get_base(board *b, int pos)
{
    if (b->base_of_group[pos] == -1)
        return pos;
    int r = pos;
    while (b->base_of_group[r] != -1) {
        r = b->base_of_group[r];
    }
    while (pos != r) {
        int fpos = b->base_of_group[pos];
        b->base_of_group[pos] = r;
        pos = fpos;
    }
    return r;
}

#define GROUP(B, P) ((B)->groups + group_index[get_base(B, P)])

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
        GROUP(b, N(b, pos))->pseudo_liberties --;
    }
    if (b->color[S(b, pos)] > empty) {
        GROUP(b, S(b, pos))->pseudo_liberties --;
    }
    if (b->color[W(b, pos)] > empty) {
        GROUP(b, W(b, pos))->pseudo_liberties --;
    }
    if (b->color[E(b, pos)] > empty) {
        GROUP(b, E(b, pos))->pseudo_liberties --;
    }
}

inline static void delete_stone_update_pseudo_liberties(board *b, int pos)
{
    if (b->color[N(b, pos)] > empty) {
        GROUP(b, N(b, pos))->pseudo_liberties ++;
    }
    if (b->color[S(b, pos)] > empty) {
        GROUP(b, S(b, pos))->pseudo_liberties ++;
    }
    if (b->color[W(b, pos)] > empty) {
        GROUP(b, W(b, pos))->pseudo_liberties ++;
    }
    if (b->color[E(b, pos)] > empty) {
        GROUP(b, E(b, pos))->pseudo_liberties ++;
    }
}

inline static bool try_delete_group(board *b, group *group)
{
    if (group->pseudo_liberties > 0)
        return false;
    int ptr = group->base;
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
    } while (ptr != group->base);
    group->base = -1;
    return true;
}

inline static void try_merge_group(board *b, group *p, group *q)
{
    if (p->base == q->base)
        return;
    if (b->color[p->base] != b->color[q->base])
        return;
    b->base_of_group[q->base] = p->base;
    //merge cyclic linked list
    int tmp = b->next_in_group[p->base];
    b->next_in_group[p->base] = b->next_in_group[q->base];
    b->next_in_group[q->base] = tmp;
    //delete q
    q->base = -1;
    p->pseudo_liberties += q->pseudo_liberties;
}

bool can_put_stone(board *b, int pos, char color)
{
    if (pos < 0 || pos >= b->len) {
        return false;
    }
    if (b->color[pos] != empty) {
        return false;
    }
    //TODO
}

bool put_stone(board *b, int pos, char color)
{
    // basic checks

    if (pos < 0 || pos >= b->len) {
        return false;
    }
    if (b->color[pos] != empty) {
        return false;
    }

    // build index map from board to groups

    for (int i = 0; i < b->group_count; i++)
        group_index[b->groups[i].base] = i;
    
    hashtype recorded_hash = b->hash;

    // put a stone 

    b->color[pos] = color;
    b->base_of_group[pos] = -1; // self
    b->next_in_group[pos] = pos;
    b->hash += p4423[pos] * color;
    group_index[pos] = b->group_count;
    b->groups[b->group_count].base = pos;
    b->groups[b->group_count].pseudo_liberties = get_empty_neighbour(b, pos);
    
    get_empty_neighbour(b, pos);

    // put a stone, update neighbour group's pseudo_liberties

    add_stone_update_pseudo_liberties(b, pos);

    // try to capture others
    bool captured_other = false;

    int oppocolor = color == white ? black : white;
    if (b->color[N(b, pos)] == oppocolor) {
        captured_other |= try_delete_group(b, GROUP(b, N(b, pos)));
    }
    if (b->color[S(b, pos)] == oppocolor) {
        captured_other |= try_delete_group(b, GROUP(b, S(b, pos)));
    }
    if (b->color[W(b, pos)] == oppocolor) {
        captured_other |= try_delete_group(b, GROUP(b, W(b, pos)));
    }
    if (b->color[E(b, pos)] == oppocolor) {
        captured_other |= try_delete_group(b, GROUP(b, E(b, pos)));
    }

    // merge with friendly neighboour stones's group

    if (b->color[N(b, pos)] == color) {
        try_merge_group(b, GROUP(b, N(b, pos)), GROUP(b, pos));
    }
    if (b->color[S(b, pos)] == color) {
        try_merge_group(b, GROUP(b, S(b, pos)), GROUP(b, pos));
    }
    if (b->color[W(b, pos)] == color) {
        try_merge_group(b, GROUP(b, W(b, pos)), GROUP(b, pos));
    }
    if (b->color[E(b, pos)] == color) {
        try_merge_group(b, GROUP(b, E(b, pos)), GROUP(b, pos));
    }

    // check suicide

    if (try_delete_group(b, GROUP(b, pos)) && !captured_other)
        return false;

    // check repetition
    
    if (b->prev_hash == b->hash)
        return false;
    b->prev_hash = recorded_hash;

    // clean removed group
    int new_group_cnt = 0;
    for (int i = 0; i <= b->group_count; i++) {
        if (b->groups[i].base >= 0) {
            new_group_cnt ++;
        }
    }
    group *ngroups = new group[new_group_cnt + 1];
    new_group_cnt = 0;
    for (int i = 0; i <= b->group_count; i++) {
        if (b->groups[i].base >= 0) {
            ngroups[new_group_cnt ++] = b->groups[i];
        }
    }
    delete b->groups;
    b->groups = ngroups;
    b->group_count = new_group_cnt;

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
    for (int i = 0; i < b->group_count; i++) {
        if (b->groups[i].pseudo_liberties != recalc_ps_lib[b->groups[i].base])
            return false;
    }
    return true;
}
