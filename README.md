# AirportSimulator
Program that simulates the operation of an airport, using pipes, shared memory, mutexes and condition variables. It receives commands from a pipe, logs the events to a file, and shows statistics when signaled.

First, it is necessary to open two terminals, one where you make and run the result (./main) and another to send commands from the pipe (ex: echo ‘DEPARTURE TP40 init: 20 takeoff: 100’ -> input_pipe). After starting the program, it creates a log file (.txt), so that all the necessary information registered in the program can be saved. When receiving a signal of type SIGUSR1, the program displays the statistics to the user. When receiving a Ctrl+C, by the user, the program terminates and all resources used are released.

Whenever it is necessary to access data, mutexes were used to lock the functions and threads that may be accessing or changing the same data. When an event occurs and it is necessary to unlock some execution of something, condition variables were used. To notify the thread in the simulation manager that the SHM data has been changed, the condition variable was placed in shared memory. Time is a shared memory data
