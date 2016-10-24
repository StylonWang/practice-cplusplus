#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

class CompressUtil {
    public:
    static int Zerodecompress(unsigned char const*, unsigned long, unsigned char*, unsigned long&);
    static int Zerocompress(unsigned char const*, unsigned long, unsigned char*, unsigned long&);
};

int main(int argc, char **argv)
{
    unsigned char samples[] = "abcdefghiZ&4";
    unsigned char *a, *b, *c;
    srandom(time(NULL));
    int ret;
    
    // randomly generate data to compress/decompress
    unsigned long data_size = 15 + random() % 128;
    unsigned long compress_size, decompress_size;

    compress_size = data_size*2;
    decompress_size = data_size *2;

    a = (unsigned char *)malloc(data_size);
    b = (unsigned char *)malloc(compress_size);
    c = (unsigned char *)malloc(decompress_size);

    for(unsigned long i=0; i+2<data_size; i+=2) {
        a[i] = a[i+1] = a[i+2] = samples[ random()%sizeof(samples) ];
    }

    fprintf(stderr, "Test data to be compressed, %ld bytes: \n", data_size);
    for(unsigned long i=0; i<data_size; ++i) {
        if(0==i%16) fprintf(stderr, "\n");
        fprintf(stderr, "%02x ", a[i]);
    }
    fprintf(stderr, "\n\n");

    ret = CompressUtil::Zerocompress(a, data_size, b, compress_size);
    fprintf(stderr, "compress from %ld to %ld, ret=%d\n",
                data_size, compress_size, ret);
    for(unsigned long i=0; i<compress_size; ++i) {
        if(0==i%16) fprintf(stderr, "\n");
        fprintf(stderr, "%02x ", b[i]);
    }
    fprintf(stderr, "\n\n");

    ret = CompressUtil::Zerocompress(b, compress_size, c, decompress_size);
    fprintf(stderr, "decompress from %ld to %ld, ret=%d\n",
                compress_size, decompress_size, ret);
    for(unsigned long i=0; i<decompress_size; ++i) {
        if(0==i%16) fprintf(stderr, "\n");
        fprintf(stderr, "%02x ", c[i]);
    }
    fprintf(stderr, "\n\n");

    if(decompress_size != data_size) {
        fprintf(stderr, "size mismatch %ld %ld\n", decompress_size, data_size);
        exit(1);
    }

    if(memcmp(a, c, data_size)) {
        fprintf(stderr, "compression error\n");
        exit(1);
    }


    return 0;
}

