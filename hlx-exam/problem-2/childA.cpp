#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <vector>

#include "common.h"

#define MODULE_NAME "childA"

static sem_t *g_print_sem;

#define dprintf(fmt, args...) \
    do { \
        sem_wait(g_print_sem); \
        fprintf(stdout, "[%s] " fmt, MODULE_NAME, ## args); \
        sem_post(g_print_sem); \
    } while(0)

#define dprintf_lock() do { sem_wait(g_print_sem); } while(0)
#define dprintf_unlock() do { sem_post(g_print_sem); } while(0)

// MUST protect dprintf_simple with dprintf_lock/unlock by yourself
#define dprintf_simple(fmt, args...) \
    do { \
        fprintf(stdout, fmt, ## args); \
    } while(0)

void signal_handler(int signo)
{
    fprintf(stderr, "childA got signal %d, exit\n", signo);
    sem_close(g_print_sem);

    exit(1);
}

int main(int argc, char **argv)
{
    // initialize
    g_print_sem = sem_open(PRINT_SEM_KEY, 0);
    if(SEM_FAILED==g_print_sem) {
        fprintf(stderr, "cannot open semaphore: %s\n", strerror(errno));
        exit(1);
    }

    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, signal_handler);

    dprintf("Child process started\n");

    std::vector<char> integers;
    while(1) {
        int a;
        char buf[8];
        
        fgets(buf, sizeof(buf), stdin);
        a = atoi(buf);

        if(a>0) { // positive integer, push to vector
            integers.push_back(a);
        }
        else if(a==0)  { // "zero" end this batch of inputs, 
               // calculate average and print results

            long sum = 0;

            if(integers.empty()) continue; // guard against zero inputs

            dprintf_lock();
            dprintf_simple("[%s] Random Numbers Received: ", MODULE_NAME);
            
            for(unsigned int i=0; i<integers.size(); ++i) {
                dprintf_simple("%d ", integers[i]);
                sum += integers[i];
            }
            dprintf_simple("\n");
            dprintf_unlock();

            dprintf("Average: %f\n", ((double)sum)/integers.size() );
            integers.clear();
        }
        else if(a<0) { // negative value signal a termination
            break;
        }
    }

    dprintf("Child process exits\n");

    sem_close(g_print_sem);

    return 0;
}

