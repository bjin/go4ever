#ifndef UCT_H
#define UCT_H

#define max_board 100000
#define max_depth 500

struct node {
    board_t* board;
    node* child = null,brother = null;
    double value;
    int visit_time = 0;
}


void initialize();

//  find a child node of bi, which has the maximum value
node* descend_by_UCB1(node* n);

//  gradually find a chain from root to a leaf node, 
//  create all of the leaf node's child node,
//   

void play_one_sequence(node* root);

//  discard all nodes in the subtree
void clean_subtree(node* n);

void update_value(double value);

void create_node(node* n);

void get_value_by_MC(node* n);
#endif
