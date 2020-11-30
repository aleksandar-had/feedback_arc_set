/**
 * @file structs.h
 * @author Aleksandar Hadzhiyski <e1426981@student.tuwien.ac.at>
 * @date 21.11.2020
 *  
 * @brief Structures header module.
 * 
 * @details The structures header module provides the necessary libraries and structure
 * declarations used by the generator and supervisor programs. It also defines the
 * names of the semaphores, ring buffer and the various sizes of some structures.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#define FREE_SEM "/1426981_free"
#define USED_SEM "/1426981_used"
#define EXCL_SEM "/1426981_excl"
#define RING_BUF "/1426981_ring"

#define BSEARCH_PROMPT_SIZE 15 /**< size of outdegree after which to use a binary search */
#define MAX_VIABLE_COUNT 8     /**< maximal size of feedback arc set to be considered */
#define BUF_SIZE 8             /**< the size of the shared memory ring buffer */

/** 
 * ---------------------------------------------------------------------------------
 *                              Structure declarations
 * --------------------------------------------------------------------------------- 
 */

/**
 * A structure to represent a directed graph.
 * Implementation based on suggestions from a Yale course in C involving directed graphs
 * computations. 
 */
typedef struct Graph_s
{
  /*@{*/
  int V; /**< the number of vertices */
  int E; /**< the number of edges    */
  /*@}*/

  /**
   *  A structure holding the successors list.
   */
  struct successors
  {
    /*@{*/
    int d;          /**< number of successors     */
    int len;        /**< array size               */
    bool is_sorted; /**< whether array is sorted  */
    int list[1];    /**< list os successor vert.  */
    /*@}*/
  } * alist[1];
} * Graph_ptr;

/**
 * A structure to represent a directed edge.
 */
typedef struct Edge_s
{
  /*@{*/
  int src;  /**< source vertex label  */
  int trgt; /**< target vertex label  */
  /*@}*/
} Edge;

/**
 *  A structure to represent a feedback arc set.
 */
typedef struct Fb_arc_set_s
{
  /*@{*/
  bool written;                 /**< has the feedback arc set already been written to the ring buffer */
  int num_e;                    /**< number of edges in the feedback arc set */
  Edge edges[MAX_VIABLE_COUNT]; /**< an array of Edge structs */
  /*@}*/
} Fb_arc_set;

/**
 *  A structure to represent a shared memory ring buffer.
 */
typedef struct Ring_buffer_s
{
  /*@{*/
  bool acyclic; /**< whether the feedback arc set has found an acyclic solution */
  volatile sig_atomic_t quit;
  int best_fb_size;          /**< the current best feedback arc set size written to the buffer */
  Fb_arc_set sets[BUF_SIZE]; /**< an array of feedback arc sets */
  /*@}*/
} Buffer;
