
#include "board.hpp"
#include "random.hpp"

#include <cstring>
#include <cstdio>

static hash_t p4423[max_len];

STATIC nbr3x3_t recalc_nbr3x3(board_t *b, index_t pos)
{
    return construct_3x3(b->stones[N(b, pos)], b->stones[S(b, pos)],
            b->stones[W(b, pos)], b->stones[E(b, pos)],
            b->stones[NE(b, pos)], b->stones[NW(b, pos)],
            b->stones[SE(b, pos)], b->stones[SW(b, pos)]);
}

void initialize()
{
    p4423[0] = 1;
    for (index_t i = 1; i < max_len; i++) {
        p4423[i] = p4423[i - 1] * 4423;
    }
}

void empty_board(board_t *b, index_t size)
{
    b->size = size;
    b->len = (size + 1) * (size + 2) + 1;
    b->hash = 0;
    b->ko_pos = -1;

    for (index_t i = 0; i < b->len; i++) {
        b->stones[i] = STONE_BORDER;
    }
    for (index_t i = 0; i < size; i++)
        for (index_t j = 0; j < size; j++) {
            index_t pos = POS(b, i, j);
            index_t k = i * size + j;
            b->stones[pos] = STONE_EMPTY;
            b->list_pos[pos] = k;
            b->list[k] = pos;
        }
    for (index_t i = 0; i < size; i++)
        for (index_t j = 0; j < size; j++) {
            index_t pos = POS(b, i, j);
            b->nbr3x3[pos] = recalc_nbr3x3(b, pos);
        }
    b->empty_ptr = b->size * b->size;
    b->group_ptr = b->size * b->size;

    b->vis_cnt = 0;
    memset(b->vis, 0, sizeof(index_t) * b->len);
}

void fork_board(board_t *nb, const board_t *b)
{
    nb->size = b->size;
    nb->len = b->len;
    nb->hash = b->hash;
    nb->ko_pos = b->ko_pos;
    nb->ko_color = b->ko_color;
    memcpy(nb->stones, b->stones, sizeof(stone_t)*b->len);
    memcpy(nb->next_in_group, b->next_in_group, sizeof(index_t)*b->len);
    memcpy(nb->group_info, b->group_info, sizeof(index_t)*b->len);
    memcpy(nb->group_liberties_sum, b->group_liberties_sum, sizeof(index_t)*b->len);
    memcpy(nb->group_liberties_sum_squared, b->group_liberties_sum_squared, sizeof(index_t)*b->len);
    memcpy(nb->list_pos, b->list_pos, sizeof(index_t)*b->len);
    memcpy(nb->list, b->list, sizeof(index_t)*b->len);
    memcpy(nb->nbr3x3, b->nbr3x3, sizeof(index_t)*b->len);
    memcpy(nb->atari_of_group, b->atari_of_group, sizeof(index_t)*b->len);
    nb->empty_ptr = b->empty_ptr;
    nb->group_ptr = b->group_ptr;
    nb->vis_cnt = 0;
    memset(nb->vis, 0, sizeof(index_t) * b->len);
}

// {{{ help function for put_stone()

// a,b are index of list, in [0, size^2]
STATIC void index_swap(board_t *b, index_t p, index_t q)
{
    b->list_pos[b->list[p]] = q;
    b->list_pos[b->list[q]] = p;
    index_t tmp = b->list[p];
    b->list[p] = b->list[q];
    b->list[q] = tmp;
}

STATIC index_t get_base(board_t *b, index_t pos)
{
    if (b->base_of_group[pos] >= max_len)
        return pos;
    index_t r = pos, tmp;
    do {
        r = b->base_of_group[r];
    } while (b->base_of_group[r] < max_len);
    do {
        tmp = b->base_of_group[pos];
        b->base_of_group[pos] = r;
        pos = tmp;
    } while (pos != r);
    return r;
}

STATIC void touch_nbr3x3(board_t *b, index_t pos)
{
    if (b->vis[pos] != b->vis_cnt) {
        b->vis[pos] = b->vis_cnt;
        b->nbr3x3_changed[b->nbr3x3_cnt++] = pos;
    }
}

// group must be base of a group
inline static index_t find_atari(board_t *b, index_t group)
{
    if (b->pseudo_liberties[group] <= max_len)
        return -1;
    if ((b->pseudo_liberties[group] - max_len) *
            b->group_liberties_sum_squared[group] ==
        b->group_liberties_sum[group] * b->group_liberties_sum[group]) {
        return b->group_liberties_sum[group] /
            (b->pseudo_liberties[group] - max_len);
    }
    return -1;
}

inline static void update_empty_neighbour(board_t *b, index_t pos)
{
    index_t z = get_base(b, pos);
    if (b->stones[N(b, pos)] == STONE_EMPTY) {
        b->pseudo_liberties[z] ++;
        b->group_liberties_sum[z] += N(b, pos);
        b->group_liberties_sum_squared[z] += N(b, pos) * N(b, pos);
    }
    if (b->stones[S(b, pos)] == STONE_EMPTY) {
        b->pseudo_liberties[z] ++;
        b->group_liberties_sum[z] += S(b, pos);
        b->group_liberties_sum_squared[z] += S(b, pos) * S(b, pos);
    }
    if (b->stones[W(b, pos)] == STONE_EMPTY) {
        b->pseudo_liberties[z] ++;
        b->group_liberties_sum[z] += W(b, pos);
        b->group_liberties_sum_squared[z] += W(b, pos) * W(b, pos);
    }
    if (b->stones[E(b, pos)] == STONE_EMPTY) {
        b->pseudo_liberties[z] ++;
        b->group_liberties_sum[z] += E(b, pos);
        b->group_liberties_sum_squared[z] += E(b, pos) * E(b, pos);
    }
}

inline static void add_stone_update_liberties(board_t *b, index_t pos)
{
    index_t z, pos2 = pos * pos;
    if (IS_STONE(b->stones[N(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, N(b, pos))] --;
        b->group_liberties_sum[z] -= pos;
        b->group_liberties_sum_squared[z] -= pos2;
    }
    if (IS_STONE(b->stones[S(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, S(b, pos))] --;
        b->group_liberties_sum[z] -= pos;
        b->group_liberties_sum_squared[z] -= pos2;
    }
    if (IS_STONE(b->stones[W(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, W(b, pos))] --;
        b->group_liberties_sum[z] -= pos;
        b->group_liberties_sum_squared[z] -= pos2;
    }
    if (IS_STONE(b->stones[E(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, E(b, pos))] --;
        b->group_liberties_sum[z] -= pos;
        b->group_liberties_sum_squared[z] -= pos2;
    }
}

inline static void delete_stone_update_liberties(board_t *b, index_t pos)
{
    index_t z, pos2 = pos * pos;
    if (IS_STONE(b->stones[N(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, N(b, pos))] ++;
        b->group_liberties_sum[z] += pos;
        b->group_liberties_sum_squared[z] += pos2;
    }
    if (IS_STONE(b->stones[S(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, S(b, pos))] ++;
        b->group_liberties_sum[z] += pos;
        b->group_liberties_sum_squared[z] += pos2;
    }
    if (IS_STONE(b->stones[W(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, W(b, pos))] ++;
        b->group_liberties_sum[z] += pos;
        b->group_liberties_sum_squared[z] += pos2;
    }
    if (IS_STONE(b->stones[E(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, E(b, pos))] ++;
        b->group_liberties_sum[z] += pos;
        b->group_liberties_sum_squared[z] += pos2;
    }
}

inline static void add_stone_update_3x3(board_t *b, index_t pos)
{
    nbr3x3_t bit = (nbr3x3_t)b->stones[pos];
#define LOOP(P, OFFSET) {\
    b->nbr3x3[P] &= ~(3U << OFFSET); \
    b->nbr3x3[P] |= bit << OFFSET; \
    touch_nbr3x3(b, P); \
}
    LOOP(W(b, pos), 0);
    LOOP(SW(b, pos), 2);
    LOOP(S(b, pos), 4);
    LOOP(SE(b, pos), 6);
    LOOP(E(b, pos), 8);
    LOOP(NE(b, pos), 10);
    LOOP(N(b, pos), 12);
    LOOP(NW(b, pos), 14);
#undef LOOP
}

inline static void delete_stone_update_3x3(board_t *b, index_t pos)
{
#define LOOP(P, OFFSET) {\
    b->nbr3x3[P] &= ~(3U << OFFSET); \
    touch_nbr3x3(b, P); \
}
    LOOP(W(b, pos), 0);
    LOOP(SW(b, pos), 2);
    LOOP(S(b, pos), 4);
    LOOP(SE(b, pos), 6);
    LOOP(E(b, pos), 8);
    LOOP(NE(b, pos), 10);
    LOOP(N(b, pos), 12);
    LOOP(NW(b, pos), 14);
#undef LOOP
}

inline static void maybe_in_atari_now(board_t *b, index_t group)
{
    index_t atari_pos = find_atari(b, group);
    if (atari_pos >= 0) {
        b->atari_of_group[group] = atari_pos;
        set_atari_bits_3x3(b->nbr3x3[atari_pos], 
                IN_GROUP(b, N(b, atari_pos), group),
                IN_GROUP(b, S(b, atari_pos), group),
                IN_GROUP(b, W(b, atari_pos), group),
                IN_GROUP(b, E(b, atari_pos), group));
        touch_nbr3x3(b, atari_pos);
    }
}

inline static void maybe_not_in_atari_now(board_t *b, index_t group)
{
    if (find_atari(b, group) < 0) {
        index_t atari_pos = b->atari_of_group[group];
        if (atari_pos >= 0) {
            b->atari_of_group[group] = -1;
            unset_atari_bits_3x3(b->nbr3x3[atari_pos], 
                    IN_GROUP(b, N(b, atari_pos), group),
                    IN_GROUP(b, S(b, atari_pos), group),
                    IN_GROUP(b, W(b, atari_pos), group),
                    IN_GROUP(b, E(b, atari_pos), group));
            touch_nbr3x3(b, atari_pos);
        }
    }
}

STATIC bool is_eyelike(board_t *b, index_t pos, stone_t color)
{
    return b->stones[pos] == STONE_EMPTY && is_eyelike_3x3(b->nbr3x3[pos], color);
}

// group must be base of a group
inline static bool try_delete_group(board_t *b, index_t group)
{
    if (b->pseudo_liberties[group] != max_len) {
        maybe_in_atari_now(b, group);
        return false;
    }
    index_t ptr = group;
    index_swap(b, b->list_pos[ptr], b->group_ptr++);
    do {
        clear_atari_bits_3x3(b->nbr3x3[ptr]);
        index_swap(b, b->list_pos[ptr], b->empty_ptr++);
        b->hash -= (hash_t)b->stones[ptr] * p4423[ptr];
        b->stones[ptr] = STONE_EMPTY;
        delete_stone_update_liberties(b, ptr);
        delete_stone_update_3x3(b, ptr);
        ptr = b->next_in_group[ptr];
    } while (ptr != group);
    do {
        ptr = b->next_in_group[ptr];
        if (IS_STONE(b->stones[N(b, ptr)])) {
            maybe_not_in_atari_now(b, get_base(b, N(b, ptr)));
        }
        if (IS_STONE(b->stones[S(b, ptr)])) {
            maybe_not_in_atari_now(b, get_base(b, S(b, ptr)));
        }
        if (IS_STONE(b->stones[W(b, ptr)])) {
            maybe_not_in_atari_now(b, get_base(b, W(b, ptr)));
        }
        if (IS_STONE(b->stones[E(b, ptr)])) {
            maybe_not_in_atari_now(b, get_base(b, E(b, ptr)));
        }
    } while (ptr != group);
    return true;
}

// p and q must be base of a group
inline static void try_merge_group(board_t *b, index_t p, index_t q)
{
    if (p == q)
        return;
    if (b->stones[p] != b->stones[q])
        return;
    //merge cyclic linked list
    index_t tmp = b->next_in_group[p];
    b->next_in_group[p] = b->next_in_group[q];
    b->next_in_group[q] = tmp;
    //link q into p
    index_swap(b, b->list_pos[q], b->group_ptr++);
    b->pseudo_liberties[p] += b->pseudo_liberties[q] - max_len;
    b->group_liberties_sum[p] += b->group_liberties_sum[q];
    b->group_liberties_sum_squared[p] += b->group_liberties_sum_squared[q];
    b->base_of_group[q] = p;
}

// return a potential ko_pos, need to check after my stone put down
// and in atari of the returned ko_pos
STATIC index_t detect_ko(board_t *b, index_t pos)
{
    switch (b->nbr3x3[pos] >> 16) {
        case 1: // E
            if (b->next_in_group[E(b, pos)] == E(b, pos))
                return E(b, pos);
            break;
        case 2: // N
            if (b->next_in_group[N(b, pos)] == N(b, pos))
                return N(b, pos);
            break;
        case 4: // W
            if (b->next_in_group[W(b, pos)] == W(b, pos))
                return W(b, pos);
            break;
        case 8: // S
            if (b->next_in_group[S(b, pos)] == S(b, pos))
                return S(b, pos);
            break;
    }
    return -1;
}

// }}}

index_t gen_move(board_t *b, stone_t color, bool ko_rule)
{
    if (b->empty_ptr == 0)
        return -1;
    index_t start = fast_random(b->empty_ptr);
    for (index_t ptr = start;;) {
        index_t pos = b->list[ptr];
        if (!is_eyelike(b, pos, color) && is_legal_move(b, pos, color, ko_rule)) {
            return pos;
        }
        if (++ptr == b->empty_ptr)
            ptr = 0;
        if (ptr == start)
            return -1;
    }
}

// moves must contain at least b->len elements
index_t gen_moves(board_t *b, stone_t color, index_t *moves, bool ko_rule)
{
    index_t cnt = 0;
    for (index_t i = 0; i < b->empty_ptr; i++) {
        if (is_legal_move(b, b->list[i], color, ko_rule)) {
            moves[cnt++] = b->list[i];
        }
    }
    return cnt;
}

bool is_legal_move(board_t *b, index_t pos, stone_t color, bool ko_rule)
{
    if (pos < 0 || pos >= b->len || b->stones[pos] != STONE_EMPTY) {
        return false;
    }

    if (ko_rule && pos == b->ko_pos && color == b->ko_color) {
        return false;
    }

    stone_t oppocolor = color == STONE_WHITE ? STONE_BLACK : STONE_WHITE;

    if (!is_atari_of_3x3(b->nbr3x3[pos], oppocolor) && is_suicide_3x3(b->nbr3x3[pos], color)) {
        return false;
    }

    return true;
}

void put_stone(board_t *b, index_t pos, stone_t color)
{
    // basic checks

    if (pos < 0 || pos >= b->len || b->stones[pos] != STONE_EMPTY)
        return;

    stone_t oppocolor = color == STONE_WHITE ? STONE_BLACK : STONE_WHITE;

    // records some variable

    b->vis_cnt ++;
    b->nbr3x3_cnt = 0;

    index_t r_ko_pos = b->ko_pos;
    stone_t r_ko_color = b->ko_color;

    b->ko_pos = detect_ko(b, pos);
    if (b->ko_pos >= 0 && b->stones[b->ko_pos] == color)
        b->ko_pos = -1;

    // put a stone 

    b->stones[pos] = color;
    b->next_in_group[pos] = pos;
    b->group_info[pos] = max_len;
    b->group_liberties_sum[pos] = 0;
    b->group_liberties_sum_squared[pos] = 0;
    b->atari_of_group[pos] = -1;
    index_swap(b, b->list_pos[pos], --b->empty_ptr);
    index_swap(b, b->list_pos[pos], --b->group_ptr);
    b->hash += p4423[pos] * (hash_t)color;

    update_empty_neighbour(b, pos);

    // update neighbour group's pseudo_liberties

    add_stone_update_liberties(b, pos);

    // try to capture others

    if (b->stones[N(b, pos)] == oppocolor) {
        try_delete_group(b, get_base(b, N(b, pos)));
    }
    if (b->stones[S(b, pos)] == oppocolor) {
        try_delete_group(b, get_base(b, S(b, pos)));
    }
    if (b->stones[W(b, pos)] == oppocolor) {
        try_delete_group(b, get_base(b, W(b, pos)));
    }
    if (b->stones[E(b, pos)] == oppocolor) {
        try_delete_group(b, get_base(b, E(b, pos)));
    }

    add_stone_update_3x3(b, pos);
    clear_atari_bits_3x3(b->nbr3x3[pos]);

    // merge with friendly neighbours' group

    if (b->stones[N(b, pos)] == color) {
        try_merge_group(b, get_base(b, N(b, pos)), get_base(b, pos));
    }
    if (b->stones[S(b, pos)] == color) {
        try_merge_group(b, get_base(b, S(b, pos)), get_base(b, pos));
    }
    if (b->stones[W(b, pos)] == color) {
        try_merge_group(b, get_base(b, W(b, pos)), get_base(b, pos));
    }
    if (b->stones[E(b, pos)] == color) {
        try_merge_group(b, get_base(b, E(b, pos)), get_base(b, pos));
    }

    // check suicide
    
    if (!try_delete_group(b, get_base(b, pos))) {
        if (b->ko_pos >= 0 && (
                    b->next_in_group[pos] != pos ||
                    find_atari(b, pos) < 0 ||
                    b->atari_of_group[pos] != b->ko_pos)) {
            b->ko_pos = -1;
        }
    }
    if (b->ko_pos >= 0) {
        b->ko_color = oppocolor;
    }
}

bool check_board(board_t *b)
{
    static bool flag[max_len];
    memset(flag, false, sizeof(bool) * max_len);
    for (index_t i = 0; i < b->len; i++) {
        if (IS_STONE(b->stones[i])) {
            flag[b->next_in_group[i]] = true;
            //check next[i] in the same group
            if (get_base(b, i) != get_base(b, b->next_in_group[i]))
                return false;
            //check same color
            if (b->stones[i] != b->stones[b->next_in_group[i]])
                return false;
        }
    }
    // check uniqueness of next[i]
    for (index_t i = 0; i < b->len; i++) {
        if (IS_STONE(b->stones[i]) != flag[i]) {
            return false;
        }
    }
    //check pseudo_liberties information
    board_t *b2 = new board_t;
    fork_board(b2, b);
    for (index_t i = 0; i < b2->len; i++) {
        if (IS_STONE(b2->stones[i])) {
            index_t j = get_base(b2, i);
            b2->group_info[j] = max_len;
            b2->group_liberties_sum[j] = 0;
            b2->group_liberties_sum_squared[j] = 0;
        }
    }
    for (index_t i = 0; i < b2->len; i++) {
        if (IS_STONE(b2->stones[i])) {
            update_empty_neighbour(b2, i);
        }
    }
    for (index_t i = 0; i < b2->len; i++) {
        if (IS_STONE(b2->stones[i])) {
            index_t p = get_base(b, i);
            index_t q = get_base(b2, i);
            if (b->group_info[p] != b2->group_info[q]) {
                delete b2;
                return false;
            }
            if (b->group_liberties_sum[p] != b2->group_liberties_sum[q] ||
                    b->group_liberties_sum_squared[p] != 
                    b2->group_liberties_sum_squared[q]) {
                delete b2;
                return false;
            }
        }
    }
    delete b2;
    // check hash value
    hash_t nhash = 0;
    for (index_t i = 0; i < b->len; i++) {
        if (IS_STONE(b->stones[i]))
            nhash += (hash_t)b->stones[i] * p4423[i];
    }
    if (nhash != b->hash)
        return false;
    // check list
    for (index_t i = 0; i < b->len; i++) {
        if (b->stones[i] != STONE_BORDER) {
            index_t j = b->list_pos[i];
            if (j < 0 || j >= b->size * b->size)
                return false;
            if (b->list[j] != i)
                return false;
            int tA = b->stones[i] == STONE_EMPTY ? 0 : 
                i == get_base(b, i) ? 2 : 1;
            int tB = j < b->empty_ptr ? 0 : j < b->group_ptr ? 1 : 2;
            if (tA != tB)
                return false;
        }
    }
    // check nbr3x3
    nbr3x3_t new_nbr[max_len];
    for (index_t i = 0; i < b->len; i++) {
        if (b->stones[i] != STONE_BORDER) {
            new_nbr[i] = recalc_nbr3x3(b, i);
        }
    }
    for (index_t i = 0; i < b->len; i++) {
        if (IS_STONE(b->stones[i]) && get_base(b, i) == i && find_atari(b, i) >= 0) {
            index_t av = find_atari(b, i);
            if (av < 0)
                return false;
            set_atari_bits_3x3(new_nbr[av],
                    IN_GROUP(b, N(b, av), i),
                    IN_GROUP(b, S(b, av), i),
                    IN_GROUP(b, W(b, av), i),
                    IN_GROUP(b, E(b, av), i));
        }
    }
    for (index_t i = 0; i < b->len; i++) {
        if (b->stones[i] != STONE_BORDER) {
            if (new_nbr[i] != b->nbr3x3[i])
                return false;
        }
    }
    return true;
}

void calc_final_score(board_t *b, int &bs, int &ws)
{
    bs = 0;
    ws = 0;
    b->vis_cnt ++;
    for (index_t i = 0; i < b->len; i++) {
        if (b->stones[i] == STONE_BORDER)
            continue;
        if (b->vis[i] == b->vis_cnt)
            continue;
        if (b->stones[i] == STONE_BLACK) {
            bs++;
            continue;
        } else if (b->stones[i] == STONE_WHITE) {
            ws++;
            continue;
        }
        // use BFS to check the color of reachable 
        int head = 0;
        int tail = 0;
        b->queue[tail++] = i;
        b->vis[i] = b->vis_cnt;
        bool reachB = false;
        bool reachW = false;
        while (head < tail) {
            index_t p = b->queue[head++];
#define EXPAND(Q) \
            if (b->stones[Q] == STONE_EMPTY) { \
                if (b->vis[Q] != b->vis_cnt) { \
                    b->vis[Q] = b->vis_cnt; \
                    b->queue[tail++] = Q; \
                } \
            } else if (b->stones[Q] == STONE_BLACK) { \
                reachB = true; \
                if (reachW) \
                break; \
            } else if (b->stones[Q] == STONE_WHITE) { \
                reachW = true; \
                if (reachB) \
                break; \
            }
            EXPAND(N(b, p));
            EXPAND(S(b, p));
            EXPAND(W(b, p));
            EXPAND(E(b, p));
#undef EXPAND
        }
        if (reachB && !reachW) {
            bs += tail;
        } else if (!reachB && reachW) {
            ws += tail;
        }
    }
}

void dump_board(const board_t *b)
{
    for (index_t i = 0; i < b->size; i++) {
        for (index_t j = 0; j < b->size; j++) {
            stone_t color = b->stones[POS(b, i, j)];
            putchar(stone2char(color));
        }
        putchar('\n');
    }
}
