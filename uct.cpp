#include "uct.hpp"
#include "board.hpp"
#include "random.hpp"
#include "gtp.hpp"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <ctime>

void init_node(node* n)
{

    n->simple.initial();
    n->rave.initial();
    n->child = NULL;
    n->brother = NULL;
}

static board_t* root_board = new board_t, *temp_board = new board_t;
static bool print = false, print2 = false;
static int *temp_moves = new int[moves_total*2+1];
static stone_t global_color;
static index_t move_rec[steps_total+10];
static stone_t color_rec[steps_total+10];
static bool move_set[moves_total*2+1];
static bool move_set_to[moves_total*2+1];
static index_t move_total[moves_total*2+1];
static index_t move_score[moves_total*2+1];
static node *root = NULL;
static int step;
static node* node_list[max_depth];

stone_t oppo_color(stone_t color)
{
    if (color == STONE_BLACK)
        return STONE_WHITE;
    else
        return STONE_BLACK;
}

index_t get_id(stone_t color, index_t move)
{
    if (color == STONE_WHITE)
        return move;
    return move+moves_total;
}

float_num neg_value(float_num value)
{
    return -value;
}

float_num subjective_score(stone_t color, float_num score)
{
    if (color == STONE_BLACK)
        return neg_value(score);
    return score;
}

static int print_count = 0;

void update_rave(int nodes_num, float_num value)
{
    node* temp_node, *loop_node;
    int last = (int)(step * coeff);
    for (int k = 0; k <= nodes_num; k++){
        memset(move_set, false, sizeof(move_set));
        memset(move_set_to, false, sizeof(move_set_to));
        temp_node = node_list[k];
        // update values to all children of temp_node
        for (int q = k+1; q < last; q++){
            index_t move_id = get_id(color_rec[q], move_rec[q]);
            move_set[move_id] = !move_set_to[move_id];
            move_set_to[move_id] = true;
            move_set_to[get_id(oppo_color(color_rec[q]), move_rec[q])] = true;
        }
        loop_node = temp_node->child;
        while (loop_node != NULL){
            if (move_set[get_id(color_rec[k+1],loop_node->move)]) {
                //update_count_rave(loop_node,subjective_score(oppo_color(color_rec[k+1]),value));
                loop_node->rave.add_sample(subjective_score(oppo_color(color_rec[k+1]),value));
            }
            loop_node = loop_node->brother;
        }
    }

}

void update_winrate(float_num value)
{
    int last = (int)(step * coeff);
    for (int i = 1; i < last; i++) {
        index_t move_id = get_id(color_rec[i], move_rec[i]);
        move_total[move_id]++;
        move_score[move_id]+=subjective_score(color_rec[i],value);
    }
}

float_num consider_winrate(float_num value, index_t move_id)
{
    if (move_total[move_id] == 0)
        return value;
    return value + ((float_num)move_score[move_id])/move_total[move_id];
}

//  find a child node of bi, which has the maximum value
node* descend_by_UCB1(node* n,stone_t color)
{
    int total = n->simple.count;
    float_num max_value = 0;
    node* max_index = NULL;
    bool found = false;
    float_num log_total_m2;
    if (total == 0)
        log_total_m2 = 0;
    else
        log_total_m2 = log(total);
    float_num value;
    node *temp = n->child;
    while (temp != NULL) {
        int vt = temp->simple.count+temp->rave.count;
        if (vt == 0){
            value = 10000+fast_irandom(1000);
        } else {
            value = -temp->mix_value(log_total_m2,0.5,0.01);//+sqrt((log_total_m2)/vt)*tree_search_coeff;
            //gtp_printf("move=%d,\tvalue=%f\n",temp->move,value);
            //value = -(temp->value)/vt+sqrt((log_total_m2)/vt)*tree_search_coeff;
            if (consider_winrate_bool) {
                index_t move_id = get_id(color,temp->move);
                value = consider_winrate(value,move_id);
            }
        }
        if ((!found) || max_value < value) {
            max_index = temp;
            max_value = value;
            found = true;
        }
        temp = temp->brother;
    }
    return max_index;
}

static double deps = 0;
static double dep_count = 0;

void play_one_sequence(node* root,stone_t color)
{

    fork_board(temp_board,root_board);
    int i = 0;
    node_list[0] = root;
    do {
        node_list[i+1] = descend_by_UCB1(node_list[i],color);
        if (node_list[i+1] == NULL) {
            return;
        }
        move_rec[i+1] = node_list[i+1]->move;
        color_rec[i+1] = color;
        put_stone(temp_board,node_list[i+1]->move,color);
        color = oppo_color(color);
        i++;
    }while (node_list[i]->child != NULL);
    deps += i;
    dep_count++;
    //gtp_printf("depth=\t%d\t%d\n",i,node_list[1]->move);
    create_node(temp_board, node_list[i], color);
    float_num white_value = get_value_by_MC(temp_board,color);
    //node_list[i]->value = subjective_score(color,white_value);
    //node_list[i]->nb = 1;
    //update_value(i, -node_list[i]->value);
    update_value(i, -subjective_score(color,white_value));
    update_rave(i, white_value);
    if (consider_winrate_bool)
        update_winrate(white_value);
}

void update_value(int depth, float_num value)
{
    for (int i = depth - 1; i >= 0; i--) {
        node_list[i]->simple.add_sample(value);
        //node_list[i]->value = node_list[i]->value + value;
        //node_list[i]->nb++;
        value = -value;
    }
}

void create_node(board_t* b, node* n,stone_t color)
{
    if (n->child != NULL)
        return;
    int len = gen_moves(b, color, temp_moves, true);
    for (int i = 0; i < len; i++){
        if (is_eyelike_3x3(b->nbr3x3[temp_moves[i]],color)) continue;
        node* new_node = new node;
        init_node(new_node);
        new_node->move = temp_moves[i];
        new_node->brother = n->child;
        n->child = new_node;
    }
}


bool on_edge(index_t move)
{
    if (GETX(root_board,move) < edge_width || GETX(root_board,move) >= root_board->size -edge_width)
        return true;
    if (GETY(root_board,move) < edge_width || GETY(root_board,move) >= root_board->size -edge_width)
        return true;
    return false;
}
static bool black_passed,white_passed;
static float_num bcnt,wcnt;
static int pos;
static stone_t color;
static int bs, ws;
static board_t* gb = new board_t;

float_num get_value_by_MC(board_t* temp_board,stone_t next_color)
{
    int tmp_step = step;
    bcnt = 0, wcnt = 0;
    for (int i = 0; i < simulation_times; i++){
        step = tmp_step;
        fork_board(gb,temp_board);
        black_passed = false;
        white_passed = false;
        color = next_color;
        while((!black_passed || !white_passed) && step < steps_total) {
            pos = color == STONE_WHITE && white_passed ||
                    color == STONE_BLACK && black_passed ? -1 : gen_move(gb, color);
            if (pos >= 0) {
                color_rec[step] = color;
                move_rec[step++] = pos;
                put_stone(gb, pos, color);
            }
            if (color == STONE_BLACK) {
                black_passed = (pos < 0);
                color = STONE_WHITE;
            } else if (color == STONE_WHITE) {
                white_passed = (pos < 0);
                color = STONE_BLACK;
            }
        }
        calc_final_score(gb, bs, ws);
        return ws+6.5-bs;
        if(bs > ws + 6.5){
            bcnt= bcnt + 1;
        }
        else {
            wcnt= wcnt + 1;
        }
    }
    return ws+6.5-bs;
    //return (wcnt)/(wcnt+bcnt);
    /*if (wcnt > 0.01)
        return 1;
    else
        return 0;*/
}

void clean_subtree(node* n)
{
    if (n == NULL) return;
    node* tp = n->child, *p;
    while (tp != NULL) {
        p = tp->brother;
        clean_subtree(tp);
        tp = p;
    }
    delete n;
}

static int steps = 0;

index_t next_move(node* root, stone_t color, index_t pre_move)
{
    steps++;
    fork_board(temp_board, root_board);
    create_node(temp_board, root, color);
    double time_out = clock() + 8 * CLOCKS_PER_SEC;
    int time_count = 0;
    deps = 0; dep_count = 0;
    while (clock() < time_out) {
        step = 0;
        play_one_sequence(root, color);
        time_count++;
        //if (time_count > 5) break;
    }
    node *max_node = NULL;
    float_num max_value = -100000;
    float_num tmp;
    node *temp = root->child;
    while (temp != NULL){
        //tmp = -temp->simple.sum/temp->simple.count;
        tmp = -temp->simple.calc_mean()*coeff_simple-temp->rave.calc_mean()*coeff_rave;
        if (consider_winrate_bool){
            index_t move_id = get_id(color,temp->move);
            tmp = consider_winrate(tmp,move_id);
        }
        //gtp_printf("move = %d,\tvalue= %f,\tnum=%d,\tvariance=%f\n",temp->move,temp->simple.sum,temp->simple.count,temp->rave.calc_variance());
        float_num d = root_board->prob_sum2[color-1][temp->move];
        if (d > 0){
            if (steps < 50)
                d = log(d)* 0.01 + 1;
            else
                d = log(d)* 0.02 + 1;
        } else {
            d = 0.001;
        }
        if (tmp > 0)
            tmp = tmp * d;
        else
            tmp = tmp / d;
        /*if (on_edge(temp->move) && steps < 20){
            if (tmp > 0){
                tmp = tmp * 0.95;
            } else {
                tmp = tmp * 1.05;
            }

        }*/
        if (max_node == NULL || max_value < tmp){
            max_node = temp;
            max_value = tmp;
        }
        temp = temp->brother;
    }
    temp = root->child;
    node * tt;
    int tota = 0;
    while (temp != NULL){
        tt = temp->brother;
        if (temp != max_node){
            clean_subtree(temp);
        }
        temp = tt;
    }
    root->child = max_node;
    if (max_node == NULL)
        return -1;
    max_node ->brother = NULL;
    root = max_node;
    gtp_printf("result=%d\n",max_node->move);
    gtp_printf("\tsimple: count=%d/%d,\tmean=%f\n",max_node->simple.count,time_count,max_node->simple.calc_mean());
    gtp_printf("\trave: count=%d,\tmean=%f\n",max_node->rave.count,max_node->rave.calc_mean());
    gtp_printf("\tavg_depth=%f,\t%f/%f\n",deps/dep_count,deps,dep_count);
    return max_node->move;
}

index_t next_move(board_t* b,stone_t color,index_t pre_move)
{
    memset(move_score,0,sizeof(move_score));
    memset(move_total,0,sizeof(move_total));
    if (root == NULL || pre_move == -1){
        root = new node;
        init_node(root);
    } else {
        node* temp = root->child;
        root = NULL;
        while (temp != NULL){
            if (temp->move == pre_move){
                //gtp_printf("found pre move%d\n",temp->nb);
                root = temp;
                break;
            }
            temp = temp->brother;
        }
        if (root == NULL){
            root = new node;
            init_node(root);
        }
    }
    print = true; print2 = true;
    fork_board(root_board,b);
    global_color = color;
    index_t ans = next_move(root,global_color,pre_move);
    gtp_printf("\t%f\n",b->prob_sum2[color-1][pos]);
    return ans;
}

