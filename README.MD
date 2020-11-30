Feedback Arc Set
Implement an algorithm which removes cycles in a directed graph by removing the least edges possible. A set of edges which must be removed to make a graph acyclic is also called a feedback arc set; and the set with the least edges is a minimal feedback arc set.

The problem of finding a minimal feedback arc set is NP-complete, therefore an exact solution becomes infeasible for moderately large graph sizes. However, a randomized algorithm can be used to find a good approximation, i.e. to find a feedback arc set which is close to minimal.

Supervisor
The supervisor sets up the shared memory and the semaphores and initializes the circular buffer required for the communication with the generators. It then waits for the generators to write solutions to the circular buffer.
The supervisor program takes no arguments.
Once initialization is complete, the supervisor reads the solutions from the circular buffer and remem- bers the best solution so far, i.e. the solution with the least edges. Every time a better solution than the previous best solution is found, the supervisor writes the new solution to standard output. If a generator writes a solution with 0 edges to the circular buffer, then the graph is acyclic and the supervisor termi- nates. Otherwise the supervisor keeps reading results from the circular buffer until it receives a SIGINT or a SIGTERM signal.
Before terminating, the supervisor notifies all generators that they should terminate as well. This can be done by setting a variable in the shared memory, which is checked by the generator processes before writing to the buffer. The supervisor then unlinks all shared resources and exits.

Generator
The generator program takes a graph as input. The program repeatedly generates a random solution to the problem as described on the first page and writes its result to the circular buffer. It repeats this procedure until it is notified by the supervisor to terminate.

The generator program takes as arguments the set of edges of the graph:
SYNOPSIS
generator EDGE1...
EXAMPLE
generator 0-1 1-2 1-3 1-4 2-4 3-6 4-3 4-5 6-0

Examples:
Invocation of the supervisor:
$ ./supervisor
[./supervisor] Solution with 2 edges: 1-2 2-0
[./supervisor] Solution with 1 edges: 2-0
Invocation of one generator:
$ ./generator 0-1 1-2 2-0
Invocation of 10 generators in parallel:
$ for i in {1..10}; do (./generator 0-1 1-2 2-0 &); done
