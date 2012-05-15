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
    n->nb = 0;
    n->child = NULL;
    n->brother = NULL;
    n->value = 0;
}

static board_t* root_board = new board_t, *temp_board = new board_t;
static bool print = false, print2 = false;
static int *temp_moves = new int[1000];
static stone_t global_color;
static float move_score[1000];
static int move_total[1000];
static index_t move_rec[1000];
static node *root = NULL;
static int step;

//  find a child node of bi, which has the maximum value
node* descend_by_UCB1(node* n,stone_t color)
{
    int total = 0;
    node* temp = n->child;
    while (temp != NULL) {
        total = total + temp->nb;
        temp = temp->brother;
    }
    temp = n->child;
    float_num max_value = 0;
    node* max_index = NULL;
    bool found = false;
    float_num log_total_m2;
    if (total == 0)
        log_total_m2 = 0;
    else
        log_total_m2 = log(total);
    float_num value;
    while (temp != NULL) {
        int vt = temp->nb;

        if (vt == 0){
            value = 10000+fast_irandom(1000);
        } else {
            value = -(temp->value)/vt+sqrt((log_total_m2)/vt);
            if (move_total[temp->move] != 0){
                value = value + move_score[temp->move]/move_total[temp->move];
            }
        }
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


void play_one_sequence(node* root,stone_t color)
{
    fork_board(temp_board,root_board);
    int i = 0;
    node_list[0] = root;
    step = 0;
    do {
        /*if (print) {
            gtp_printf("time_out\n");
        }*/
        node_list[i+1] = descend_by_UCB1(node_list[i],color);
        if (node_list[i+1] == NULL)
            return;
        if (global_color == color){
            move_rec[step++] = node_list[i+1]->move;
        }
        /*if (print) {
            gtp_printf("time_out\n");
        }*/
        put_stone(temp_board,node_list[i+1]->move,color);
        if (color == STONE_BLACK) {
            color = STONE_WHITE;
        } else {
            color = STONE_BLACK;
        }
        i++;
    }while (node_list[i]->nb != 0);
    create_node(temp_board, node_list[i], color);
    node_list[i]->value = get_value_by_MC(temp_board,color);
    //gtp_printf("%f\n",node_list[i]->value);
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

void create_node(board_t* b, node* n,stone_t color)
{
    if (n->child != NULL)
        return;
    if (n->nb != 0) return;
    n->nb = 1;
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
        while((!black_passed || !white_passed) && step < 1000) {
            pos = color == STONE_WHITE && white_passed ||
                    color == STONE_BLACK && black_passed ? -1 : gen_move(gb, color);
            if (pos >= 0) {
                if (global_color == color){
                    move_rec[step++] = pos;
                }
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

        //gtp_printf("finalscore:%d,%d\n",bs,ws);
        if(bs > ws + 6.5){
            for (int k = 0; k < step; k++){
                move_total[move_rec[k]]++;
                if (global_color == STONE_BLACK){
                    //gtp_printf("get_rec_black_%d\n",move_rec[k]);
                    move_score[move_rec[k]]++;
                }
            }
            bcnt= bcnt + 1.0;
        }
        else {
            for (int k = 0; k < step; k++){
                move_total[move_rec[k]]++;
                if (global_color == STONE_WHITE){
                    //gtp_printf("get_rec_white_%d\n",move_rec[k]);
                    move_score[move_rec[k]]++;
                }
            }
            wcnt= wcnt+ 1.0;
        }
    }
    if (next_color == STONE_BLACK)
        return (bcnt/simulation_times);
    else
        return (wcnt/simulation_times);
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

stone_t oppo_color(stone_t color)
{
    if (color == STONE_BLACK)
        return STONE_WHITE;
    else
        return STONE_BLACK;
}

static int steps= 0;
static bool visited[max_len];
int ans = 0;

void comp_count(board_t* b, index_t pos, stone_t color)
{
    if (visited[pos]){
        return ;
    }
    if (b->stones[pos] != STONE_BLACK && b->stones[pos] != STONE_WHITE){
        return ;
    }
    if (b->stones[pos] == color){
        ans++;
        visited[pos] = true;
        comp_count(b, N(b,pos), color);
        comp_count(b, W(b,pos), color);
        comp_count(b, E(b,pos), color);
        comp_count(b, S(b,pos), color);
    }

}

float_num interrupting_point(board_t* b,index_t pos,stone_t color)
{
    for (int i = 0; i < max_len;i++){
        visited[i] = false;
    }
    int op_comp = 0, op_count = 0;
    int comp = 0, count = 0;
    int nb_pos;
    int tot_count = 0,tot_count_op = 0;
    nb_pos = N(b,pos);
    if (color == b->stones[nb_pos]){
        ans = 0;
        comp_count(b, nb_pos, b->stones[nb_pos]);
        tot_count += ans;
        if (ans > 0) comp++;
        count++;
    } else if (oppo_color(color) == b->stones[nb_pos]){
        ans = 0;
        comp_count(b, nb_pos, b->stones[nb_pos]);
        if (ans > 0) op_comp++;
        tot_count_op += ans;
        op_count++;
    }
    nb_pos = S(b,pos);
    if (color == b->stones[nb_pos]){
        ans = 0;
        comp_count(b, nb_pos, b->stones[nb_pos]);
        tot_count += ans;
        if (ans > 0) comp++;
        count++;
    } else if (oppo_color(color) == b->stones[nb_pos]){
        ans = 0;
        comp_count(b, nb_pos, b->stones[nb_pos]);
        if (ans > 0) op_comp++;
        tot_count_op += ans;
        op_count++;
    }
    nb_pos = E(b,pos);
    if (color == b->stones[nb_pos]){
        ans = 0;
        comp_count(b, nb_pos, b->stones[nb_pos]);
        tot_count += ans;
        if (ans > 0) comp++;
        count++;
    } else if (oppo_color(color) == b->stones[nb_pos]){
        ans = 0;
        comp_count(b, nb_pos, b->stones[nb_pos]);
        if (ans > 0) op_comp++;
        tot_count_op += ans;
        op_count++;
    }
    nb_pos = W(b,pos);
    if (color == b->stones[nb_pos]){
        ans = 0;
        comp_count(b, nb_pos, b->stones[nb_pos]);
        tot_count += ans;
        if (ans > 0) comp++;
        count++;
    } else if (oppo_color(color) == b->stones[nb_pos]){
        ans = 0;
        comp_count(b, nb_pos, b->stones[nb_pos]);
        if (ans > 0) op_comp++;
        tot_count_op += ans;
        op_count++;
    }
    float_num p = 0;
    if (op_comp >= 2 && comp > 0){
        //gtp_printf("found interruption 1\n");
        p = p + (tot_count+tot_count_op)/(10.0);
        if (op_comp> 2)
            p = p / 1.5;
        if (op_comp < op_count){
            p = p*2 + 0.1;
        }
    }
    if (comp > 1 && op_comp > 0){
        //gtp_printf("found interruption 2\n");
        p = p + (tot_count+tot_count_op)/(10.0);
        if (op_comp < op_count){
            p = p * 2 + 0.1;
        }
    }
    return p;
}

static int airs;
static int c_airs;

void air_count(board_t* b, index_t pos, stone_t color)
{
    if (visited[pos]){
        return;
    }
    visited[pos] = true;
    if (b->stones[pos] == STONE_EMPTY){
        airs++;
        return;
    }
    if (b->stones[pos] != color) {
        return;
    }
    c_airs++;
    air_count(b,N(b,pos),color);
    air_count(b,E(b,pos),color);
    air_count(b,S(b,pos),color);
    air_count(b,W(b,pos),color);
}

float_num forming_atari(board_t* b, index_t pos, stone_t color)
{
    if (color != b->stones[pos])
        return 0;
    for (int i = 0; i <max_len; i++)
        visited[i] = false;
    board_t* temp_b = new board_t;
    fork_board(temp_b, b);
    put_stone(temp_b,pos,color);
    airs = 0; c_airs = 0;
    air_count(temp_b,pos,color);
    delete temp_b;
    if (airs <= 1){
        //gtp_printf("forming_atari\n");
        return -c_airs;
    }
    return 0;
}

index_t next_move(node* root, stone_t color, index_t pre_move)
{
    steps++;
    //return gen_move(root_board, color);
    fork_board(temp_board, root_board);
    create_node(temp_board, root, color);
    double time_out = clock() + 8 * CLOCKS_PER_SEC;
    while (clock() < time_out) {
        play_one_sequence(root, color);
    }
    node *max_node = NULL;
    float_num max_value = 0;
    float_num tmp;
    node *temp = root->child;
    while (temp != NULL){
        if (move_total[temp->move] == 0){
            tmp = 0;
        } else {
            tmp = move_score[temp->move]/move_total[temp->move];
        }
        float_num d = root_board->prob_sum2[global_color-1][temp->move];
        /*if (d > 0) {
            if (steps < 30)
                d = log(d) * 0.01 + 1;
            else
                d = log(d) * 0.02 + 1;
        }

        else
            d = 0.0001;*/
        //gtp_printf("move= %d,value= %f,tmp= %f,d= %f,nb= %d\n\r",temp->move,-temp->value/temp->nb,tmp,d,temp->nb);
        tmp = -temp->value/temp->nb + tmp;
        /*if (tmp > 0 )
            tmp = tmp * d;
        else
            tmp = tmp / d;*/
        //tmp *= root_board->prob_sum2[global_color-1][temp->move];
        /*if (is_atari_of_3x3(root_board->nbr3x3[temp->move],global_color)){
            gtp_printf("found atari!!\n");
            tmp = tmp + 0.1;
        }*/
        //tmp += interrupting_point(temp_board,temp->move,global_color);
        //tmp += forming_atari(temp_board,temp->move,global_color);
        /*stone_t oppocolor = global_color;
        if (global_color == STONE_BLACK)
            oppocolor = STONE_WHITE;
        else
            oppocolor = STONE_BLACK;
        fork_board(temp_board,root_board);

        put_stone(temp_board,temp->move,global_color);
        tmp -= forming_atari(temp_board,N(temp_board,temp->move),oppo_color(global_color))/10;
        tmp -= forming_atari(temp_board,S(temp_board,temp->move),oppo_color(global_color))/10;
        tmp -= forming_atari(temp_board,E(temp_board,temp->move),oppo_color(global_color))/10;
        tmp -= forming_atari(temp_board,W(temp_board,temp->move),oppo_color(global_color))/10;
        if (is_atari_of_3x3(root_board->nbr3x3[temp->move],oppocolor)){
            gtp_printf("found oppo atari!!\n");
            tmp = tmp + 0.2;
        }*/
        //tmp = tmp + temp->nb / 5000.0;
        if (on_edge(temp->move) && steps < 30){
            if (tmp > 0)
                tmp = tmp * 0.95;
            else
                tmp = tmp * 1.05;
        }
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
    /*if (!is_legal_move(root_board, max_node->move, global_color, true)){
        gtp_printf("what!!!\n");
        for (int i = 0; i >= 0; i++){
            if (i == 5) i = 0;
        }
    }*/
    if (max_node == NULL)
        return -1;
    //gtp_printf("final_result=%d\t%d\t%f\n",max_node->move,max_node->nb,max_value);

    max_node ->brother = NULL;
    root = max_node;
    /*if (root->nb == 0){
        put_stone(root_board,max_node->move,global_color);
        global_color = oppo_color(global_color);
        create_node(root_board,root,global_color);
    }*/
    return max_node->move;
}

void init_uct()
{
    for (int i = 0; i < 1000; i++){
        move_score[i] = 0;
        move_total[i] = 0;
    }
}

index_t next_move(board_t* b,stone_t color,index_t pre_move)
{
    init_uct();
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
    //gtp_printf("%f\n",b->prob_sum2[color-1][pos]);
    return ans;
}

