#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "board.hpp"
#include "gtp.hpp"


int main(int argc, char *argv[])
{
    initialize();
    play_gtp();
    return 0;
}
