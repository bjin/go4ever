
#include "board.hpp"
#include <cstring>

double gamma_data[2][1 << 20];

STATIC void gamma_add(nbr3x3_t bits, double val)
{
    for (int dir = 0; dir < 4; dir++) {
        for (int mir = 0; mir < 2; mir++) {
            gamma_data[0][bits] = val;
            gamma_data[1][reverse_color_3x3(bits)] = val;
            bits = mirror_3x3(bits);
        }
        bits = rotate90_3x3(bits);
    }
}

struct item {
    nbr3x3_t bits;
    double val;
};

const item items[] = {
#include "gamma.data"
(item){0, 0}
};

void gamma_init()
{
    memset(gamma_data, 0, sizeof(gamma_data));
    for (int it = 0; it < 2051; ++it)
        gamma_add(items[it].bits, items[it].val);
}
