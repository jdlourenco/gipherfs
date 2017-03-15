#ifndef WORKING_AES_H_
#define WORKING_AES_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
//#include <iostream>
#include <stdlib.h>

#include <math.h>
#include <string.h>
#include <unistd.h>

//#include <helper_timer.h>
//#include <helper_cuda.h>

#define AES_BLOCK_SIZE 16

#ifdef __cplusplus
extern "C" {
#endif


void aes_decryption_wrapper(unsigned char * BufferData, ssize_t buffer_size);
void aes_encryption_wrapper(unsigned char * BufferData, ssize_t buffer_size);
int validate_pad_block(int pad, unsigned char* pad_block);
void aes_keyschedule();
void init_aes_data_on_device();
void free_aes_data_on_device();

#ifdef __cplusplus
}
#endif

int compute_pad_value(int block_length);
int get_pad_value(unsigned char* block_data);
int pad_block(unsigned char* aes_block, char* plain_block, int pad_value);


int file_exists(const char *filename);
void dump_to_file(unsigned char* data, const char* path, size_t size);
void printAsState(unsigned char byteArray[], int length);
#endif
