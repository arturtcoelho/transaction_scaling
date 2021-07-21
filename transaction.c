#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef long mask_t;

#define MAX_TRANSACTIONS sizeof(mask_t) * 8
#define MAX_SCALING sizeof(mask_t) * 8

typedef struct {
    int timestamp, transaction;
    char operation, atribute;
} transaction_t;

typedef struct {
    transaction_t s[MAX_TRANSACTIONS][MAX_SCALING];
    int transactions;
    int time;
} scaling_t;

typedef struct {
    char g[MAX_TRANSACTIONS][MAX_TRANSACTIONS];
    int n;
} graph_t;

void read_scaling(scaling_t *);
void print_transaction(transaction_t *);
void print_scaling(scaling_t *);
int find_conflict(scaling_t *, char, char, int, int, int);
mask_t test_serial_conflict(scaling_t *);
int get_last_write(transaction_t *, int);
mask_t equiv_vision(scaling_t *, int, int);
mask_t test_vision(scaling_t *);
void print_results(scaling_t *, mask_t, mask_t);
mask_t check_graph_loop(graph_t *);
int seach_graph(graph_t *, mask_t, int, int, int);

int main()
{    
    scaling_t scaling;
    read_scaling(&scaling);
    // print_scaling(&scaling);

    mask_t serial_res = test_serial_conflict(&scaling);
    mask_t vision_res = test_vision(&scaling);

    print_results(&scaling, serial_res, vision_res);

    return 0;
}

void clear_scaling(scaling_t *s)
{
    for (int i = 0; i < MAX_TRANSACTIONS; i++) {
        for (int j = 0; j < MAX_SCALING; j++) {
            s->s[i][j].timestamp = 0;
            s->s[i][j].transaction = 0;
            s->s[i][j].operation = 0;
            s->s[i][j].atribute = 0;
        }
    }
}

void read_scaling(scaling_t *s)
{
    clear_scaling(s);

    transaction_t buff;
    int i = 0;
    int t = 0;
    while (!feof(stdin)) {
        scanf("%d %d %c %c", &buff.timestamp, 
                                &buff.transaction, 
                                &buff.operation, 
                                &buff.atribute);
        buff.timestamp--;
        buff.transaction;
        if (buff.transaction > t) t = buff.transaction;
        if (t > MAX_TRANSACTIONS || i > MAX_SCALING) break;

        if (buff.timestamp < i) {
            i++;
            break;
        }

        memcpy(&s->s[buff.timestamp][buff.transaction-1], 
                &buff, sizeof(transaction_t));
        i++;
    }    

    if (i == MAX_SCALING) {
        fprintf(stderr, "Buffer overflow, please increase scaling buffer size\n");
        exit(1);
    }

    if (t == MAX_TRANSACTIONS) {
        fprintf(stderr, "Buffer overflow, please increase transaction buffer size\n");
        exit(1);
    }

    s->time = i-1;
    s->transactions = t;
}

void print_transaction(transaction_t *t)
{
    if (t->transaction) {
        printf("%c ", t->transaction+48);
        printf("%c ", t->operation);
        printf("%c ", t->atribute);
    } else {
        printf("%6c", ' ');
    }
}

void print_scaling(scaling_t *s)
{
    for (int i = 0; i < s->time; i++) {
        printf("%3d: ", i+1);
        for (int j = 0; j < s->transactions; j++) {
            print_transaction(&s->s[i][j]);
        }
        printf("\n");   
    }
    
}

mask_t test_serial_conflict(scaling_t *s)
{

    graph_t gr;
    gr.n = s->transactions;
    for (int i = 0; i < s->transactions; i++) {
        for (int j = 0; j < s->transactions; j++) {
            gr.g[i][j] = 0;
        }
    }

    int conflict = 0;
    int base = 0;
    mask_t transactions_active = 0;
    for (int j = 0; j < s->transactions; j++) {
        for (int i = 0; i < s->time; i++) {
            
            transactions_active |= 1 << j;
            switch (s->s[i][j].operation) {
                case '\0':
                    break;
                case 'R':
                    conflict = find_conflict(s, 'R', s->s[i][j].atribute, j, i, base);
                    if (conflict >= 0) {
                        gr.g[conflict][j] = 1;
                    }
                    break;
                case 'W':
                    conflict = find_conflict(s, 'W', s->s[i][j].atribute, j, i, base);
                    if (conflict >= 0) {
                        gr.g[conflict][j] = 1;
                    }
                    break;
                case 'C':
                    transactions_active &= 1 << j;
                    if (!transactions_active) base = j;
                    break;
                default:
                    fprintf(stderr, "Operation error (%d)\n", s->s[i][j].operation);
                    exit(1);
                    break;
            }
        }
    }

    // // Print graph
    // printf("\n");
    // printf("Conflict graph:\n");
    // for (int i = 0; i < s->transactions; i++) {
    //     printf("%3d: ", i);
    //     for (int j = 0; j < s->transactions; j++) {
    //         printf("%c ", gr.g[i][j]+48);
    //     }
    //     printf("\n");
    // }
    // printf("\n");

    return check_graph_loop(&gr);

}

int find_conflict(scaling_t *s, char op, char at, int trans, int time, int base)
{
    for (int i = 0; i < s->time && i < time; i++) {
        for (int j = base; j < s->transactions; j++) {
            if (j == trans || !s->s[i][j].operation || s->s[i][j].atribute != at) continue;

            if ((op == 'R' && s->s[i][j].operation == 'W') || op == 'W') {
                return j;
            }
        }
    }
    return -1;
}

mask_t check_graph_loop(graph_t *gr)
{
    mask_t res = 0;
    for (int i = 0; i < gr->n; i++) {
        if (seach_graph(gr, 0, i, i, 0)) {
            res |= 1 << i;
        }   
    }     
    return res;
}

int seach_graph(graph_t *gr, mask_t path, int curr, int obj, int lim)
{
    if (path && curr == obj) return 1;
    for (int i = 0; i < gr->n; i++) {
        if (i != curr && 
            gr->g[curr][i] && 
            lim < gr->n*gr->n &&
            seach_graph(gr, path | 1 << curr, i, obj, lim+1)) {
            return 1;
        }
    }
    return 0;
}

int compar_transaction(const void *a, const void *b)
{
    transaction_t *A = (transaction_t*)a;
    transaction_t *B = (transaction_t*)b;
    if (A->transaction > B->transaction) {
        return 1;
    }
    return 0;
}

int compar_transaction_inverted(const void *a, const void *b)
{
    transaction_t *A = (transaction_t*)a;
    transaction_t *B = (transaction_t*)b;
    if (A->transaction < B->transaction) {
        return 1;
    }
    return 0;
}

int get_last_write(transaction_t *s, int index)
{
    char at = s[index].atribute;
    int trans = s[index].transaction;
    for (int i = index-1; i >= 0; i--) {
        if (s[i].atribute == at && s[i].operation == 'W') {
            return i;
        }
    }

    return -1;
}

mask_t equiv_vision(scaling_t *s, int beg, int max)
{
    mask_t ret = 0;

    transaction_t sl[s->time];
    transaction_t s_cpy[s->time];
    for (int i = 0; i < s->time; i++) {
        sl[i].timestamp = 0;
        sl[i].transaction = 0;
        sl[i].operation = 0;
        sl[i].atribute = 0;

    }
    

    // printf("\nScaling:\n");

    int sl_transaction = 0;
    int sl_time = 0;
    int last = 0;

    for (int i = beg; i < max+1; i++) {
        for (int j = 0; j < s->transactions; j++) {
            if (s->s[i][j].operation) {
                ret |= s->s[i][j].transaction;
                memcpy(&sl[sl_time], &s->s[i][j], sizeof(transaction_t));
                sl_time++;
            }
        }
    }

    memcpy(s_cpy, &sl, sizeof(transaction_t)*sl_time);
    qsort(&sl, sl_time, sizeof(transaction_t), compar_transaction);

    // for (int i = 0; i < sl_time; i++){
    //     print_transaction(&sl[i]);
    //     printf("\n");
    // }

    int i = sl_time-1;
    while (sl[i].operation != 'W' && i >= 0) {
        i--;
    }
    if (i != -1) {
        for (int j = sl_time-1; j >= 0; j--) {
            if (s_cpy[j].operation == 'W') {
                if (sl[i].transaction != s_cpy[j].transaction){
                    if (sl[i].atribute == s_cpy[j].atribute){
                        return ret;
                    }
                } 
                break;
            }
        }
    }


    for (int i = 0; i < sl_time; i++) {
        if (sl[i].operation == 'R') {
            // print_transaction(&sl[i]);
            // printf("\n");
            int w1 = get_last_write(sl, i);
            int w2 = get_last_write(s_cpy, sl[i].timestamp);
            // printf("w1 = %d, w2 = %d\n", w1, w2);
            if (w1 != w2) {
                return ret;
            }
        }
    }

    return 0;
}

mask_t test_vision(scaling_t *s)
{
    int last = 0;
    mask_t transactions_active = 0;
    mask_t res = 0;

    for (int i = 0; i < s->time; i++) {
        for (int j = 0; j < s->transactions; j++) {
            if (!transactions_active && i) {
                res |= equiv_vision(s, last, i);
                last = i+1;
                transactions_active |= 1 << j+1;
            }

            char op = s->s[i][j].operation;

            if (op == 'R' || op == 'W') {
                transactions_active |= 1 << j;
            } else if (op == 'C') {
                transactions_active ^= 1 << j;
            }

        }
    }

    if (!transactions_active) {
        res |= equiv_vision(s, last, s->time);
    }

    return res;
}

void print_results(scaling_t *s, 
                    mask_t serial_res, 
                    mask_t vision_res)
{
    // printf("%ld, %ld\n", serial_res, vision_res);

    mask_t cases[] = {serial_res & vision_res,
                        serial_res & ~vision_res,
                        ~serial_res & vision_res,
                        ~serial_res & ~vision_res};

    // for (int i = 0; i < 4; i++) {
    //     printf("%ld\n", cases[i]);
    // }

    char strs[][6] = {"NS NV", "NS SV", "SS NV", "SS SV"};

    int flag;
    int scalings = 0;

    for (int j = 0; j < 4; j++) {
        flag = 0;
        for (int i = 0; i < s->transactions; i++) {
            if (cases[j] & 1 << i){
                if (flag == 0) {
                    flag = 1;
                    scalings++;
                    printf("%d %d", scalings, i+1);
                } else {
                    printf(",%d", i+1);
                }

            }
        }
        if (flag) {
            printf(" %s\n", strs[j]);
        }
    }

}
