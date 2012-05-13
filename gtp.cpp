/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This is GNU GO, a Go program. Contact gnugo@gnu.org, or see   *
 * http://www.gnu.org/software/gnugo/ for more information.      *
 *                                                               *
 * To facilitate development of the Go Text Protocol, the two    *
 * files gtp.c and gtp.h are licensed under less restrictive     *
 * terms than the rest of GNU Go.                                *
 *                                                               *
 * Copyright 2001 by the Free Software Foundation.               *
 *                                                               *
 * Permission is hereby granted, free of charge, to any person   *
 * obtaining a copy of this file gtp.c, to deal in the Software  *
 * without restriction, including without limitation the rights  *
 * to use, copy, modify, merge, publish, distribute, and/or      *
 * sell copies of the Software, and to permit persons to whom    *
 * the Software is furnished to do so, provided that the above   *
 * copyright notice(s) and this permission notice appear in all  *
 * copies of the Software and that both the above copyright      *
 * notice(s) and this permission notice appear in supporting     *
 * documentation.                                                *
 *                                                               *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY     *
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE    *
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR       *
 * PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO      *
 * EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS  *
 * NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR    *
 * CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING    *
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF    *
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT    *
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS       *
 * SOFTWARE.                                                     *
 *                                                               *
 * Except as contained in this notice, the name of a copyright   *
 * holder shall not be used in advertising or otherwise to       *
 * promote the sale, use or other dealings in this Software      *
 * without prior written authorization of the copyright holder.  *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "board.hpp"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <fstream>

#include "random.hpp"
#include "uct.hpp"
#include "gtp.hpp"

/* These are copied from gnugo.h. We don't include this file in order
 * to remain as independent as possible of GNU Go internals.
 */
#define EMPTY        0
#define WHITE        1
#define BLACK        2

#define VERSION "0.0.1"
/* We need to keep track of the board size in order to be able to
 * convert between coordinate descriptions. We could also have passed
 * the board size in all calls needing it, but that would be
 * unnecessarily inconvenient.
 */
static int gtp_boardsize = -1;

/* List of known commands. */
static struct gtp_command commands[] = {
    {"black",                   gtp_playblack},
    {"boardsize",               gtp_set_boardsize},
    //{"estimate_score",          gtp_estimate_score},
    {"final_score",             gtp_final_score},

    {"genmove_black",           gtp_genmove_black},
    {"genmove_white",           gtp_genmove_white},
    {"komi",                    gtp_set_komi},
    {"level",                   gtp_set_level},
    {"help",                    gtp_help},
    {"level",                   gtp_set_level},
    {"name",                    gtp_name},
    {"protocol_version",        gtp_protocol_version},
    {"quit",                    gtp_quit},


    {"showboard",               gtp_showboard},
    {"undo",                    gtp_undo},
    {"version",                 gtp_version},
    {"white",                   gtp_playwhite},
    {NULL,                      NULL}
};

static board_t* b;

static int prev_move = -1;

int BoardSize = 13;

using namespace std;

ofstream outfile("/home/acm/go4ever/output.txt");

void init_go4ever()
{

    outfile << "aaaaa\n";
    b = new board_t;
    empty_board(b, BoardSize);
}

void play_gtp()
{

    init_go4ever();
    outfile << "aaaaa\n";
    gtp_internal_set_boardsize(13);
    gtp_main_loop(commands);
}

static float komi;

static int
gtp_set_komi(char *s, int id)
{
    if (sscanf(s, "%f", &komi) < 1)
        return gtp_failure(id, "komi not a float");

    return gtp_success(id, "");
}

static int
gtp_final_score(char *s, int id)
{
    float final_score;
    gtp_printid(id, GTP_SUCCESS);
    int bs, ws;
    calc_final_score(b, bs, ws);
    final_score = bs - ws - komi; //black first hand
    if (final_score > 0.1)
        gtp_printf("B+%3.1f", final_score);
    else if (final_score < -0.1)
        gtp_printf("W+%3.1f", -final_score);
    else
        gtp_printf("0");
    gtp_printf(" ");
    return gtp_finish_response();
}

static int
gtp_set_level(char *s, int id)
{
    static int level;
    int new_level;
    if (sscanf(s, "%d", &new_level) < 1)
        return gtp_failure(id, "level not an integer");

    level = new_level;
    int TimeLimit = 0;
    switch(level)
    {
        case 1: TimeLimit = 1000; break;
        case 2: TimeLimit = 2500; break;
        case 3: TimeLimit = 5000; break;
        case 4: TimeLimit = 10000; break;
        case 5: TimeLimit = 15000; break;
        case 6: TimeLimit = 22500; break;
        case 7: TimeLimit = 30000; break;
        case 8: TimeLimit = 40000; break;
        case 9: TimeLimit = 60000; break;
        case 10: TimeLimit = 90000; break;
        default: TimeLimit = 120000; break;
    }
    return gtp_success(id, "");
}

static int
gtp_version(char *s, int id)
{
    return gtp_success(id, VERSION);
}

static int
gtp_undo(char *s, int id)
{
    //go4ever_unmake_move();
    return gtp_success(id, "");
}

static int
gtp_protocol_version(char *s, int id)
{
    return gtp_success(id, "1");
}

static int
gtp_name(char *s, int id)
{
    return gtp_success(id, "go4ever");
}

static int
gtp_playblack(char *s, int id)
{
    int i, j;
    char *c;

    for (c = s; *c; c++)
        *c = tolower((int)*c);

    if (strncmp(s, "pass", 4) == 0) {
        i = -1;
        j = -1;
    }
    else if (!gtp_decode_coord(s, &i, &j))
        return gtp_failure(id, "invalid coordinate");

    if(i != -1 && j != -1)
    {
        prev_move = POS(b,i,j);
        put_stone(b,POS(b,i,j),STONE_BLACK);
    }
    else {
        prev_move = -1;
    }
    return gtp_success(id, "");
}

static int
gtp_playwhite(char *s, int id)
{
    int i, j;
    char *c;

    for (c = s; *c; c++)
        *c = tolower((int)*c);

    if (strncmp(s, "pass", 4) == 0) {
        i = -1;
        j = -1;
    }
    else if (!gtp_decode_coord(s, &i, &j))
        return gtp_failure(id, "invalid coordinate");

    if(i != -1 && j != -1)
    {
        prev_move = POS(b,i,j);
        put_stone(b,POS(b,i,j),STONE_WHITE);
    }
    else {
        prev_move = -1;
    }

    return gtp_success(id, "");
}


static int genmove(int *i, int *j, int side)
{
    int move;
    int val;

    stone_t true_side;
    switch(side) {
        case 0: true_side = STONE_EMPTY; break;
        case 1: true_side = STONE_WHITE; break;
        case 2: true_side = STONE_BLACK; break;
    }
    /*Where should I ask for the next posisiton to put the stone.*/
    //gtp_printf("before_init_node\n");
    //gtp_printf("before_next_move\n");

    move = next_move(b, true_side, prev_move);

    if(move == -1)
    {
        *i = -1;
        *j = -1;
    }
    else
    {
        *i = GETX(b, move);
        *j = GETY(b, move);
    }
    return 0;
}

static void play_move(int i, int j, int side)
{
    stone_t true_side;
    switch(side) {
        case 0: true_side = STONE_EMPTY; break;
        case 1: true_side = STONE_WHITE; break;
        case 2: true_side = STONE_BLACK; break;
    }
    if(i != -1)
    {
        prev_move = POS(b,i,j);
        put_stone(b,POS(b,i,j),true_side);
    } else {
        prev_move = -1;
    }
}

static int gtp_genmove_black(char *s, int id)
{
    int i, j;

    if (genmove(&i, &j, BLACK) >= 0)
        play_move(i, j, BLACK);

    gtp_printid(id, GTP_SUCCESS);
    gtp_print_vertex(i, j);
    return gtp_finish_response();
}

static int gtp_genmove_white(char *s, int id)
{
    //gtp_printf("gtp_genmove_white\n");
    int i, j;
    if (genmove(&i, &j, WHITE) >= 0)
        play_move(i, j, WHITE);

    gtp_printid(id, GTP_SUCCESS);
    gtp_print_vertex(i, j);
    return gtp_finish_response();
}

static int gtp_help(char *s, int id)
{
    int k;

    gtp_printid(id, GTP_SUCCESS);

    for (k = 0; commands[k].name != NULL; k++)
        gtp_printf("%s\n", commands[k].name);

    gtp_printf("\n");
    return GTP_OK;
}

static int
gtp_quit(char *s, int id)
{
    gtp_success(id, "");
    return GTP_QUIT;
}

static int
gtp_showboard(char *s, int id)
{
    dump_board(b);
    return gtp_success(id, "");
}

static int
gtp_set_boardsize(char *s, int id)
{
    int boardsize;
    if (sscanf(s, "%d", &boardsize) < 1)
        return gtp_failure(id, "boardsize not an integer");

    if (boardsize < 5 || boardsize > 19)
        return gtp_failure(id, "unacceptable boardsize");

    BoardSize = boardsize;
    init_go4ever();
    gtp_internal_set_boardsize(boardsize);
    return gtp_success(id, "");
}

/* Read stdin linewise and interpret as GTP commands. */
void
gtp_main_loop(struct gtp_command commands[])
{
    char line[GTP_BUFSIZE];
    char command[GTP_BUFSIZE];
    char *p;
    int i;
    int id;
    int n;
    int status = GTP_OK;
    //FILE *out;
    //out = fopen("out.txt", "w");
    outfile << "aaaaa\n";
    while (status == GTP_OK) {
        /* Read a line from stdin. */
        if (!fgets(line, GTP_BUFSIZE, stdin))
            break; /* EOF or some error */

        //fprintf(out, "%s\n", line);
        //fflush(out);

        /* Remove comments. */
        if ((p = strchr(line, '#')) != NULL)
            *p = 0;

        p = line;

        /* Look for an identification number. */
        if (sscanf(p, "%d%n", &id, &n) == 1)
            p += n;
        else
            id = -1; /* No identification number. */

        /* Look for command name. */
        if (sscanf(p, " %s %n", command, &n) < 1)
            continue; /* Whitespace only on this line, ignore. */
        p += n;

        /* Search the list of commands and call the corresponding function
         * if it's found.
         */

        for (i = 0; commands[i].name != NULL; i++) {
            if (strcmp(command, commands[i].name) == 0) {
                status = (*commands[i].function)(p, id);
                break;
            }
        }
        //fprintf(out, "%d %s\n", i, commands[i].name);
        //fflush(out);

        if (commands[i].name == NULL)
            gtp_failure(id, "unknown command: '%s'", command);

        if (status == GTP_FATAL)
            gtp_panic();
    }
    //fclose(out);
}


/* Set the board size used in coordinate conversions. */
void
gtp_internal_set_boardsize(int size)
{
    gtp_boardsize = size;
    BoardSize = gtp_boardsize;
}

/*
 * This function works like printf, except that it only understands
 * very few of the standard formats, to be precise %c, %d, %f, %s.
 * But it also accepts %m, which takes two integers and writes a move,
 * and %C, which takes a color value and writes a color string.
 */
void
gtp_mprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    for ( ; *fmt ; ++fmt ) {
        if (*fmt == '%') {
            switch (*++fmt) {
                case 'c':
                    {
                        /* rules of promotion => passed as int, not char */
                        int c = va_arg(ap, int);
                        putc(c, stdout);
                        break;
                    }
                case 'd':
                    {
                        int d = va_arg(ap, int);
                        fprintf(stdout, "%d", d);
                        break;
                    }
                case 'f':
                    {
                        double f = va_arg(ap, double); /* passed as double, not float */
                        fprintf(stdout, "%f", f);
                        break;
                    }
                case 's':
                    {
                        char *s = va_arg(ap, char*);
                        fputs(s, stdout);
                        break;
                    }
                case 'm':
                    {
                        int m = va_arg(ap, int);
                        int n = va_arg(ap, int);
                        if (m == -1 && n == -1)
                            fputs("PASS", stdout);
                        else if ((m<0) || (n<0) || (m>=gtp_boardsize) || (n>=gtp_boardsize))
                            fprintf(stdout, "??");
                        else
                            fprintf(stdout, "%c%d", 'A' + n + (n >= 8), gtp_boardsize - m);
                        break;
                    }
                case 'C':
                    {
                        int color = va_arg(ap, int);
                        if (color == WHITE)
                            fputs("white", stdout);
                        else if (color == BLACK)
                            fputs("black", stdout);
                        else
                            fputs("empty", stdout);
                        break;
                    }
                default:
                    fprintf(stdout, "\n\nUnknown format character '%c'\n", *fmt);
                    break;
            }
        }
        else
            putc(*fmt, stdout);
    }
    va_end(ap);
}


/* This currently works exactly like printf. */
void
gtp_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    fflush(stdout);
}


/* Write success or failure indication plus identity number if one was
 * given.
 */
void
gtp_printid(int id, int status)
{
    if (status == GTP_SUCCESS)
        gtp_printf("=");
    else
        gtp_printf("?");

    if (id < 0)
        gtp_printf(" ");
    else
        gtp_printf("%d ", id);
}


/* Finish a GTP response by writing a double newline and returning GTP_OK. */
int
gtp_finish_response()
{
    gtp_printf("\n\n");
    return GTP_OK;
}


/* Write a full success response. Except for the id number, the call
 * is just like one to printf.
 */
int
gtp_success(int id, const char *format, ...)
{
    va_list ap;
    gtp_printid(id, GTP_SUCCESS);
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    return gtp_finish_response();
}


/* Write a full failure response. The call is identical to gtp_success. */
int
gtp_failure(int id, const char *format, ...)
{
    va_list ap;
    gtp_printid(id, GTP_FAILURE);
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    return gtp_finish_response();
}


/* Write a panic message. */
void
gtp_panic()
{
    gtp_printf("! panic\n\n");
}


/* Convert a string describing a color, "b", "black", "w", or "white",
 * to GNU Go's integer representation of colors. Return the number of
 * characters read from the string s.
 */
int
gtp_decode_color(char *s, int *color)
{
    char color_string[7];
    int i;
    int n;

    assert(gtp_boardsize > 0);

    if (sscanf(s, "%6s%n", color_string, &n) != 1)
        return 0;

    for (i = 0; i < (int) strlen(color_string); i++)
        color_string[i] = tolower((int) color_string[i]);

    if (strcmp(color_string, "b") == 0
            || strcmp(color_string, "black") == 0)
        *color = BLACK;
    else if (strcmp(color_string, "w") == 0
            || strcmp(color_string, "white") == 0)
        *color = WHITE;
    else
        return 0;

    return n;
}


/* Convert an intersection given by a string to two coordinates
 * according to GNU Go's convention. Return the number of characters
 * read from the string s.
 */
int
gtp_decode_coord(char *s, int *i, int *j)
{
    char column;
    int row;
    int n;

    assert(gtp_boardsize > 0);

    if (sscanf(s, " %c%d%n", &column, &row, &n) != 2)
        return 0;

    if (tolower((int) column) == 'i')
        return 0;
    *j = tolower((int) column) - 'a';
    if (tolower((int) column) > 'i')
        --*j;

    *i = gtp_boardsize - row;

    if (*i < 0 || *i >= gtp_boardsize || *j < 0 || *j >= gtp_boardsize)
        return 0;

    return n;
}

/* Convert a move, i.e. "b" or "w" followed by a vertex to a color and
 * coordinates. Return the number of characters read from the string
 * s.
 */
int
gtp_decode_move(char *s, int *color, int *i, int *j)
{
    int n1, n2;

    assert(gtp_boardsize > 0);

    n1 = gtp_decode_color(s, color);
    if (n1 == 0)
        return 0;

    n2 = gtp_decode_coord(s + n1, i, j);
    if (n2 == 0)
        return 0;

    return n1 + n2;
}

/* This a bubble sort. Given the expected size of the sets to
 * sort, it's probably not worth the overhead to set up a call to
 * qsort.
 */

static void
sort_moves(int n, int movei[], int movej[])
{
    int b, a;
    for (b = n-1; b > 0; b--) {
        for (a = 0; a < b; a++) {
            if (movei[a] > movei[b]
                    || (movei[a] == movei[b] && movej[a] > movej[b])) {
                int tmp;
                tmp = movei[b];
                movei[b] = movei[a];
                movei[a] = tmp;
                tmp = movej[b];
                movej[b] = movej[a];
                movej[a] = tmp;
            }
        }
    }
}

/* Write a number of space separated vertices. The moves are sorted
 * before being written.
 */
void
gtp_print_vertices(int n, int movei[], int movej[])
{
    int k;

    assert(gtp_boardsize > 0);

    sort_moves(n, movei, movej);
    for (k = 0; k < n; k++) {
        if (k > 0)
            gtp_printf(" ");
        if (movei[k] == -1 && movej[k] == -1)
            gtp_printf("PASS");
        else if (movei[k] < 0 || movei[k] >= gtp_boardsize
                || movej[k] < 0 || movej[k] >= gtp_boardsize)
            gtp_printf("??");
        else
            gtp_printf("%c%d", 'A' + movej[k] + (movej[k] >= 8),
                    gtp_boardsize - movei[k]);
    }
}

/* Write a single move. */
void
gtp_print_vertex(int i, int j)
{
    gtp_print_vertices(1, &i, &j);
}
