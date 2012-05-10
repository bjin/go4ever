#include "uct.hpp"
#include "board.cpp"
#include <cstring>
#include <cstdio>

static board_t *board_list[max_board];
static double board_value[max_board];
static int child[max_board];
static int brother[max_board];
static int visit_num[max_board];
static int board_num;

void initialize()
{
}


//  find a child node of bi, which has the maximum value
node* descend_by_UCB1(node* n)
{
    int total = 0;
    node* temp = *n->child;
    while (temp != null) {
        total = total + *temp.visit_time;
        temp = *temp->brother;
    }
    temp = *n->child;
    double max_value;
    int max_index;
    bool found = false;
    while (temp != null) {
        if (*temp.visit_time == 0) {
            return temp;
        }
        int vt = *temp.visit_time;
        double value = -(*temp.value)/vt+sqrt(2*log(total)/vt);
        if (!found || max_value < value) {
            max_index = temp;
            max_value = value;   
        }
        temp = *temp->brother;
    }
    /*
    int next_move = gen_move(*n->board,color);
    if (next_move!= -1){
        //fork a new board, play one step, 
        board_t nb = new board_t();
        fork_board(&nb, board_list[bi]);
        put_stone(&nb, next_move, color, true);
        // nb is the child of boardlist[bi]
        //  |                   |
        //  |                   |   
        //  V                   V
        // board_num            bi
        board_num++;
        board_list[board_num - 1] = &nb;
        if (child[bi] == -1){
            child[bi] = board_num - 1;                    
        } else {
            brother[board_num - 1] = child[bi];
            child[bi] = board_num - 1;
        }
        return board_list[board_num - 1];
    }

    // if there exists a infinity value just pick it and return
    // otherwise we choose the max value among all children nodes
    int total = 0;
    int c = child[bi];
    while (c != -1) {
        total = total + visit_num[c];
        c = brother[c];
    }
    c = child[bi];
    double max_value = -10000, temp;
    int max_index = -1;
    while (c != -1){
        temp = -board_value[c]/visit_num[c]+sqrt(2*log(total)/visit_num[c]);
        if (temp > max_value || max_index == -1) {
            max_index = c;
            max_value = temp;
        }
        c = brother[c];
    }
    return board_list[max_index];*/
}

static node* node_list[max_depth];
void playOneSequence(node* root)
{
    int i = 0;
    node_list[0] = root;
    do {
        node_list[i+1] = descend_by_UCB1(node_list[i]);
        i++;
    }while (*node_list[i].visit_time != 0);
    *node_list[i].visit_time++;
    create_node(node[i]);
    *node_list[i].value = get_value_by_MC(node_list[i]);
    update_value(i,-*node_list[i].value);
}

void update_value(int depth, double value)
{
    for (int i = depth - 1; i >= 0; i--) {
        *node_list[i].value = *node_list[i].value + value;
        *node_list[i].visit_time = *node_list[i].visit_time + 1;
        value = -value;
    }
}

void create_node(node* n)
{
     
}

double get_value_by_MC(node* n)
{
    
}

void clean_subtree(node* n)
{
    
}
