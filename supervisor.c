/**
 * @file supervisor.c
 * @author Aleksandar Hadzhiyski <e1426981@student.tuwien.ac.at>
 * @date 21.11.2020
 *  
 * @brief Supervisor program module.
 * @details The supervisor module is responsible for the result presentation of the 
 * different generator computations. It makes sure that the generator results are saved
 * in the shared memory and only then overwritten when they have been printed/presented.
 * The supervisor runs as long as it is terminated externally or an acyclic result has
 * been found by a generator. In both cases, the supervisor indicates to the generators
 * that they must terminate as well via a shared atomic variable "quit" which both
 * the supervisor and all generators monitor constantly. The termination of the 
 * supervisor is preceeded by closing and unlinking all shared resources.
 * 
 * USAGE: The supervisor does not take any arguments.
 * supervisor
 */

#include "fb_arc_set.h"

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
 * After such a signal has been sent by the user, this function ensures that the
 * supervisor closes and unlinks the free space, used space and mutual exclusion 
 * semaphores which they opened and also unmap and destroy the shared memory ring buffer.
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
	ring_buf->quit = 1;
}

/**
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
 * @brief This is the main program of the supervisor module.
 * @details The program performs all of the supervisor functions as described in the
 * task requirements. Initially, it checks that the supervisor program has been
 * called correctly, afterwards it sets up all shared memory objects and semaphores.
 * The main tasks are performed in the program loop which monitors if the shared atomic
 * variable "quit" is set to true. The program makes sure that all of the feedback arc
 * sets written to the ring buffer are presented in the correct order and that no 
 * solution is overwritten before it has been presented.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS.
 */
int main(int argc, char *argv[])
{
	char *prog = argv[0];

	if (argc > 1)
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
	 * Shared memory objects definitions
	 */
	shm_buf_fd = create_shm(RING_BUF);
	truncate_shm(shm_buf_fd, BUF_SIZE);
	ring_buf = (Buffer *)open_shm(shm_buf_fd, sizeof(Buffer));

	/**
	 * Semaphore objects definitions
	 */
	excl_sem = open_sem(EXCL_SEM, 1, 0);
	used_sem = open_sem(USED_SEM, 0, 0);
	free_sem = open_sem(FREE_SEM, BUF_SIZE, 0);

	/**
	 * Definition of fb arc set array and free/used counters
	 */
	Fb_arc_set fb_arc_set;
	int free_space = 0;
	int used_space = 0;
	int read_from = 0;

	ring_buf->best_fb_size = INT16_MAX; /**< By default, consider the worst fb arc set possible */

	while (quit != 1)
	{
		/**
		 * Used in the creation/debugging stage. Is now obsolete.
		 */
		getval_sem(free_sem, &free_space);
		getval_sem(used_sem, &used_space);

		wait_sem(used_sem); /**< Block the used_sem. */

		/**
		 * Find a position in the ring buffer which is already written to.
		 */
		while (ring_buf->sets[read_from].written == false)
		{
			read_from = (read_from + 1) % BUF_SIZE;
		}

		/**
		 * Get the written fb arc set from the buffer and free it for overwriting.
		 */
		fb_arc_set = ring_buf->sets[read_from];
		ring_buf->sets[read_from].written = false;

		post_sem(free_sem); /**< Indicate that there is place for writing in the ring buffer. */

		read_from = (read_from + 1) % BUF_SIZE;

		/**
		 * Acyclic graph found.
		 */
		if (fb_arc_set.num_e == 0)
		{
			fprintf(stdout, "[%s] The graph is acyclic!\n", prog);
			ring_buf->acyclic = true;
			signal_handler(SIGTERM);
		}
		else if (fb_arc_set.num_e < ring_buf->best_fb_size)
		{
			ring_buf->best_fb_size = fb_arc_set.num_e;
			print_solution(fb_arc_set.edges, prog, fb_arc_set.num_e);
		}
	}
	exit(EXIT_SUCCESS);
}
