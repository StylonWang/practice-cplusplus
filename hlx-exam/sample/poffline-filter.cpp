
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>

#include <pcap/pcap.h>

#include <set>
#include <string>

#include "../tcpiphdr.h"

static long g_count = 0;

std::set<std::string> g_src_ip;
std::set<std::string> g_dst_ip;

void capture_callback(u_char *useless, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    const struct sniff_ethernet *ethpkt = (const struct sniff_ethernet *)packet;
    const struct sniff_ip *ippkt = (const struct sniff_ip *)(packet+SIZE_ETHERNET);
    const struct sniff_tcp *tcppkt;
    const unsigned char *payload;
    int size_tcp;
    int size_ip;
    std::string ip_src, ip_dst;

    g_count++;

    if(pkthdr->len < SIZE_ETHERNET+20) { // IP header is at least 20 bytes
        fprintf(stderr, "mal-formed packet with size %d\n", pkthdr->len);
        return;
    }

    size_ip = IP_HL(ippkt)*4;
    tcppkt = (const struct sniff_tcp *)(packet+SIZE_ETHERNET+size_ip);

    size_tcp = TH_OFF(tcppkt)*4;
    payload = (unsigned char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);

    ip_src = inet_ntoa(ippkt->ip_src);
    ip_dst = inet_ntoa(ippkt->ip_dst);

    g_src_ip.insert( ip_src );
    g_dst_ip.insert( ip_dst );

    //fprintf(stdout, "IP %s -> %s\n", ip_src, ip_dst);
}

void report_capture_result(void)
{
    fprintf(stdout, "%ld packets captured\n", g_count);

    fprintf(stdout, "Src IP addresses:\n");
    for(std::set<std::string>::iterator i=g_src_ip.begin(); i!=g_src_ip.end(); ++i) {
        fprintf(stdout, "%s ", i->c_str());

    }
    fprintf(stdout, "\n");

    fprintf(stdout, "Dst IP addresses:\n");
    for(std::set<std::string>::iterator i=g_dst_ip.begin(); i!=g_dst_ip.end(); ++i) {
        fprintf(stdout, "%s ", i->c_str());

    }
    fprintf(stdout, "\n");

}

void signal_handler(int signo)
{
    report_capture_result();
    exit(1);
}

int main(int argc, char **argv)
{
    char *file, errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct bpf_program bpf;
    int ret;

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

    // set up capture filter. we need in-bound traffic only.
    
    ret = pcap_compile(handle, &bpf, "tcp port 18964 and dst host 10.116.164.201", 0, 0);
    if(-1==ret) {
        fprintf(stderr, "pcap_compile: %s\n", pcap_geterr(handle));
        return 1;
    }

    ret = pcap_setfilter(handle, &bpf);
    if(-1==ret) {
        fprintf(stderr, "pcap_setfilter: %s\n", pcap_geterr(handle));
        return 1;
    }

    // sniff until error occurs
    pcap_loop(handle, -1, capture_callback, NULL);

    report_capture_result();

    return 0;
}

