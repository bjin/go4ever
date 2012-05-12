#include "uct.hpp"
#include "board.hpp"
#include "random.hpp"
#include "gtp.hpp"
#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include <cmath>

void init_node(node* n)
{
    n->nb = 0;
    n->child = NULL;
    n->brother = NULL;
    n->parent = NULL;
    n->offset = -1;
}

static board_t* root_board = new board_t, *temp_board = new board_t;

static int *temp_moves = new int[200];
static stone_t global_color;
static float move_score[500];
static int move_total[500];
static node *root = NULL;

void test(index_t& offset)
{
    offset++;
}

//  find a child node of bi, which has the maximum value
node* descend_by_UCB1(node* n,stone_t color)
{
    //int len = gen_moves(temp_board,color,temp_moves,true);
    //gtp_printf("decendbyUCB%d\n",n->next_expand);
    //gtp_printf("test%d",n->offset);
    //test(n->offset);
    //gtp_printf("test%d",n->offset);
    index_t move = gen_moves_next(temp_board,color,n->offset,true);
    //gtp_printf("decendbyUCB%d,%d\n",n->offset,move);
    if (move != -1){
        // still has some children unexpanded
        node* new_node = new node;
        init_node(new_node);
        new_node->move = move;
        new_node->parent = n;
        new_node->brother = n->child;
        n->child = new_node;
        return new_node;
	}
	//gtp_printf("out\n");
    float_num total = 0;
    node* temp = n->child;
    while (temp != NULL) {
        total = total + temp->nb;
        temp = temp->brother;
    }
    //gtp_printf("out\n");
    temp = n->child;
    float_num max_value = 0;
    //gtp_printf("total=%d\t",total);
    node* max_index;
    bool found = false;
    float_num log_total_m2 = 2*log(total);
    while (temp != NULL) {
        int vt = temp->nb;
        float_num value = -(temp->value)/vt+sqrt(log_total_m2/vt);
        //gtp_printf("%f\t",value);
        if (!found || max_value < value) {
            max_index = temp;
            max_value = value;
            found = true;
        }
        temp = temp->brother;
    }
    //gtp_printf("\t%lf",max_value);
    return max_index;
}

static node* node_list[max_depth];
int depth_count[30];

void play_one_sequence(node* root,stone_t color)
{
    //gtp_printf("descending\n");
    fork_board(temp_board,root_board);
    //gtp_printf("descending\n");
    int i = 0;
    node_list[0] = root;
    int depth = 0;
    do {
        depth++;
        node_list[i+1] = descend_by_UCB1(node_list[i],color);
        put_stone(temp_board,node_list[i+1]->move,color);
        //gtp_printf("\t%d",node_list[i+1]->move);
        if (color == STONE_BLACK) {
            color = STONE_WHITE;
        } else {
            color = STONE_BLACK;
        }
        i++;
    }while (node_list[i]->offset >= 0);
    //gtp_printf("\n");
    depth_count[depth]++;
    create_node(node_list[i],color);
    node_list[i]->value = get_value_by_MC(temp_board,color)-0.5;
    //gtp_printf("get_MC_value:%f\n",node_list[i]->value);
    update_value(i,-node_list[i]->value);
}

void update_value(int depth, float_num value)
{
    for (int i = depth - 1; i >= 0; i--) {
        node_list[i]->value = node_list[i]->value + value;
        node_list[i]->nb++;
        value = -value;
    }
}

static int total = 0;

void create_node(node* n,stone_t color)
{
    n->offset = 0;
    n->nb = 1;
}

static index_t move_rec[300];
static int step;

float_num get_value_by_MC(board_t* bo,stone_t next_color)
{
    timeval *start = new timeval;
    timeval *end = new timeval;

    gettimeofday(start, NULL);
    fast_srandom(start->tv_sec);
    float_num bcnt = 0.0, wcnt = 0.0;
    board_t* b = new board_t;
    for (int i = 0; i < simulation_times; i++) {
        fork_board(b, bo);
        int steps = 0;
        bool black_passed = false;
        bool white_passed = false;
        stone_t color = next_color;
        int pos;
        step = 0;
        while((!black_passed || !white_passed)) {
            pos = gen_move(b, color, true);
            if (pos >= 0) {
                if (step < 10) {
                    if (global_color == color){
                        move_rec[step++] = pos;
                    }
                }
                put_stone(b, pos, color);
            }
            if (color == STONE_BLACK) {
                black_passed = (pos < 0);
                color = STONE_WHITE;
            } else if (color == STONE_WHITE) {
                white_passed = (pos < 0);
                color = STONE_BLACK;
            }
        }
        int bs, ws;
        calc_final_score(b, bs, ws);
        //gtp_printf("finalscore:%d,%d\n",bs,ws);
        if(bs > ws + 6.5){
            for (int k = 0; k < step; k++){
                move_total[move_rec[k]]++;
                if (global_color == STONE_BLACK){
                    //gtp_printf("get_rec_black_%d\n",move_rec[k]);
                    move_score[move_rec[k]]++;
                }
            }
            bcnt= bcnt + 1;
        }
        else {
            for (int k = 0; k < step; k++){
                move_total[move_rec[k]]++;
                if (global_color == STONE_WHITE){
                    //gtp_printf("get_rec_white_%d\n",move_rec[k]);
                    move_score[move_rec[k]]++;
                }
            }
            wcnt= wcnt + 1;
        }
    }
    delete b;
    delete start;
    delete end;
    if (next_color == STONE_BLACK)
        return (bcnt/simulation_times);
    else
        return (wcnt/simulation_times);
}

void clean_subtree(node* n)
{
    if (n == NULL) return;
    node* temp = n -> child;
    while (temp != NULL) {
        clean_subtree(temp);
        temp = temp->brother;
    }
    //delete n->board;
    delete n;
}

static int steps = 0;

index_t next_move(node* root, stone_t color)
{
    steps++;
    if (steps < 50) {
        return gen_move(root_board, color, true);
    }
    for (int i = 0; i < 30; i++){
        depth_count[i] = 0;
    }
    if (root->offset == NULL){
        fork_board(temp_board,root_board);
        create_node(root,color);
    }
    int time_out = 1000;
    while (time_out > 0){
        time_out--;
        play_one_sequence(root, color);
    }

    node* temp = root->child, *max_node = NULL;
    float_num max_value = 0;
    float_num tmp;
    while (temp != NULL){
        if (move_total[temp->move] == 0){
            tmp = 0;
        } else {
            tmp = 0;//move_score[temp->move]/move_total[temp->move];
        }
        gtp_printf("move=%d, %f, %d\n",temp->move,tmp,temp->nb);
        if (max_node == NULL || max_value < temp->value + tmp){
            max_node = temp;
            max_value = temp->value + tmp;
        }
        temp = temp->brother;
    }
    gtp_printf("\n");
    temp = root->child;
    while (temp != NULL){
        if (temp != max_node){
            clean_subtree(temp);
        }
        temp = temp->brother;
    }
    //gtp_printf("after_clean\n");
    return max_node->move;
}

void init_uct()
{
    for (int i = 0; i < 300; i++){
        move_score[i] = 0;
        move_total[i] = 0;
    }
}

index_t next_move(board_t* b,stone_t color)
{
    //gtp_printf("next_move\n");
    init_uct();
    if (root == NULL){
        root = new node;
        init_node(root);
    }
    //gtp_printf("yes\n");
    root_board = b;
    global_color = color;
    index_t ans = next_move(root,color);
    root = NULL;
    for (int i = 0; i < 30; i++){
        gtp_printf("depth=%d,\tcount=%d\n", i, depth_count[i]);
    }
    return ans;
}

