
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pcap/pcap.h>

static long g_count = 0;

void capture_callback(u_char *useless, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    g_count++;
    fprintf(stderr, "%ld ", g_count);
}

void signal_handler(int signo)
{
    fprintf(stdout, "%ld packets captured\n", g_count);
    exit(1);
}

int main(int argc, char **argv)
{
    char *file, errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;

    if(argc<2) {
        fprintf(stderr, "Usage: %s file.pcap\n\n", argv[0]);
        return 1;
    }

    file = argv[1];

    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);

    printf("Use offline file: %s\n", file);

    handle = pcap_open_offline(file, errbuf);
    if(NULL == handle) {
        fprintf(stderr, "pcap_open_live: %s\n", errbuf);
        return 1;
    }

    // sniff until error occurs
    pcap_loop(handle, -1, capture_callback, NULL);

    return 0;
}

