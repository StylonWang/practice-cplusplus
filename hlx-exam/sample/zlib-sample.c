/*
 * Use zlib to compress and uncompress a randomly generated binary string.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <zlib.h>

int main(int argc, char **argv)
{
    unsigned char *a, *b, *c;
    int result;
    int i;

    srand(time(NULL));

    unsigned long data_size = random() % 1024;
    unsigned long compressed_buf_size = data_size*11/10 + 12;
    unsigned long uncompressed_data_size;

    // use first argument as data size, or data size is randomly chosen
    if(argc>1) {
        data_size = atoi(argv[1]);
    }

    fprintf(stderr, "Original data size %ld\n", data_size);

    // random binary data
    a = (unsigned char *)malloc(data_size);
    //for(i=0; i< data_size/2; ++i) { // only randomize first half of the buffer
    for(i=0; i< data_size; ++i) { // randomize all of the buffer
        a[i] = random(); 
    }
    

    // create compressed buffer
    b = (unsigned char *)malloc(compressed_buf_size);

    result = compress(b, &compressed_buf_size, a, data_size);

    if(Z_OK==result) {
        fprintf(stderr, "compress success\n");
        fprintf(stderr, "Compressed data size %ld\n", compressed_buf_size);
    }
    else if(Z_MEM_ERROR==result) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    else if(Z_BUF_ERROR==result) {
        fprintf(stderr, "output buffer not large enough!\n");
        exit(1);
    }


    // size divide by 4 to test if the follow buffer reallocation works
    uncompressed_data_size = data_size/4; 

retry_uncompress:
    fprintf(stderr, "uncompress dst buffer is %ld bytes\n", uncompressed_data_size);
    c = (unsigned char *)malloc(uncompressed_data_size);

    result = uncompress(c, &uncompressed_data_size, b, compressed_buf_size);

    if(Z_OK==result) {
        fprintf(stderr, "uncompress success\n");
        fprintf(stderr, "UnCompressed data size %ld\n", uncompressed_data_size);
    }
    else if(Z_MEM_ERROR==result) {
        fprintf(stderr, "uncompress: out of memory\n");
        exit(1);
    }
    else if(Z_BUF_ERROR==result) {
        fprintf(stderr, "uncompressed: output buffer not large enough!\n");
        free(c);
        uncompressed_data_size *= 2;
        fprintf(stderr, "try again\n");
        goto retry_uncompress;
    }

    result = memcmp(a, c, data_size);
    if(result) {
        fprintf(stderr, "compression not working as expected\n");
        exit(1);
    }
    else {
        fprintf(stderr, "compression/uncompression success!\n");
    }

    return 0;
}
