#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include <vector>

#define MODULE_NAME "mainParent"

// TODO: protect fprintf with semaphore

#define dprintf(fmt, args...) \
    do { \
        fprintf(stdout, "[%s] " fmt, MODULE_NAME, ## args); \
    } while(0)

#define dprintf_simple(fmt, args...) \
    do { \
        fprintf(stdout, fmt, ## args); \
    } while(0)

pid_t runChildA(void)
{

    //dprintf("ChildA Process Created\n");
}

pid_t runChildB(void)
{

    //dprintf("ChildB Process Created\n");
}

void passToChildA(std::vector<char> &integers)
{

}

void passToChildB(std::vector<char> &integers)
{

}

void shutdownChilds(pid_t pidA, pid_t pidB)
{

}

void waitForChilds(pid_t pidA, pid_t pidB)
{

}

void generateIntegers(int n, std::vector<char> &integers)
{
    integers.clear(); // is this needed?

    dprintf("Generating %d random integers: ", n);
    for(int i=0; i<n; ++i) {
        char t = 1+random()%50;
        integers.push_back(t);
        dprintf_simple("%d ", t);
    }
    dprintf_simple("\n");
}

int main(int argc, char **argv)
{
    pid_t pidA, pidB;

    dprintf("Main Process Started\n");

    srandom(time(NULL));

    pidA = runChildA();
    pidB = runChildB();

    while(1) {
        int n;
        char buf[8];
        std::vector<char> integers;

        dprintf("Enter a positive integer or -1 to exit:     ");

        fgets(buf, sizeof(buf), stdin);
        n = atoi(buf);

        if(-1==n) break;   // got instruction to exit
        if(n<=0) continue; // we need positive integers

        generateIntegers(n, integers);

        passToChildA(integers);
        passToChildB(integers);
    }

    shutdownChilds(pidA, pidB);

    dprintf("Process Waits\n");
    waitForChilds(pidA, pidB);
    dprintf("Process Exits\n");

    return 0;
}

