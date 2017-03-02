#ifndef FS_UTIL_H_
#define FS_UTIL_H_

#define BILLION  1E9
#define DATE_STR_LEN 14

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/fs.h>

#define PATH_DELIM "/"
#define CURRENT_DIR "."
#define PARENT_DIR ".."

#define INSTR_DIR "/instrumentation_res/"
//#define INSTR_DIR "/home/jdpl/cuda-tese/fuse/plainfs/instrumentation_res/"

#define MAX_BLOCK_STR_SIZE 10

#define _GNU_SOURCE

#ifndef O_DIRECTORY
#define O_DIRECTORY 0
#endif


// typedef struct cache_entry_t {
//   char* filename;
//   char* file_key;
//   struct cache_entry_t* next;
// } cache_entry;

typedef struct cache_t {
  // cache_entry* file_list;
  // int key_generator;
  // int cache_total_size;
  int block_size;
  short int debug;
  char* backing_dir;
  // char* cache_dir;
  char* mount_dir;
} cache;

int init_cache(cache* file_cache, char* mount_dir, char* back_dir);

int open_create_dir(char* dir_path);
size_t read_cache_block(int backing_fd, char* buffer, size_t size, off_t offset);
size_t write_cache_block(cache* file_cache, int backing_fd, const char* buffer, size_t size);

void insert_into_cache(cache* file_cache, char* filename);
void display_cache(cache* file_cache);
int cache_contains_entry(cache* file_cache, char* filename);
void delete_cache_entry(cache* file_cache, char* filename);

off_t get_file_size(int fd);

char* get_back_path(cache* file_cache, const char* path);

int write_cache_updates(cache* file_cache);
int copy_file_to_cache(char* file_path, cache file_cache);

char* translate_path(cache* file_cache, const char* path);
int load_cache_contents(cache* file_cache, char* path);

char* get_instrumentation_path(char *type, char *pwd);

// timing functions
double get_current_timestamp();
char* get_current_time();
#endif
