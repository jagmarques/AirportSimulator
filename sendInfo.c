// C program to implement one side of FIFO 
// This side writes first, then reads 
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>

//No outro programa verificar a lenght e proteger
#define MAX_LENGHT 200
  
int main() 
{   
    int fd; 
    char arr[MAX_LENGHT];

    int x = 0;

    fd = open("input_pipe", O_WRONLY); 
    while (x < 1) 
    {   
        

        /*
        strcpy(arr, "DEPARTURE TP440 init: 12 takeoff: 20\n"); //Slot 0
        write(fd, arr, MAX_LENGHT+1);
        usleep(10000);
        strcpy(arr, "DEPARTURE TP442 init: 16 takeoff: 30\n"); //Slot 2
        write(fd, arr, MAX_LENGHT+1);
        usleep(10000);
        /*
        strcpy(arr, "ARRIVAL TP437 init: 14 eta: 20 fuel: 100\n");//Slot 1
        write(fd, arr, MAX_LENGHT+1);
        usleep(10000);
        strcpy(arr, "ARRIVAL TP437 init: 18 eta: 8 fuel: 10\n");//Slot 3
        write(fd, arr, MAX_LENGHT+1);
        */

       //PRIORITÁRIO
        
        strcpy(arr, "ARRIVAL TP437 init: 8 eta: 16 fuel: 18\n");//Slot 3
        write(fd, arr, MAX_LENGHT+1);
        usleep(10000);
        strcpy(arr, "ARRIVAL TP437 init: 8 eta: 20 fuel: 22\n");//Slot 3
        write(fd, arr, MAX_LENGHT+1);
        usleep(10000);
        strcpy(arr, "ARRIVAL TP437 init: 8 eta: 26 fuel: 29\n");//Slot 3
        write(fd, arr, MAX_LENGHT+1);
        usleep(10000);
        strcpy(arr, "ARRIVAL TP437 init: 14 eta: 20 fuel: 100\n");//Slot 1
        write(fd, arr, MAX_LENGHT+1);
        usleep(10000);
        strcpy(arr, "ARRIVAL TP437 init: 18 eta: 8 fuel: 40\n");//Slot 3
        write(fd, arr, MAX_LENGHT+1);
        
        

        x++;
    }

    close(fd); 

    return 0; 
} 
