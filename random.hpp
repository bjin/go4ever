#ifndef RANDOM_H
#define RANDOM_H

void fast_srandom(unsigned long seed);
unsigned long fast_getseed(void);

unsigned short fast_random(unsigned int max);
static unsigned int fast_irandom(unsigned int max);

float fast_frandom();

static inline unsigned int fast_irandom(unsigned int max)
{
    if (max <= 65536)
        return fast_random(max);
    int himax = (max - 1) / 65536;
    unsigned short hi = fast_random(himax + 1);
    return ((unsigned int)hi << 16) | fast_random(hi < himax ? 65536 : max % 65536);
}

static inline double fast_drandom()
{
    const static double inv_max_uint = 1.0 / double (1U << 31);
    return fast_irandom(1U << 31) * inv_max_uint;
}

#endif
