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
 * obtaining a copy of this file gtp.h, to deal in the Software  *
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

#include <stdarg.h>

/* Maximum allowed line length in GTP. */
#define GTP_BUFSIZE 1000

/* Status returned from callback functions. */
#define GTP_QUIT    -1
#define GTP_OK       0
#define GTP_FATAL    1

/* Whether the GTP command was successful. */
#define GTP_SUCCESS  0
#define GTP_FAILURE  1

/* Function pointer for callback functions. */
typedef int (*gtp_fn_ptr)(char *s, int id);

/* Elements in the array of commands required by gtp_main_loop. */
struct gtp_command {
  const char *name;
  gtp_fn_ptr function;
};

void play_gtp();
static int gtp_set_komi(char *s, int id);
static int gtp_final_score(char *s, int id);
static int gtp_set_level(char *s, int id);
static int gtp_version(char *s, int id);
static int gtp_undo(char *s, int id);
static int gtp_protocol_version(char *s, int id);
static int gtp_name(char *s, int id);
static int gtp_playblack(char *s, int id);
static int gtp_playwhite(char *s, int id);
static int genmove(int *i, int *j, int side);
static void play_move(int i, int j, int side);
static int gtp_genmove_black(char *s, int id);
static int gtp_genmove_white(char *s, int id);
static int gtp_help(char *s, int id);
static int gtp_quit(char *s, int id);
static int gtp_showboard(char *s, int id);
static int gtp_set_boardsize(char *s, int id);

void gtp_main_loop(struct gtp_command commands[]);
void gtp_internal_set_boardsize(int size);
void gtp_mprintf(const char *format, ...);
void gtp_printf(const char *format, ...);
void gtp_printid(int id, int status);
int gtp_finish_response(void);
int gtp_success(int id, const char *format, ...);
int gtp_failure(int id, const char *format, ...);
void gtp_panic(void);
int gtp_decode_color(char *s, int *color);
int gtp_decode_coord(char *s, int *m, int *n);
int gtp_decode_move(char *s, int *color, int *i, int *j);
void gtp_print_vertices(int n, int movei[], int movej[]);
void gtp_print_vertex(int i, int j);

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
