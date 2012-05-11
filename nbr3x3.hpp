
#ifndef NBR3X3_H
#define NBR3X3_H

typedef unsigned nbr3x3_t;

// 20bits
// 0 - 15, 2 bits for each direction
//
//  6  4  2
//  8  x  0
// 10 12 14
//
// 16 - 19, 1 bits for E N W S, in atari or not

#define NBR_BITS   (0x01111U)
#define DIAG_BITS  (0x04444U)
#define UPPER_BITS (0xF0000U)
#define LOWER_BITS (0x0FFFFU)
#define E_BITS3 (0x0003U)
#define N_BITS3 (0x0030U)
#define W_BITS3 (0x0300U)
#define S_BITS3 (0x3000U)

STATIC bool is_eyelike_3x3(nbr3x3_t bits, stone_t color)
{
    // check if all ENWS are color or border
    if (~bits >> (color - 1) & NBR_BITS)
        return false;
    // count all other player's stone in DIAG
    int cnt = __builtin_popcount(bits >> (2 - color) & DIAG_BITS & ~bits >> (color - 1));
    // and ONE if a stone is off border
    if (bits & (bits >> 1) & DIAG_BITS)
        cnt++;
    return cnt < 2;
}

STATIC nbr3x3_t rotate90_3x3(nbr3x3_t bits)
{
    nbr3x3_t upper = bits >> 16;
    nbr3x3_t lower = bits & LOWER_BITS;
    upper = upper >> 1 | (upper & 1) << 3;
    lower = lower >> 4 | (lower & 15) << 12;
    return lower | upper << 16;
}

STATIC nbr3x3_t mirror_3x3(nbr3x3_t bits)
{
    nbr3x3_t upper = bits >> 16;
    nbr3x3_t lower = bits & LOWER_BITS;
    upper = (upper & 3) << 2 | (upper & 12) >> 2;
    upper = (upper & 5) << 1 | (upper & 10) >> 1;
    lower = (lower & 0x00FF) << 8 | (lower & 0xFF00) >> 8;
    lower = (lower & 0x0F0F) << 4 | (lower & 0xF0F0) >> 4;
    return lower | upper << 16;
}

STATIC nbr3x3_t reverse_color_3x3(nbr3x3_t bits)
{
    nbr3x3_t upper = bits >> 16;
    nbr3x3_t lower = bits & LOWER_BITS;
    lower = (lower & 0x5555) << 1 | (lower & 0xAAAA) >> 1;
    return lower | upper << 16;
}

STATIC stone_t get_eyelike_color(nbr3x3_t bits)
{
    bits = ((bits & 0x3300) >> 8) & (bits & 0x0033);
    bits = ((bits & 0x30) >> 4) & (bits & 0x03);
    return (stone_t)bits;
}

STATIC nbr3x3_t construct_3x3(stone_t n, stone_t s, stone_t w, stone_t e,
        stone_t ne, stone_t nw, stone_t se, stone_t sw)
{
    return (nbr3x3_t)e << 0 | (nbr3x3_t)n << 4 | (nbr3x3_t)w << 8 | (nbr3x3_t)s << 12 |
        (nbr3x3_t)ne << 2 | (nbr3x3_t)nw << 6 | (nbr3x3_t)sw << 10 | (nbr3x3_t)se << 14;
}

STATIC void set_atari_bits_3x3(nbr3x3_t &bits, bool n, bool s, bool w, bool e)
{
    nbr3x3_t mask = (nbr3x3_t)e << 16 | (nbr3x3_t)n << 17
        | (nbr3x3_t)w << 18 | (nbr3x3_t)s << 19;
    bits |= mask;
}

STATIC void unset_atari_bits_3x3(nbr3x3_t &bits, bool n, bool s, bool w, bool e)
{
    nbr3x3_t mask = (nbr3x3_t)e << 16 | (nbr3x3_t)n << 17
        | (nbr3x3_t)w << 18 | (nbr3x3_t)s << 19;
    bits &= ~mask;
}

STATIC void clear_atari_bits_3x3(nbr3x3_t &bits)
{
    bits &= LOWER_BITS;
}

inline static void dump_nbr3x3(nbr3x3_t bits)
{
#define ST(p) stone2char((stone_t)(bits >> (p) & 3))
#define ED(p) (bits >> (p) & 1 ? '!' : ' ')
    printf("%c  %c%c %c \n", ST(6), ST(4), ED(17), ST(2));
    printf("%c%c *  %c%c\n", ST(8), ED(18), ST(0), ED(16));
    printf("%c  %c%c %c \n", ST(10), ST(12), ED(19), ST(14));
#undef ST
#undef ED
}
#endif
