/**
 * @file sendInfo.c
 * @brief Simple utility used to push scripted commands through the FIFO.
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_LENGTH 200

static void sleep_for_micros(long micros)
{
    if (micros <= 0)
    {
        return;
    }

    struct timespec ts;
    ts.tv_sec = micros / 1000000;
    ts.tv_nsec = (micros % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

int main(void)
{
    const char *commands[] = {
        "ARRIVAL TP437 init: 8 eta: 16 fuel: 18\n",
        "ARRIVAL TP437 init: 8 eta: 20 fuel: 22\n",
        "ARRIVAL TP437 init: 8 eta: 26 fuel: 29\n",
        "ARRIVAL TP437 init: 14 eta: 20 fuel: 100\n",
        "ARRIVAL TP437 init: 18 eta: 8 fuel: 40\n"
    };
    const size_t total_commands = sizeof(commands) / sizeof(commands[0]);

    int fd = open("input_pipe", O_WRONLY);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    for (size_t i = 0; i < total_commands; ++i)
    {
        char buffer[MAX_LENGTH];
        strncpy(buffer, commands[i], sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        const size_t len = strlen(buffer);
        if (write(fd, buffer, len) < 0)
        {
            perror("write");
            break;
        }

        sleep_for_micros(10000);
    }

    close(fd);
    return 0;
}
