/**
 * @file fb_arc_set.h
 * @author Aleksandar Hadzhiyski <e1426981@student.tuwien.ac.at>
 * @date 21.11.2020
 *  
 * @brief Feedback arc set header module.
 * 
 * @details The feedback arc set header module provides the necessary libraries and function
 * declarations used by the generator and supervisor programs.
 */

#ifndef FB_ARC_SET_H__ /* prevent multiple inclusion */
#define FB_ARC_SET_H__
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <regex.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "structs.h"

/** 
 * ---------------------------------------------------------------------------------
 *                              Helper function declarations
 * --------------------------------------------------------------------------------- 
*/

/**
 * Mandatory usage function.
 * @brief This functions prints an error message to stderr after an incorrect call to 
 * one of the two programs: supervisor or generator. 
 * @details The function delivers an error message indicating the correct way
 * to call the program.
 * @param prog The name of the program which is called incorrectly.
 * @return none
 */
void usage(char *prog);

/**
 * 2-values comparison function.
 * @brief This function compares two values.
 * @param a Constant void pointer to first object.
 * @param b Constant void pointer to second object.
 * @return Returns 0 if the objects are the same (value-wise)
 * and any other value if they are different.
 */
int cmpfunc(const void *a, const void *b);

/**
 * Integer value in array function.
 * @brief This function checks if a value is in an array.
 * @details The function doesn't check if the size passed is the actual size
 * of the array, meaning that both indexing out of bounds and not checking all elements
 * are possible scenarios.
 * @param val Value that must be found.
 * @param arr Array of elements to search in.
 * @param size Number of array first array elements to search from.
 * @return Returns 1 if the value is in the array, 0 otherwise.
 */
int val_in_arr(int val, int *arr, size_t size);

/**
 * Source from argument function.
 * @brief This function extracts the source vertex label from an argument.
 * @details The function assumes that the argument passed has a correct formatting.
 * @param arg Argument formatted as <source>-<target> from which to extract the source.
 * return Returns the source vertex label as an int.
 */
int src_from_arg(char *arg);

/**
 * Target from argument function.
 * @brief This function extracts the target vertex label from an argument.
 * @details The function assumes that the argument passed has a correct formatting.
 * @param arg Argument formatted as <source>-<target> from which to extract the target.
 * return Returns the target vertex label as an int.
 */
int trgt_from_arg(char *arg);

/**
 * Argument edge format function.
 * @brief This function asserts that the passed parameter has a correct formatting.
 * @details The function makes sure that the parameter has a format of the form:
 * <source>-<target> where source and target are integers.
 * @param arg String to be checked for correct formatting.
 * @return Returns 1 if the parameter is formatted correctly, 0 otherwise.
 */
int assert_edge_format(char *arg);

/**
 * Random number function.
 * @brief This function generates a random number within the lower-upper range.
 * @details The function doesn't check if the passed parameter lower and upper
 * are valid.
 * @param lower Lower range for the possible random numbers.
 * @param upper Upper range for the possible random numbers.
 * @return Returns a random integer number.
 */
int generate_random(int lower, int upper);

/**
 * Shuffle vertex function.
 * @brief This function takes a vertex array and shuffles it randomly.
 * @details The function doesn't check if actual size of the array is the same as the
 * size parameter. That's why only array elements up to index size are shuffled.
 * The shuffling is done using the Monte Carlo randomized algorithm.
 * @param set Array of integers to be shuffled.
 * @param size Size of the array to be shuffled.
 * @return none
 */
void shuffle_vertex_set(int *set, size_t size);

/**
 * Print solution function.
 * @brief This function prints a feedback arc set solution.
 * @details The function takes an array of Edge structures and prints each Edge
 * from that array in the format <source>-<target> to stdout.
 * @param edge_set Edge structures array.
 * @param prog Name of program which calls the print solution function.
 * @param size Size of the Edge structures array.
 * @return none
 */
void print_solution(Edge edge_set[], char *prog, int size);

/** 
 * ---------------------------------------------------------------------------------
 *                             Graph_ptr function declarations
 * --------------------------------------------------------------------------------- 
 */

/**
 * Graph creation function.
 * @brief This function creates a Graph_ptr with n vertices.
 * @details The function allocates enough memory for a Graph_ptr object with n vertices
 * along all other information such as a successors list.
 * @param n Number of vertices in the graph.
 * @return Returns a pointer to a Graph_ptr struct.
 */
Graph_ptr graph_create(int n);

/**
 * Graph destruction function.
 * @brief This function destroys a Graph_ptr.
 * @details The function deallocates/frees up the memory which was occupied by a Graph_ptr.
 * @param Graph_ptr Pointer to a Graph_ptr struct.
 * @return none
 */
void graph_destroy(Graph_ptr);

/**
 * Add edge function.
 * @brief This function adds a directed edge between two vertices of an existing Graph_ptr.
 * @details The function asserts that it's possible to create an edge between
 * two vertices and adds a directed edge afterwards, updating the respective successors
 * list of the Graph_ptr.
 * @param Graph_ptr Pointer to a Graph_ptr struct.
 * @param source Source vertex label.
 * @param target Target vertex label.
 * @return none
 */
void graph_add_edge(Graph_ptr, int source, int target);

/**
 * Graph_ptr vertex count function.
 * @brief This function gives the cardinality Graph_ptr.
 * @param Graph_ptr Pointer to a Graph_ptr struct.
 * @return Returns the number of vertices in a Graph_ptr.
 */
int graph_vertex_count(Graph_ptr);

/**
 * Graph_ptr edge count function.
 * @brief This function gives the number of edges in a Graph_ptr.
 * @param Graph_ptr Pointer to a Graph_ptr struct.
 * @return Returns the number of edges in a Graph_ptr.
 */
int graph_edge_count(Graph_ptr);

/**
 * Vertex out degree function.
 * @brief This function gives the out degree of a source vertex in a Graph_ptr.
 * @param Graph_ptr Pointer to a Graph_ptr struct.
 * @param source Source vertex label.
 * @return Returns the out degree of a source vertex in the graph.
 */
int graph_out_degree(Graph_ptr, int source);

/**
 * Edge exists function.
 * @brief This function checks if a directed edge between two vertices exists.
 * @details The function assumes that both the source and target vertices actually
 * exist in the graph and checks blindly if there is a directed edge between the two.
 * @param Graph_ptr Pointer to a Graph_ptr struct.
 * @param source Source vertex label.
 * @param target Target vertex label.
 * @return Returns 1 if such a directed edge exists, 0 otherwise.
 */
int graph_has_edge(Graph_ptr, int source, int target);

/**
 * ---------------------------------------------------------------------------------
 *                             Semaphore function declarations
 * ---------------------------------------------------------------------------------
 *
 * Most of the functions are simply the standard semaphore functions
 * to which an appropriate level of error checking/handling was added.
 */

/**
 * Open semaphore function.
 * @brief This function opens a semaphore and performs error checking.
 * @details The function relies on the standard semaphore function sem_open(3).
 * Additionally, after calling sem_open(), the function handles any errors if such
 * were to occur.
 * @param sem_name Semaphore name.
 * @param sem_size Semaphore size.
 * @param mode Mode of the semaphore. With 1: supervisor semaphore; 
 * 0: generator semaphore.
 * @return Returns a pointer to a semaphore if successful, an error message otherwise.
 */
sem_t *open_sem(char *sem_name, size_t sem_size, int mode);

/**
 * Get semaphore value function.
 * @brief This function places a semaphore value into an int and performs error checking.
 * @details The function relies on the standard semaphore function sem_getvalue(3).
 * Additionally, after calling sem_getvalue(), the function handles any errors if such
 * were to occur.
 * @param sem Semaphore pointer.
 * @param res Integer pointer to a variable in which the semaphore value is placed.
 * @return none
 */
void getval_sem(sem_t *sem, int *res);

/**
 * Wait semaphore function.
 * @brief This function decrements a semaphore and performs error checking.
 * @details The function relies on the standard semaphore function sem_wait(3).
 * Additionally, after calling sem_wait(), the function handles any errors if such
 * were to occur.
 * @param sem Semaphore pointer.
 * @return none
 */
void wait_sem(sem_t *sem);

/**
 * Post semaphore function.
 * @brief This function increments a semaphore and performs error checking.
 * @details The function relies on the standard semaphore function sem_post(3).
 * Additionally, after calling sem_post(), the function handles any errors if such
 * were to occur.
 * @param sem Semaphore pointer.
 * @return none
 */
void post_sem(sem_t *sem);

/**
 * Close semaphore function.
 * @brief This function closes a sempahore and performs error checking.
 * @details The function relies on the standard semaphore function sem_close(3).
 * Additionally, after calling sem_close(), the function handles any errors if such
 * were to occur.
 * @param sem Semaphore pointer.
 * @return none
 */
void close_sem(sem_t *sem);

/** 
 * ---------------------------------------------------------------------------------
 *                            Shared memory function declarations
 * ---------------------------------------------------------------------------------
 * Most of the functions are simply the standard shared memory functions
 * to which an appropriate level of error checking/handling was added.
 */

/**
 * Create shared memory function.
 * @brief This function creates a shared memory object and performs error checking.
 * @details The function relies on the standard shared memory function shm_open(3).
 * Additionally, after calling shm_open(), the function handles any errors if such
 * were to occur.
 * @param shm_name Name of the shared memory.
 * @return Returns an int shared memory file descriptor.
 */
int create_shm(char *shm_name);

/**
 * Truncate shared memory function.
 * @brief This function truncates a shared memory object and performs error checking.
 * @details The function relies on the standard function ftruncate(2).
 * Additionally, after calling ftruncate(), the function handles any errors if such
 * were to occur.
 * @param shm_fd Shared memory file descriptor.
 * @param shm_size Size which will be truncated.
 * @return none
 */
void truncate_shm(int shm_fd, size_t shm_size);

/**
 * Open shared memory function.
 * @brief This function maps a shared memory object and performs error checking.
 * @details The function relies on the standard shared memory function mmap(2).
 * Additionally, after calling mmap(), the function handles any errors if such
 * were to occur.
 * @param shm_fd Shared memory file descriptor.
 * @param shm_size Shared memory size to be truncated.
 * @return Returns a void pointer.
 */
void *open_shm(int shm_df, size_t shm_size);

/**
 * Write to shared memory function.
 * @brief This function writes to a shared memory object and performs error checking.
 * @details The function relies on the standard function memcpy().
 * Additionally, after calling memcpy(), the function handles any errors if such
 * were to occur.
 * @param shm Void pointer to a shared memory.
 * @param data Data to be written to the shared memory.
 * @param data_size Size of the data.
 * @return none.
 */
void write_to_shm(void *shm, char *data, size_t data_size);

/**
 * Unmap shared memory function.
 * @brief This function unmaps a shared memory object and performs error checking.
 * @details The function relies on the standard shared memory function munmap(2).
 * Additionally, after calling munmap(), the function handles any errors if such
 * were to occur.
 * @param shm Void pointer to a shared memory.
 * @param shm_size Shared memory size to be unmapped.
 * @return Returns a void pointer.
 */
void unmap_shm(void *shm, size_t shm_size);

#endif