
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
    b->prev_hash = -1;
    b->prev_pos = 0;
    b->prev_color = STONE_EMPTY;

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
    nb->prev_hash = b->prev_hash;
    nb->prev_pos = b->prev_pos;
    nb->prev_color = b->prev_color;
    memcpy(nb->stones, b->stones, sizeof(stone_t)*b->len);
    memcpy(nb->next_in_group, b->next_in_group, sizeof(index_t)*b->len);
    memcpy(nb->group_info, b->group_info, sizeof(index_t)*b->len);
    memcpy(nb->group_liberties_xor, b->group_liberties_xor, sizeof(index_t)*b->len);
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

STATIC void change_nbr3x3(board_t *b, index_t pos)
{
    if (b->vis[pos] != b->vis_cnt) {
        b->vis[pos] = b->vis_cnt;
        b->nbr3x3_changed[b->nbr3x3_cnt++] = pos;
    }
}

// can only handle atari with 2 pseudo_liberties now
// but I think it's enough
// group must be base of a group
STATIC index_t find_atari(board_t *b, index_t group)
{
    if (b->pseudo_liberties[group] == max_len + 1) {
        return b->group_liberties_xor[group];
    } else if (b->pseudo_liberties[group] == max_len + 2) {
        if (b->group_liberties_xor[group] == 0) {
            // the only two pseudo_liberties is the same, it's atari
            // enumerate all stones in this group
            for (index_t ptr = group;;) {
                if (b->stones[N(b, ptr)] == STONE_EMPTY)
                    return N(b, ptr);
                if (b->stones[S(b, ptr)] == STONE_EMPTY)
                    return S(b, ptr);
                if (b->stones[W(b, ptr)] == STONE_EMPTY)
                    return W(b, ptr);
                if (b->stones[E(b, ptr)] == STONE_EMPTY)
                    return E(b, ptr);
                ptr = b->next_in_group[ptr];
                if (ptr == group)
                    break;
            }
        }
    }
    return -1;
}

inline static void update_empty_neighbour(board_t *b, index_t pos)
{
    index_t z = get_base(b, pos);
    if (b->stones[N(b, pos)] == STONE_EMPTY) {
        b->pseudo_liberties[z]++;
        b->group_liberties_xor[z] ^= N(b, pos);
    }
    if (b->stones[S(b, pos)] == STONE_EMPTY) {
        b->pseudo_liberties[z]++;
        b->group_liberties_xor[z] ^= S(b, pos);
    }
    if (b->stones[W(b, pos)] == STONE_EMPTY) {
        b->pseudo_liberties[z]++;
        b->group_liberties_xor[z] ^= W(b, pos);
    }
    if (b->stones[E(b, pos)] == STONE_EMPTY) {
        b->pseudo_liberties[z]++;
        b->group_liberties_xor[z] ^= E(b, pos);
    }
}

inline static void add_stone_update_liberties(board_t *b, index_t pos)
{
    index_t z;
    if (IS_STONE(b->stones[N(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, N(b, pos))] --;
        b->group_liberties_xor[z] ^= pos;
    }
    if (IS_STONE(b->stones[S(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, S(b, pos))] --;
        b->group_liberties_xor[z] ^= pos;
    }
    if (IS_STONE(b->stones[W(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, W(b, pos))] --;
        b->group_liberties_xor[z] ^= pos;
    }
    if (IS_STONE(b->stones[E(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, E(b, pos))] --;
        b->group_liberties_xor[z] ^= pos;
    }
}

inline static void delete_stone_update_liberties(board_t *b, index_t pos)
{
    index_t z;
    if (IS_STONE(b->stones[N(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, N(b, pos))] ++;
        b->group_liberties_xor[z] ^= pos;
    }
    if (IS_STONE(b->stones[S(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, S(b, pos))] ++;
        b->group_liberties_xor[z] ^= pos;
    }
    if (IS_STONE(b->stones[W(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, W(b, pos))] ++;
        b->group_liberties_xor[z] ^= pos;
    }
    if (IS_STONE(b->stones[E(b, pos)])) {
        b->pseudo_liberties[z=get_base(b, E(b, pos))] ++;
        b->group_liberties_xor[z] ^= pos;
    }
}

inline static void add_stone_update_3x3(board_t *b, index_t pos)
{
    nbr3x3_t mask = 3;
    nbr3x3_t bit = (nbr3x3_t)b->stones[pos];
#define LOOP(P) {\
    b->nbr3x3[P] &= ~mask; \
    b->nbr3x3[P] |= bit; \
    change_nbr3x3(b, P); \
    mask <<= 2; \
    bit <<= 2; \
}
    LOOP(W(b, pos));
    LOOP(SW(b, pos));
    LOOP(S(b, pos));
    LOOP(SE(b, pos));
    LOOP(E(b, pos));
    LOOP(NE(b, pos));
    LOOP(N(b, pos));
    LOOP(NW(b, pos));
#undef LOOP
}

inline static void delete_stone_update_3x3(board_t *b, index_t pos)
{
    nbr3x3_t mask = 3;
#define LOOP(P) {\
    b->nbr3x3[P] &= ~mask; \
    change_nbr3x3(b, P); \
    mask <<= 2; \
}
    LOOP(W(b, pos));
    LOOP(SW(b, pos));
    LOOP(S(b, pos));
    LOOP(SE(b, pos));
    LOOP(E(b, pos));
    LOOP(NE(b, pos));
    LOOP(N(b, pos));
    LOOP(NW(b, pos));
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
        change_nbr3x3(b, atari_pos);
    }
}

inline static void maybe_not_in_atari_now(board_t *b, index_t group)
{
    if (!IS_IN_ATARI(b, group)) {
        index_t atari_pos = b->atari_of_group[group];
        if (atari_pos >= 0) {
            b->atari_of_group[group] = -1;
            unset_atari_bits_3x3(b->nbr3x3[atari_pos], 
                    IN_GROUP(b, N(b, atari_pos), group),
                    IN_GROUP(b, S(b, atari_pos), group),
                    IN_GROUP(b, W(b, atari_pos), group),
                    IN_GROUP(b, E(b, atari_pos), group));
            change_nbr3x3(b, atari_pos);
        }
    }
}

STATIC bool is_eyelike(board_t *b, index_t pos, stone_t color)
{
    return b->stones[pos] == STONE_EMPTY && is_eyelike_3x3(b->nbr3x3[pos], color);
}

// group must be base of a group
inline static void try_delete_group(board_t *b, index_t group)
{
    if (b->pseudo_liberties[group] != max_len) {
        maybe_in_atari_now(b, group);
        return;
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
    b->group_liberties_xor[p] ^= b->group_liberties_xor[q];
    b->base_of_group[q] = p;
}

// }}}

index_t gen_move(board_t *b, stone_t color)
{
    if (b->empty_ptr == 0)
        return -1;
    return b->list[fast_random(b->empty_ptr)];
}

// moves must contain at least b->len elements
index_t gen_moves(board_t *b, stone_t color, index_t *moves, bool ko_rule)
{
    index_t cnt = 0;
    for (index_t i = 0; i < b->empty_ptr; i++) {
        if (put_stone(b, b->list[i], color, ko_rule, true, false)) {
            moves[cnt++] = b->list[i];
        }
    }
    return cnt;
}

bool put_stone(board_t *b, index_t pos, stone_t color, 
        bool ko_rule, bool check_legal, bool update_board)
{
    // basic checks

    if (pos < 0 || pos >= b->len) {
        return false;
    }
    if (b->stones[pos] != STONE_EMPTY) {
        return false;
    }

    hash_t recorded_hash = b->hash;
    b->vis_cnt ++;
    b->nbr3x3_cnt = 0;

    // put a stone 

    index_swap(b, b->list_pos[pos], --b->empty_ptr);
    index_swap(b, b->list_pos[pos], --b->group_ptr);
    b->stones[pos] = color;
    b->next_in_group[pos] = pos;
    b->group_info[pos] = max_len;
    b->group_liberties_xor[pos] = 0;
    b->hash += p4423[pos] * (hash_t)color;
    b->atari_of_group[pos] = -1;

    update_empty_neighbour(b, pos);

    // update neighbour group's pseudo_liberties

    add_stone_update_liberties(b, pos);

    // try to capture others

    bool captured_other = false;
    stone_t oppocolor = color == STONE_WHITE ? STONE_BLACK : STONE_WHITE;

    if (b->stones[N(b, pos)] == oppocolor) {
        captured_other = b->pseudo_liberties[get_base(b, N(b, pos))] == max_len;
    }
    if (!captured_other && b->stones[S(b, pos)] == oppocolor) {
        captured_other |= b->pseudo_liberties[get_base(b, S(b, pos))] == max_len;
    }
    if (!captured_other && b->stones[W(b, pos)] == oppocolor) {
        captured_other |= b->pseudo_liberties[get_base(b, W(b, pos))] == max_len;
    }
    if (!captured_other && b->stones[E(b, pos)] == oppocolor) {
        captured_other |= b->pseudo_liberties[get_base(b, E(b, pos))] == max_len;
    }
    if (!captured_other && check_legal) {
        // check if it's a suicide or not. if so, revert back and return false
        index_t current_pseudo_liberties = b->pseudo_liberties[pos] - max_len;
        index_t baseN = -1, baseS = -1, baseW = -1, baseE = -1;
        if (b->stones[N(b, pos)] == color) {
            baseN = get_base(b, N(b, pos));
            current_pseudo_liberties += b->pseudo_liberties[baseN] - max_len;
        }
        if (b->stones[S(b, pos)] == color) {
            baseS = get_base(b, S(b, pos));
            if (baseS != baseN)
                current_pseudo_liberties += b->pseudo_liberties[baseS] - max_len;
        }
        if (b->stones[W(b, pos)] == color) {
            baseW = get_base(b, W(b, pos));
            if (baseW != baseN && baseW != baseS)
                current_pseudo_liberties += b->pseudo_liberties[baseW] - max_len;
        }
        if (b->stones[E(b, pos)] == color) {
            baseE = get_base(b, E(b, pos));
            if (baseE != baseN && baseE != baseS && baseE != baseW)
                current_pseudo_liberties += b->pseudo_liberties[baseE] - max_len;
        }
        if (current_pseudo_liberties == 0) {
            // surely it's a suicide, revert back
            delete_stone_update_liberties(b, pos);
            b->stones[pos] = STONE_EMPTY;
            b->hash -= p4423[pos] * color;
            index_swap(b, b->list_pos[pos], b->group_ptr++);
            index_swap(b, b->list_pos[pos], b->empty_ptr++);
            return false;
        }
    } 

    if (!update_board) {
        // revert back, ignore ko rule
        // TODO : add ko rule detection for read-only mode
        delete_stone_update_liberties(b, pos);
        b->stones[pos] = STONE_EMPTY;
        b->hash -= p4423[pos] * color;
        index_swap(b, b->list_pos[pos], b->group_ptr++);
        index_swap(b, b->list_pos[pos], b->empty_ptr++);
        return true;
    }

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

    try_delete_group(b, get_base(b, pos));

    maybe_in_atari_now(b, get_base(b, pos));

    // check ko

    if (check_legal && ko_rule && b->prev_hash == b->hash) {
        // by ko rule, revert back by redo last move

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
            b2->group_liberties_xor[j] = 0;
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
            if (b->group_liberties_xor[p] != b2->group_liberties_xor[q]) {
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
        if (IS_STONE(b->stones[i]) && get_base(b, i) == i && IS_IN_ATARI(b, i)) {
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
