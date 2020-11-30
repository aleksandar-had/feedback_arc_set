/**
 * @file generator.c
 * @author Aleksandar Hadzhiyski <e1426981@student.tuwien.ac.at>
 * @date 21.11.2020
 *  
 * @brief Generator program module.
 * @details The generator module is responsible for the result generation given a graph.
 * It makes sure that a directed graph has been properly created and that results are 
 * not overlapping i.e. only results that are better than the previously generated ones 
 * are forwarded to the shared memory ring buffer. The generator runs as long as it is 
 * terminated via the shared atomic "quit" variable or an acyclic solution has been
 * passed to the shared memory ring buffer. In both cases, the termination of the 
 * generator program is preceeded by closing of all shared resources used by it.
 * 
 * USAGE: The generator takes at least one edge as arguments.
 * generator EDGE1 ...
 */

#include "fb_arc_set.h"

/**
 * @brief a global atomic variable that indicates when a program must quit
 */
volatile sig_atomic_t quit = 0;

/**
 * @brief Definition of the shared memory file descriptor and ring buffer.
 */
static int shm_buf_fd;
static Buffer *ring_buf;

/**
 * @brief Definition of the three semaphores needed for the fb_arc_set assignment.
 */
static sem_t *excl_sem;
static sem_t *used_sem;
static sem_t *free_sem;

/**
 * Free before exit function.
 * @brief This function frees up the occupied shared ressources.
 * @details The function is used in the signal handling of SIGKILL and SIGTERM.
 * After such a signal has been sent by the user, this function ensures that
 * the generator(s) close the free space, used space and mutual exclusion semaphores
 * which they opened and also unmap the shared memory ring buffer.
 * @param none
 * @return none
 */
static void free_before_exit(void)
{
    close_sem(free_sem);
    sem_unlink(FREE_SEM);

    close_sem(used_sem);
    sem_unlink(USED_SEM);

    close_sem(excl_sem);
    sem_unlink(EXCL_SEM);

    unmap_shm(ring_buf, sizeof(Buffer));
    shm_unlink(RING_BUF);
    close(shm_buf_fd);
}

/**
 * Signal handler function.
 * @brief This function is called when a program receives a signal.
 * @details The function is called, to indicate that the caller program 
 * must be stopped, hence the global atomic quit variable is set to 1.
 * @param none
 * @return none
 **/
void signal_handler(int signum)
{
    quit = 1;
}

/**s
 * Signal processing function.
 * @brief This functions handles the signals that the user sends.
 * @details According to good practices, the program response to the SIGINT and SIGTERM
 * signals is changed.
 * @param none
 * @return none
 **/
void process_signal(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/**
 * Program entry point.
 * @brief This is the main program of the generator module.
 * @details The program performs all of the generator functions as described in the
 * task requirements. Initially, it checks that the generator program has been
 * called correctly, afterwards it sets up the graph and all shared memory objects and
 * semaphores. The main tasks are performed in the program loop which monitors if the
 * shared atomic variable "quit" is set to true or an acyclic result has been saved in
 * the shared memory ring buffer. The program uses a Monte carlo randomized algorithm to
 * shuffle the vertices and then pick generate a feedback arc set. All feedback arc sets
 * that are worse (bigger) than the locally best (smallest) one are discarded.
 * The program makes sure that if a better solution than the one present in the shared 
 * memory ring buffer has been computed, that it will be written to the ring buffer
 * without a race condition.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS.
 */
int main(int argc, char *argv[])
{
    char *prog = argv[0];

    int src, trgt;
    int i, k;
    int num_e;
    int num_v;

    if (argc < 2)
    {
        usage(prog);
    }

    if (atexit(free_before_exit) != 0)
    {
        fprintf(stderr, "ERROR: Free before exit function failed!\n");
        exit(EXIT_FAILURE);
    }

    process_signal();

    /**
     * Assume in worst memory case scenario, each edge introduces 2 new vertices.
     * A type of a bipartite graph.
     */
    int max_set[2 * argc];

    for (i = 1, k = 0; i < argc; i++)
    {
        if (!assert_edge_format(argv[i]))
        {
            fprintf(stderr, "[%s] ERROR: Edge parameter formatted incorrectly!\n", prog);
            usage(prog);
        }
        src = src_from_arg(argv[i]);
        trgt = trgt_from_arg(argv[i]);

        if (!val_in_arr(src, max_set, k))
            max_set[k++] = src;

        if (!val_in_arr(trgt, max_set, k))
            max_set[k++] = trgt;
    }

    int vertex_set[k]; /**< very convenient since k depicts the number of elements due to the last indexing being k++ */
    num_v = k;
    num_e = argc - 1;

    for (i = 0; i < num_v; i++)
        vertex_set[i] = max_set[i];

    Graph_ptr g = graph_create(num_v);

    /**
     * Assert the graph is created correctly.
     */
    assert(graph_vertex_count(g) == num_v);
    assert(graph_edge_count(g) == 0);

    /**
     * Create edges in the graph.
     */
    for (i = 1; i < argc; i++)
    {
        src = src_from_arg(argv[i]);
        trgt = trgt_from_arg(argv[i]);

        graph_add_edge(g, src, trgt);
    }

    /**
     * Assert that all edges were created.
     */
    assert(graph_edge_count(g) == num_e);

    srand(time(0)); /**< generate random seed, only once per generator */

    /**
     * Initialize edges array and fb arc set counters.
     */
    Edge edge_set[num_e];
    int fb_size = 0;

    int best_fb_size = num_e - 1; /**< worst case scenario */
    int calc_fb_size = 0;
    bool better_sol_ex = false;

    /**
     * Shared memory objects definitions.
     */
    int write_at = 0;
    shm_buf_fd = create_shm(RING_BUF);
    ring_buf = (Buffer *)open_shm(shm_buf_fd, sizeof(Buffer));

    /**
     * Semaphores definitions.
     */
    excl_sem = open_sem(EXCL_SEM, 1, 1);
    used_sem = open_sem(USED_SEM, 0, 1);
    free_sem = open_sem(FREE_SEM, BUF_SIZE, 1);

    while (quit != 1 && ring_buf->acyclic != true && ring_buf->quit != 1)
    {
        shuffle_vertex_set(vertex_set, num_v);

        calc_fb_size = 0;

        /**
         * Check for edges applying the algorithm described in the task.
         */
        for (int i = 1, edge_set_i = 0; i < num_v; ++i)
        {
            for (int j = 0; j < i; ++j)
            {
                if (graph_has_edge(g, vertex_set[i], vertex_set[j]))
                {
                    edge_set[edge_set_i].src = vertex_set[i];
                    edge_set[edge_set_i].trgt = vertex_set[j];
                    ++edge_set_i;
                    ++calc_fb_size;
                }
            }
            /**
             * Stop preemptively if the best local feedback arc set size has already
             * been reached.
             */
            if (calc_fb_size >= best_fb_size)
            {
                better_sol_ex = true;
                break;
            }
        }
        if (better_sol_ex)
        {
            better_sol_ex = false;
            continue;
        }

        fb_size = calc_fb_size;

        /**
         * A better feedback arc set than the best local one has been found.
         */
        if (best_fb_size > fb_size && fb_size <= MAX_VIABLE_COUNT)
        {
            fprintf(stdout, "Buffer: %d, Calculated: %d\n", ring_buf->best_fb_size, fb_size);
            /**
             * The fb arc set was found, but the ring buffer already holds a better solution.
             */
            if (fb_size > ring_buf->best_fb_size)
            {
                best_fb_size = ring_buf->best_fb_size;
                continue;
            }
            best_fb_size = fb_size;

            Fb_arc_set fb_arc_set;
            memset(&fb_arc_set, 0, sizeof(Fb_arc_set));

            for (int i = 0; i < best_fb_size; ++i)
            {
                fb_arc_set.edges[i].src = edge_set[i].src;
                fb_arc_set.edges[i].trgt = edge_set[i].trgt;
            }

            /**
		     * Used in the creation/debugging stage. Is now obsolete.
		     *
             * @code
             * fprintf(stdout, "Solution found:");
             * for (int i = 0; i < best_fb_size; ++i)
             * {
             *  fprintf(stdout, " %d->%d", fb_arc_set.edges[i].src, fb_arc_set.edges[i].trgt);
             * }
             * fprintf(stdout, "\n");
             * @endcode
             */

            fb_arc_set.written = true;
            fb_arc_set.num_e = best_fb_size;

            wait_sem(excl_sem); /**< wait for/block other generators by decrementing the exclusion semaphore */

            /**
             * Decrement the free space semaphore.
             * If buffer is full, block the write operation until buffer has space.
             */
            wait_sem(free_sem);

            /**
             * Find a position in the ring buffer which isn't written to.
             */
            while (ring_buf->sets[write_at].written == true)
            {
                write_at = (write_at + 1) % BUF_SIZE;
            }

            ring_buf->sets[write_at] = fb_arc_set;

            post_sem(used_sem); /**< buffer now holds one more element, increment the used space semaphore */

            write_at = (write_at + 1) % BUF_SIZE;

            post_sem(excl_sem); /**< unblock other generators wanting to write */

            /**
             *  Acyclic graph found.
             */
            if (fb_size == 0 && ring_buf->best_fb_size == 0)
            {
                signal_handler(SIGINT);
            }
        }
    }
    graph_destroy(g);
    exit(EXIT_SUCCESS);
}
