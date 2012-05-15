#ifndef UCT_H
#define UCT_H

#include "board.hpp"
#include <cmath>
#define max_board 100000
#define max_depth 500
#define null (-1)
#define simulation_times 1
#define factor 1
#define edge_width 2
#define coeff 0.75
#define moves_total 250
#define steps_total 350
#define tree_search_coeff 1
#define consider_winrate_bool false
#define coeff_simple 1
#define coeff_rave 0.3

struct stat {
    float_num sum,square_sum;
    int count;
    float_num calc_mean()
    {
        if (count == 0)
            return 0;
        else
            return (sum/count);
    }
    float_num calc_variance(){
        if (count == 0) return 0;
        float_num mean = calc_mean();
        return (square_sum/count)-mean*mean;
    }
    void add_sample(float_num value){
        count++;
        sum += value;
        square_sum += value*value;
    }
    void initial(){
        count = 0;
        sum = 0;
        square_sum = 0;
    }
};

struct node {
    node* child;
    node* brother;
    stat simple, rave;
    //float_num value,rave_value;
    //int nb,rave_nb;
    index_t move;
    //float_num sum,rave_sum;
    float_num mix_value(float_num log_total, float_num mix_ratio1, float_num mix_ratio2){

        stat stat1 = simple;
        stat stat2 = rave;

        float b1 = mix_ratio1;
        float b2 = mix_ratio2;
        float n1 = stat1.count;
        float n2 = stat2.count;

        float s1 = stat1.sum;
        float s2 = stat2.sum;

        float v1 = stat1.square_sum;
        float v2 = stat2.square_sum;

        float nn1 = n1 * n1;
        float nn2 = n2 * n2;

        float x1 = (v1 + b1*nn1) * n1  -  s1 * s1;
        float x2 = (v2 + b2*nn2) * n2  -  s2 * s2;

        float t1 = nn1 * x2;
        float t2 = nn2 * x1;
        if (fabs(t1*n1+t2*n2) <= 0.0001){
            return -100000;
        }
        float mix = (t1*s1 + t2*s2) / (t1*n1 + t2*n2);
        return mix;
    }
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

