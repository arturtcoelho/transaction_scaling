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
void print_transaction(scaling_t *, int, int);
void print_scaling(scaling_t *);
int find_error(scaling_t *, char, char, int, int, int);
mask_t test_serial_conflict(scaling_t *);
mask_t test_vision(scaling_t *);
void print_results(scaling_t *, mask_t, mask_t);
mask_t check_graph_loop(graph_t *);
int seach_graph(graph_t *, mask_t, int, int);

int main()
{    
    scaling_t scaling;
    read_scaling(&scaling);
    print_scaling(&scaling);

    mask_t serial_res = test_serial_conflict(&scaling);
    mask_t vision_res = test_vision(&scaling);

    print_results(&scaling, serial_res, vision_res);

    return 0;
}

void read_scaling(scaling_t *s)
{
    for (int i = 0; i < MAX_TRANSACTIONS; i++) {
        for (int j = 0; j < MAX_SCALING; j++) {
            s->s[i][j].timestamp = 0;
            s->s[i][j].transaction = 0;
            s->s[i][j].operation = 0;
            s->s[i][j].atribute = 0;
        }
    }

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
        if (s->s[buff.timestamp][buff.transaction].timestamp) {
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

void print_transaction(scaling_t *s, int i, int j)
{
    printf("%c ", s->s[i][j].transaction ? 
                    s->s[i][j].transaction+48 : ' ');
    printf("%c ", s->s[i][j].operation ? 
                    s->s[i][j].operation : ' ');
    printf("%c ", s->s[i][j].atribute ? 
                    s->s[i][j].atribute : ' ');
}

void print_scaling(scaling_t *s)
{
    for (int i = 0; i < s->time; i++) {
        printf("%3d: ", i+1);
        for (int j = 0; j < s->transactions; j++) {
            print_transaction(s, i, j);
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
    int limit = 0;
    mask_t transactions_active = 0;
    for (int j = 0; j < s->transactions; j++) {
        for (int i = 0; i < s->time; i++) {
            
            transactions_active |= 1 << j;
            switch (s->s[i][j].operation) {
                case '\0':
                    break;
                case 'R':
                    conflict = find_error(s, 'R', s->s[i][j].atribute, j, i, base);
                    if (conflict >= 0) {
                        // printf("ALO R %d %d\n", conflict, j);
                        gr.g[conflict][j] = 1;
                    }
                    break;
                case 'W':
                    conflict = find_error(s, 'W', s->s[i][j].atribute, j, i, base);
                    if (conflict >= 0) {
                        // printf("ALO W %d %d\n", conflict, j);
                        gr.g[conflict][j] = 1;
                    }
                    break;
                case 'C':
                    transactions_active &= i << j;
                    if (!transactions_active) base = j;
                    break;
                default:
                    fprintf(stderr, "Operation error (%d)\n", s->s[i][j].operation);
                    exit(1);
                    break;
            }
        }
    }

    // Print graph
    printf("\n");
    printf("Conflict graph:\n");
    for (int i = 0; i < s->transactions; i++) {
        printf("%3d: ", i);
        for (int j = 0; j < s->transactions; j++) {
            printf("%c ", gr.g[i][j]+48);
        }
        printf("\n");
    }

    return check_graph_loop(&gr);

}

int find_error(scaling_t *s, char op, char at, int trans, int time, int base)
{
    // printf("search: %c %c %d %d %d\n", op, at, trans, time, base);
    for (int i = 0; i < s->time && i < time; i++) {
        for (int j = base; j < s->transactions; j++) {
            if (j == trans || !s->s[i][j].operation || s->s[i][j].atribute != at) continue;

            if ((op == 'R' && s->s[i][j].operation == 'W') || op == 'W') {
                // printf("res: %d %d %d\n", trans, i, j);
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
        if (seach_graph(gr, 0, i, i)) {
            res |= 1 << i;
        }   
    }     
    return res;
}

int seach_graph(graph_t *gr, mask_t path, int curr, int obj)
{
    if (path && curr == obj) return 1;
    for (int i = 0; i < gr->n; i++) {
        if (i != curr && 
            gr->g[curr][i] && 
            seach_graph(gr, path | 1 << curr, i, obj)) {
            return 1;
        }
    }
    return 0;
    
}

mask_t test_vision(scaling_t *s)
{
    
}

void print_results(scaling_t *s, 
                    mask_t serial_res, 
                    mask_t vision_res)
{
    mask_t serial_ok = 0;

    int flag = 0;

    for (int i = 0; i < s->transactions; i++) {
        if (serial_res & 1 << i) {
            if (flag) {
                printf(",");
            } else {
                printf("1 ");
            }
            printf("%d", i+1);
            flag = 1;
        } else {
            serial_ok |= 1 << i;
        }
    }
    if (flag) printf(" NS\n");
    
    flag = 0;

    for (int i = 0; i < s->transactions; i++) {
        if (serial_ok & 1 << i) {
            if (flag) {
                printf(",");
            } else {
                printf("2 ");
            }
            printf("%d", i+1);
            flag = 1;
        }
    }
    if (flag) printf(" SS\n");

}
