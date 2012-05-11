#include "uct.hpp"
#include "board.hpp"
#include "random.hpp"
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
}

//  find a child node of bi, which has the maximum value
node* descend_by_UCB1(node* n)
{
    int total = 0;
    node* temp = n->child;
    while (temp != NULL) {
        total = total + temp->nb;
        temp = temp->brother;
    }
    temp = n->child;
    double max_value;
    node* max_index;
    bool found = false;
    while (temp != NULL) {
        if (temp->nb == 0) {
            return temp;
        }
        int vt = temp->nb;
        double value = -(temp->value)/vt+sqrt(2*log(total)/vt);
        if (!found || max_value < value) {
            max_index = temp;
            max_value = value;
            found = true;
        }
        temp = temp->brother;
    }
}

static node* node_list[max_depth];
void play_one_sequence(node* root,stone_t color)
{
    int i = 0;
    node_list[0] = root;
    do {
        node_list[i+1] = descend_by_UCB1(node_list[i]);
        if (color == STONE_BLACK) {
            color = STONE_WHITE;
        } else {
            color = STONE_BLACK;
        }
        i++;
    }while (node_list[i]->child != NULL);
    node_list[i]->nb++;
    create_node(node_list[i],color);
    node_list[i]->value = get_value_by_MC(node_list[i],color);
    update_value(i,-node_list[i]->value);
}

void update_value(int depth, double value)
{
    for (int i = depth - 1; i >= 0; i--) {
        node_list[i]->value = node_list[i]->value + value;
        node_list[i]->nb = node_list[i]->nb + 1;
        value = -value;
    }
}

void create_node(node* n,stone_t color)
{
    int *moves = new int[300];
    int len = gen_moves(n->board,color,moves,true);
    for (int i = 0; i < len; i++) {
        node* new_node = new node;
        init_node(new_node);
        board_t* new_board = new board_t;
        fork_board(new_board,n->board);
        put_stone(new_board,moves[i],color);
        new_node->board = new_board;
        new_node->parent = n;
        new_node->brother = n->child;
        new_node->move = moves[i];
        n -> child = new_node;
    }
}

float_num get_value_by_MC(node* n,stone_t next_color)
{
    timeval *start = new timeval;
    timeval *end = new timeval;
    gettimeofday(start, NULL);
    fast_srandom(start->tv_sec);
    int bcnt = 0, wcnt = 0;
    board_t* b = n->board;
    for (int i = 0; i < simulation_times; i++) {
        int steps = 0;
        bool black_passed = false;
        bool white_passed = false;
        stone_t color = next_color;
        while((!black_passed || !white_passed) && steps < b->size * b->size + 10) {
            int tries = b->size / 2 + 1;
            while (tries > 0) {
                index_t pos = gen_move(b, color);
                if (pos >=0) {
                    put_stone(b, pos, color);
                    break;
                }
                tries--;
            }
        if (color == STONE_BLACK) {
            black_passed = tries <= 0;
            color = STONE_WHITE;
        } else if (color == STONE_WHITE) {
            white_passed = tries <= 0;
            color = STONE_BLACK;
        }
        steps ++;
        }
        int bs, ws;
        calc_final_score(b, bs, ws);
        if(bs > ws + 2.5)
            bcnt ++;
        else
            wcnt ++;
    }
    delete b;
    if (next_color == STONE_BLACK)
        return (bcnt >= wcnt);
    // return (bcnt/times);
    else
        return (wcnt >= bcnt);
    // return (wcnt/times)
}

void clean_subtree(node* n)
{
    if (n == NULL) return;
    node* temp = n -> child;
    while (temp != NULL) {
        clean_subtree(temp);
        temp = temp->brother;
    }
    delete n->board;
    delete n;
}

index_t next_move(node* root, stone_t color)
{
    if (root->child == NULL){
        create_node(root,color);
    }
    int time_out = 50;
    while (time_out > 0){
        time_out--;
        play_one_sequence(root, color);
    }
    node* temp = root->child, *max_node = NULL;
    while (temp != NULL){
        if (max_node == NULL || max_node->value < temp->value){
            max_node = temp;
        } else {
        }
        temp = temp->brother;
    }
    temp = root->child;
    while (temp != NULL){
        if (temp != max_node){
            clean_subtree(temp);
        }

    }
    return max_node->move;
}

index_t next_move(board_t* b,stone_t color)
{
    node* root = new node;
    init_node(root);
    root->board = b;
    return next_move(root,color);
}

