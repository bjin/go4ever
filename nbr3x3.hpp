
#ifndef NBR3X3_H
#define NBR3X3_H

#include <cstdio>

typedef unsigned nbr3x3_t;

// 20bits
// 0 - 15, 2 bits for each direction
//
//  8  6  3
// 11  x  1
// 13 16 18
//
// [0, 5, 10, 15]: 1 bits for E N W S, in atari or not

// 9876 5432 1098 7654 3210
//   01    0 1    01    01
#define NBR_BITS   (0x10842U)
// 9876 5432 1098 7654 3210
// 01    01    01    0 1
#define DIAG_BITS  (0x42108U)
// 9876 5432 1098 7654 3210
//      1     1     1     1
#define ATARI_BITS (0x08421U)


STATIC bool is_eyelike_3x3(nbr3x3_t bits, stone_t color)
{
    // check if all ENWS are color or border
    if (~bits >> (color - 1) & NBR_BITS)
        return false;
    // count all other player's stone in DIAG
    int cnt = ((bits >> (2 - color) & DIAG_BITS & ~bits >> (color - 1)) >> 3) % 31;
    // and ONE if a stone is off border
    cnt += (bits & (bits >> 1) & DIAG_BITS) > 0;
    return cnt < 2;
}

STATIC bool is_atari_of_3x3(nbr3x3_t bits, stone_t color)
{
    // find out my stone's in N,S,W,E
    nbr3x3_t nbr_color = bits >> (color - 1) & NBR_BITS & ~bits >> (2 - color);
    // check if any of my stones is in atari
    return bits << 1 & nbr_color;
}

STATIC bool is_suicide_3x3(nbr3x3_t bits, stone_t color)
{
    // check if there are empty cell in N,S,W,E
    if (~bits & NBR_BITS & ~bits >> 1)
        return false;
    // find out my stone's in N,S,W,E
    nbr3x3_t nbr_color = bits >> (color - 1) & NBR_BITS & ~bits >> (2 - color);
    // check if all my stones are in atari
    if (~bits << 1 & nbr_color)
        // one of my stone is not in atari
        return false;
    return true;
}

STATIC nbr3x3_t rotate90_3x3(nbr3x3_t bits)
{
}

STATIC nbr3x3_t mirror_3x3(nbr3x3_t bits)
{
}

STATIC nbr3x3_t reverse_color_3x3(nbr3x3_t bits)
{
}

STATIC stone_t get_eyelike_color_3x3(nbr3x3_t bits)
{
}

STATIC nbr3x3_t construct_3x3(stone_t n, stone_t s, stone_t w, stone_t e,
        stone_t ne, stone_t nw, stone_t se, stone_t sw)
{
    return (nbr3x3_t)e << 1 | (nbr3x3_t)n << 6 | (nbr3x3_t)w << 11 | (nbr3x3_t)s << 16 |
        (nbr3x3_t)ne << 3 | (nbr3x3_t)nw << 8 | (nbr3x3_t)sw << 13 | (nbr3x3_t)se << 18;
}

STATIC void set_atari_bits_3x3(nbr3x3_t &bits, bool n, bool s, bool w, bool e)
{
    nbr3x3_t mask = (nbr3x3_t)e << 0 | (nbr3x3_t)n << 5
        | (nbr3x3_t)w << 10 | (nbr3x3_t)s << 15;
    bits |= mask;
}

STATIC void unset_atari_bits_3x3(nbr3x3_t &bits, bool n, bool s, bool w, bool e)
{
    nbr3x3_t mask = (nbr3x3_t)e << 0 | (nbr3x3_t)n << 5
        | (nbr3x3_t)w << 10 | (nbr3x3_t)s << 15;
    bits &= ~mask;
}

STATIC void clear_atari_bits_3x3(nbr3x3_t &bits)
{
    bits &= ~ATARI_BITS;
}

inline static void dump_nbr3x3(nbr3x3_t bits)
{
#define ST(p) stone2char((stone_t)(bits >> (p) & 3))
#define ED(p) (bits >> (p) & 1 ? '!' : ' ')
    printf("%c  %c%c %c \n", ST(8), ST(6), ED(5), ST(3));
    printf("%c%c *  %c%c\n", ST(11), ED(10), ST(1), ED(0));
    printf("%c  %c%c %c \n", ST(13), ST(16), ED(15), ST(18));
#undef ST
#undef ED
}
#endif
