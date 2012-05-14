#ifndef UCT_H
#define UCT_H

#include "board.hpp"

#define max_board 100000
#define max_depth 500
#define null (-1)
#define simulation_times 1
#define factor 1
#define edge_width 2
#define coeff 0.75
#define moves_total 250

struct node {
    node* child;
    node* brother;
    float_num value;
    int nb;
    index_t move;
};



void init_node(node* n);

//  find a child node of bi, which has the maximum value
node* descend_by_UCB1(node* n);

//  gradually find a chain from root to a leaf node,
//  create all of the leaf node's child node,
//

void play_one_sequence(node* root,stone_t color);

//  discard all nodes in the subtree
void clean_subtree(node* n);

void update_value(int depth,float_num value);

void create_node(board_t* b, node* n,stone_t color);

float_num get_value_by_MC(board_t* n,stone_t color);

index_t next_move(board_t* root,stone_t color, index_t pre_move);

#endif

