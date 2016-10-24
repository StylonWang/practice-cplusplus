
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
#include <vector>

#include "tcpiphdr.h"
#include "ftdhdr.h"

static long g_count = 0;

std::set<std::string> g_src_ip;
std::set<std::string> g_dst_ip;

class CompressUtil {
    public:
    static int Zerodecompress(unsigned char const*, unsigned long, unsigned char*, unsigned long&);
};

static unsigned char *z_inflate(const unsigned char *p, int len, unsigned long &out_len)
{
    unsigned char *c = NULL;

    out_len = len*10;
    c = (unsigned char *)malloc(out_len);
    CompressUtil::Zerodecompress(p, len, c, out_len);

    fprintf(stderr, "inflated %d to %ld\n", len, out_len);

    return c;
}


static void parse_ftd_packet(const struct timeval *ts, const unsigned char *buf, int len)
{
    const FTDHeader *ftd = (FTDHeader *)buf; 
    const FTDExtHeader *ftdext = (FTDExtHeader *) (buf+sizeof(*ftd));
    const FTDCHeader *ftdc;
    unsigned long ftdc_len = 0; // length of FTDC including header
    static unsigned long time_base = 0;

    if(0==time_base) {
        time_base = ts->tv_sec;
    }

    //TODO: check against 'len'

    fprintf(stdout, "%ld.%06ld %d ", ts->tv_sec - time_base, ts->tv_usec/1000, len);

    fprintf(stdout, "{%s ", (FTDTypeNone==ftd->FTDType)? "FTDTypeNone" :
                           (FTDTypeFTDC==ftd->FTDType)? "FTDTypeFTDC" :
                           (FTDTypeCompressed==ftd->FTDType)? "FTDTypeCompressed" : "Unknown");
    fprintf(stdout, "%d %d}, ", ftd->FTDExtLength, ntohs(ftd->FTDDataLength));

    if(ftd->FTDExtLength>0) {
        fprintf(stdout, "{E %d %d }, ", ftdext->FTDTag, ftdext->FTDTagLength);
    }
    else {
        fprintf(stdout, "{E }, ");
    }
    
    fprintf(stdout, "\n\t");

    if(FTDTypeFTDC==ftd->FTDType) {
        const unsigned char *p = (buf+ sizeof(*ftd) + ftd->FTDExtLength);
        ftdc_len = len - (sizeof(*ftd) + ftd->FTDExtLength);
        if(ftd->FTDExtLength) {
            p += ftdext->FTDTagLength;
            ftdc_len -= ftdext->FTDTagLength;
        }

        ftdc = (FTDCHeader *)p;
    }
    else if(FTDTypeCompressed==ftd->FTDType) {
        const unsigned char *p = (buf+ sizeof(*ftd) + ftd->FTDExtLength);
        if(ftd->FTDExtLength) {
            p += ftdext->FTDTagLength; 
        }
        // p points to compressed FTDC
        fprintf(stderr, "p-buf = %ld\n", p-buf);
        
        unsigned char *n; // n points to uncompressed FTDC

        n = z_inflate(p, ntohs(ftd->FTDDataLength), ftdc_len);
        fprintf(stderr, "n=%p %ld\n", n, ftdc_len);
        if(NULL==n) {
            fprintf(stderr, "uncompress FTDC error\n");
            return;
        }

        ftdc = (FTDCHeader *)n;

        // above decompress is not working and cause following parsing to seg-fault.
        // skip parsing for now.
        ftdc = NULL;
    }
    else {
        ftdc = NULL;
    }

    // parse ftdc
    if(ftdc) {
        fprintf(stderr, "{FTDC %d %d %d - %d %d - %d %d}\n",
                ftdc->Version, ntohl(ftdc->TID), ftdc->Chain, 
                ntohs(ftdc->SequenceSeries), ntohl(ftdc->SequenceNumber),
                ntohs(ftdc->FieldCount), ntohs(ftdc->FTDCContentLength)
                );
        // TODO: check ftdc_len

        ftdc_len -= sizeof(*ftdc); //skip header, proceed to fields
        struct FTDField *f = (struct FTDField *) (((unsigned char *)ftdc) + sizeof(*ftdc));
        while(ftdc_len>0) {
            fprintf(stdout, "{F %d %d }, ", ntohl(f->FieldId), ntohs(f->FieldLength));

            // parse next field
            int flen = sizeof(f->FieldId) + sizeof(f->FieldLength) + f->FieldLength;
            ftdc_len -= flen;
            f = (struct FTDField *) (((unsigned char *)f) + flen);
        }

    }
                           
    fprintf(stdout, "\n");
}


static void capture_callback(u_char *useless, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    //const struct sniff_ethernet *ethpkt = (const struct sniff_ethernet *)packet;
    const struct sniff_ip *ippkt = (const struct sniff_ip *)(packet+SIZE_ETHERNET);
    const struct sniff_tcp *tcppkt;
    const unsigned char *payload;
    int size_tcp;
    int size_ip;
    int len;
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

    // calculate payload length
    len = pkthdr->len - (SIZE_ETHERNET + size_ip + size_tcp);
    parse_ftd_packet(&pkthdr->ts, payload, len); 
    //fprintf(stdout, "IP %s -> %s\n", ip_src, ip_dst);
}

static void report_capture_result(void)
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

static void signal_handler(int signo)
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

