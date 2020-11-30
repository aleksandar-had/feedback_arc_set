/**
 * @file fb_arc_set.c
 * @author Aleksandar Hadzhiyski <e1426981@student.tuwien.ac.at>
 * @date 21.11.2020
 *  
 * @brief Feedback arc set module.
 * 
 * @details The feedback arc set module provides the variable and function
 * definitions of the variables and objects declared in the feedback arc set header
 * module.
 */

#include "fb_arc_set.h"

/** 
 * ---------------------------------------------------------------------------------
 *                            Helper functions implementations
 *  ---------------------------------------------------------------------------------
 */

void usage(char *prog)
{

    if (strcmp(prog, "./generator") == 0)
    {
        fprintf(stderr, "Usage: %s EDGE1 EDGE2...\n", prog);
        exit(EXIT_FAILURE);
    }
    else if (strcmp(prog, "./supervisor") == 0)
    {
        fprintf(stderr, "Usage: %s takes no arguments!\n", prog);
        exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(stderr, "Unknown program!\n");
        exit(EXIT_FAILURE);
    }
}

int cmpfunc(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int val_in_arr(int val, int *arr, size_t size)
{
    for (int i = 0; i < size; i++)
    {
        if (arr[i] == val)
            return 1;
    }
    return 0;
}

int src_from_arg(char *arg)
{
    int i, n;
    n = 0;

    for (i = 0; arg[i] >= '0' && arg[i] <= '9' && arg[i] != '-'; ++i)
        n = 10 * n + (arg[i] - '0');
    return n;
}

int trgt_from_arg(char *arg)
{
    int i, n;
    n = i = 0;

    while (arg[i] != '-')
        ++i;
    ++i;

    for (; arg[i] >= '0' && arg[i] <= '9' && arg[i] != '\0'; i++)
        n = 10 * n + (arg[i] - '0');
    return n;
}

/**
 * @details The function makes use of regex to assert the correct edge format.
 */
int assert_edge_format(char *arg)
{
    int val;
    regex_t regex;
    int reti = regcomp(&regex, "[0-9]-[0-9]", REG_EXTENDED);
    if (reti != 0)
    {
        fprintf(stderr, "Edge argument formatting cannot be asserted due to REGEX compilation failure!\n");
        exit(EXIT_FAILURE);
    }
    val = regexec(&regex, arg, 0, NULL, 0);
    if (val == REG_NOMATCH)
        return 0;
    return 1;
}

int generate_random(int lower, int upper)
{
    return ((rand() % (upper - lower + 1)) + lower);
}

/**
 * @details The function iterates through the elements in the array and swaps an element
 * with another one which is positioned after the first element.
 */
void shuffle_vertex_set(int *set, size_t size)
{

    int rand_pos, temp, i;

    for (i = 0; i < size; i++)
    {
        rand_pos = generate_random(i, size - 1);
        temp = set[i];
        set[i] = set[rand_pos];
        set[rand_pos] = temp;
    }
}

void print_solution(Edge edge_set[], char *prog, int size)
{
    fprintf(stdout, "[%s] Solution with %d edges:", prog, size);
    for (int i = 0; i < size; ++i)
        fprintf(stdout, " %d-%d", edge_set[i].src, edge_set[i].trgt);
    fprintf(stdout, "\n");
}

/** 
 * ---------------------------------------------------------------------------------
 *                                Graph_ptr functions implementations
 * ---------------------------------------------------------------------------------
 *
 * Implementations based on suggestions from a Yale course in C involving directed graphs
 * computation. 
 */

Graph_ptr graph_create(int n)
{
    Graph_ptr g;
    int i;

    g = malloc(sizeof(struct Graph_s) + sizeof(struct successors *) * (n - 1));
    assert(g);

    g->V = n;
    g->E = 0;

    for (i = 0; i < n; i++)
    {
        g->alist[i] = malloc(sizeof(struct successors));
        assert(g->alist[i]);

        g->alist[i]->d = 0;
        g->alist[i]->len = 1;
        g->alist[i]->is_sorted = true;
    }

    return g;
}

void graph_destroy(Graph_ptr g)
{
    int i;

    for (i = 0; i < g->V; i++)
        free(g->alist[i]);
    free(g);
}

void graph_add_edge(Graph_ptr g, int u, int v)
{
    assert(u >= 0);
    assert(u < g->V);
    assert(v >= 0);
    assert(v < g->V);

    while (g->alist[u]->d >= g->alist[u]->len)
    {
        g->alist[u]->len *= 2;
        g->alist[u] = realloc(g->alist[u], sizeof(struct successors) + sizeof(int) * (g->alist[u]->len - 1));
    }

    g->alist[u]->list[g->alist[u]->d++] = v;
    g->alist[u]->is_sorted = false;

    g->E++;
}

int graph_vertex_count(Graph_ptr g)
{
    return g->V;
}

int graph_edge_count(Graph_ptr g)
{
    return g->E;
}

int graph_out_degree(Graph_ptr g, int source)
{
    assert(source >= 0);
    assert(source < g->V);

    return g->alist[source]->d;
}

int graph_has_edge(Graph_ptr g, int source, int target)
{
    int i;

    if (graph_out_degree(g, source) >= BSEARCH_PROMPT_SIZE)
    {
        /** 
         * in case the source has too many neighbors, instead of doing a linear search,
         * first sort the adjacency list and after that, use the built-in binary search
         */
        if (!g->alist[source]->is_sorted)
            qsort(g->alist[source]->list, g->alist[source]->d, sizeof(int), cmpfunc);
        return bsearch(&target, g->alist[source]->list, g->alist[source]->d, sizeof(int), cmpfunc) != 0;
    }
    else
    {
        /**
         * for a small enough number of neighbors, we can simply not bother to optimize searching,
         * instead do a straight-forward linear comparison search
         */
        for (i = 0; i < g->alist[source]->d; i++)
        {
            if (g->alist[source]->list[i] == target)
                return 1;
        }
        return 0;
    }
}

/**
 * ---------------------------------------------------------------------------------
 *                          Semaphore functions implementations                      
 * ---------------------------------------------------------------------------------
 */

sem_t *open_sem(char *sem_name, size_t sem_size, int mode)
{
    sem_t *sem;

    if (mode == 0)
        sem = sem_open(sem_name, O_CREAT | O_EXCL, 0600, sem_size);
    else
        sem = sem_open(sem_name, sem_size);

    if (sem == SEM_FAILED)
    {
        fprintf(stderr, "ERROR: Semaphore creation failed!\n");
        exit(EXIT_FAILURE);
    }
    return sem;
}

void getval_sem(sem_t *sem, int *res)
{
    if (sem_getvalue(sem, res) == -1)
    {
        fprintf(stderr, "ERROR: Semaphore value fetch failed!\n");
        exit(EXIT_FAILURE);
    }
}

void wait_sem(sem_t *sem)
{
    if (sem_wait(sem) == -1)
    {
        if (errno == EINTR)
            return;
        fprintf(stderr, "ERROR: Semaphore decrement/lock failed!\n");
        exit(EXIT_FAILURE);
    }
}

void post_sem(sem_t *sem)
{
    if (sem_post(sem) == -1)
    {
        fprintf(stderr, "ERROR: Semaphore increment/unlock failed!\n");
        exit(EXIT_FAILURE);
    }
}

void close_sem(sem_t *sem)
{
    if (sem_close(sem) == -1)
    {
        fprintf(stderr, "ERROR: Semaphore closing failed!\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * ---------------------------------------------------------------------------------
 *                       Shared memory functions implementations                     
 * ---------------------------------------------------------------------------------
 */

int create_shm(char *shm_name)
{
    int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0600);
    if (shm_fd == -1)
    {
        fprintf(stderr, "ERROR: Shared memory object creation failed!\n");
        exit(EXIT_FAILURE);
    }
    return shm_fd;
}

void *open_shm(int shm_fd, size_t shm_size)
{
    void *shm;
    shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (shm == MAP_FAILED)
    {
        fprintf(stderr, "ERROR: Failed mapping shared memory object\n");
        exit(EXIT_FAILURE);
    }

    return shm;
}

void truncate_shm(int shm_fd, size_t shm_size)
{
    if (ftruncate(shm_fd, shm_size) < 0)
    {
        fprintf(stderr, "ERROR: Failed truncating shared memory object\n");
        exit(EXIT_FAILURE);
    }
}

void write_to_shm(void *shm, char *data, size_t data_size)
{
    memcpy(shm, data, data_size);
}

void unmap_shm(void *shm, size_t shm_size)
{
    if (munmap(shm, shm_size) == -1)
    {
        fprintf(stderr, "ERROR: Failed unmapping shared memory object\n");
        exit(EXIT_FAILURE);
    }
}
