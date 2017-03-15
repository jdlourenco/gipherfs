/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` hello.c -o hello
*/

#include "gipherfs.h"

#define FUSE_DEBUG "-d"
#define FUSE_SINGLETHREADED "-s"
#define FUSE_ARGC 2

#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

cache* file_cache;
char* file_read_intrumentation_path;
char* file_write_intrumentation_path;

int gipherfs_getattr(const char *path, struct stat *stbuf) {

  debug_print("FUSE: getattr of %s\n",path);

  int res;
  off_t file_size;

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] getattr: back_path=%s\n",back_path);
  res = lstat(back_path, stbuf);

  if(res) {
    perror("");
    return -errno;
  }

  if(!S_ISDIR(stbuf->st_mode)) {

    file_size = stbuf->st_size;
    debug_print("[FUSE] getattr: file_size=%jd\n",file_size);

    int pad = 0;

    if(file_size != 0) {
      int aux_fd = open(back_path, O_RDONLY);
      unsigned char* pad_block = (unsigned char*) malloc(sizeof(unsigned char) * AES_BLOCK_SIZE);

      pread(aux_fd, pad_block, AES_BLOCK_SIZE, file_size-AES_BLOCK_SIZE);

      aes_decryption_wrapper(pad_block, AES_BLOCK_SIZE);

      pad = (int)pad_block[AES_BLOCK_SIZE-1];
      debug_print("[FUSE] getattr: pad=%d\n", pad);

      free(pad_block);
      close(aux_fd);
    }

    stbuf->st_size-=pad;
  }

  free(back_path);

  return res;
}

int gipherfs_getxattr(const char *path, const char *name, char *value, size_t size) {

  debug_print("[FUSE] getxattr: path=%s name=%s value=%s size=%jd\n", path, name, value, size);

  int res;

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] getxattr: backpath=%s\n", back_path);  

  // fusexmp uses lgetxattr instead of getxattr
  // I don't know what is best but I'm using getxattr for now
  //res = lgetxattr(back_path, name, value, size);
  res = getxattr(back_path, name, value, size);
  free(back_path);
  if(res == -1) {
    perror("");
    return -errno;
  }

  return res;
}

/*
static int gipherfs_opendir(const char *path, struct fuse_file_info *fi) {

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] opendir: backpath=%s\n", back_path);

  if((dp = opendir(back_path)) == NULL) {
    perror("");
    return -errno;
  }



  free(back_path);

}
*/

int gipherfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

  debug_print("[FUSE] readdir: path=%s\n", path);
  
  DIR *dp;
  struct dirent *de;

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] readdir: backpath=%s\n", back_path);
  
  
  dp = opendir(back_path);

  if(dp==NULL) {
    perror("");
    return -errno;
  }

  while((de=readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    if(filler(buf, de->d_name, &st, 0)){
      perror("");
      debug_print("%s\n", "[FUSE] readdir: filler function returned an error, buf is full");
      break;
    }
  }

  free(back_path);
  closedir(dp);
  return 0;
}

int gipherfs_rmdir(const char *path) {
  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] rmdir: path=%s backpath=%s\n", path, back_path);
  
  // On  success,  zero is returned.  On error, -1 is returned, and errno is set appropriately.
  if(rmdir(back_path)) {
    debug_print("[FUSE] rmdir: failed back_path=%s\n", back_path);
    perror("");
    return -errno;
  }

  free(back_path);
  return 0;
}


//TODO: so vai funcionar para ficheiros sem hierarquia
int gipherfs_open(const char *path, struct fuse_file_info *fi) {
  int fd;
  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] open: path=%s back_path=%s\n", path, back_path);

  fd = open(back_path, fi->flags);
 
  if(fd==-1) {
    debug_print("[FUSE] open: failed fd=%d\n",fd);
    perror("");
    return -errno;
  }

  debug_print("[FUSE] open: sucessfull fd=%d\n",fd);
  fi->fh = fd;
  
  free(back_path);
  return 0;
}

int gipherfs_flush(const char *path, struct fuse_file_info *fi) {
  int close_ret, dupfd;
  
  (void) path;
  /* This is called from every close on an open file, so call the
     close on the underlying filesystem.	But since flush may be
     called multiple times for an open file, this must not really
     close the file.  This is important if used on a network
     filesystem like NFS which flush the data/metadata on close() */
  debug_print("FUSE: flush will dup fd=%jd\n",fi->fh);

  dupfd = dup(fi->fh);
  if(dupfd == -1) {
    debug_print("[FUSE] dup: failed fd=%d\n",dupfd);
    perror("");
    return -errno;
  }

  debug_print("FUSE: flush dupfd=%d\n",dupfd);
  close_ret = close(dupfd);
  if(close_ret == -1) {
    debug_print("[FUSE] flush: failed fd=%d\n",dupfd);
    perror("");
    return -errno;
  }

  return 0;
}

int gipherfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi) {
  (void) path;

  debug_print("[FUSE] fsynch: path=%s isdatasync=%d fd=%jd)\n", path, isdatasync, fi->fh);

  #ifndef HAVE_FDATASYNC
    (void) isdatasync;
  #else
    if(isdatasync)
      if(fdatasync(fi->fh) {
        perror("");
        return -errno;
      }
    else
  #endif

  if(fsync(fi->fh)) {
    perror("");
    return -errno;
  }

  return 0;
}

int* get_block_range(off_t offset, size_t size, int block_size) {
  int* block_range = (int*)malloc(sizeof(int) * 2);

  // first block
  block_range[0] = offset / block_size;

  // number of blocks
  block_range[1] = (int) ceil(size / (block_size * 1.0));

  return block_range;
}

void print_block_data(unsigned char *block_data, int block_data_length) {
  for(int block_data_i = 0; block_data_i < block_data_length; block_data_i++)
    printf("block_data[%d]=%c (int)%d\n", block_data_i, block_data[block_data_i], (int)block_data[block_data_i]);
}

/*
 * this will need to read a block at a time
 * determine each block physical address:
 * if it is not in cache
 *   store it in the cache as a single file (the file name will be the block address to)
 * if it is in cache
 *   read the cached file (corresponding to a block)
 * this may be too expensive, opening and closing a file for each block read but let's try it
 */
int gipherfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

  //char* block_data;
  unsigned char *pad_block, *block_data;
  size_t bytes_read, total_bytes_read;
  //int block, n_blocks, first_block, block_idx_start, block_idx_end, n_aes_blocks, aes_block_i, pad, block_length;
  int block, n_blocks, first_block, block_idx_start, block_idx_end, n_aes_blocks, pad, block_length;
  int* block_range;

  debug_print("[FUSE] read: fd=%jd size=%jd block_size=%d offset=%jd\n",fi->fh, size, file_cache->block_size, offset);

  off_t file_size = get_file_size(fi->fh);

  int last_file_block = floor(file_size / file_cache->block_size);
  int last_aes_block  = (file_size / AES_BLOCK_SIZE) - 1; // -1 because last is the pad block

  debug_print("[fuse] read: last_file_block=%d\n", last_file_block);
  debug_print("[fuse] read: last_aes_block=%d\n", last_aes_block);

  debug_print("[fuse] read: file_size=%jd\n", file_size);

  pad = 0;

  block_range = get_block_range(offset, size, file_cache->block_size);

  first_block = block_range[0];
  n_blocks    = block_range[1];

  if(file_size == 0) {
    return 0;
  }

  if(first_block+n_blocks >= last_file_block && file_size != 0) {
    debug_print("[fuse] read: reading pad block starting at %d\n", last_aes_block*AES_BLOCK_SIZE);
    pad_block = (unsigned char*)malloc(sizeof(unsigned char)*AES_BLOCK_SIZE);

    // this shouldn't be deciphered on the GPU because it is only one AES_BLOCK
    // it is not worth it but I am doing it now to test the CUDA code
    ssize_t pad_read = pread(fi->fh, pad_block, AES_BLOCK_SIZE, last_aes_block*AES_BLOCK_SIZE);
    debug_print("[fuse] read: read %jd pad bytes\n", pad_read);

    aes_decryption_wrapper(pad_block, pad_read);

    pad = (int) pad_block[AES_BLOCK_SIZE-1];

    if(!validate_pad_block(pad, pad_block))
      debug_print("%s\n", "[fuse] read: INVALID PAD BLOCK");

    free(pad_block);
    debug_print("[fuse] read: there are %d pad bytes in the last aes data block\n", pad);
  }

  debug_print("[FUSE] read: reading %d block(s) starting at %d\n", n_blocks, first_block);

  block_data = (unsigned char*) malloc(sizeof(unsigned char)*file_cache->block_size);

  for(block=first_block, total_bytes_read=0; block<first_block + n_blocks; block+=1) {
    debug_print("[FUSE] read: reading block %d\n", block);

    // here I'm not using the offset arg because I want to read the whole block
    if(((int) (bytes_read = pread(fi->fh, block_data, file_cache->block_size, block*file_cache->block_size))) < 0) {
      perror("");
      return -errno;
    }

    debug_print("[FUSE] tried to read %d actually read %jd\n", file_cache->block_size, bytes_read);

    /*
     * decipher data
     */
//    unsigned char* aes_block = (unsigned char*) malloc(sizeof(unsigned char) * AES_BLOCK_SIZE);

    n_aes_blocks = bytes_read / AES_BLOCK_SIZE;
    
    if(bytes_read % AES_BLOCK_SIZE != 0)
      debug_print("%s\n", "[fuse] read: THIS SHOULD NEVER HAPPEN");

    debug_print("[fuse] read: bytes_read=%jd\n", bytes_read);
    debug_print("n_aes_blocks=%d\n", n_aes_blocks);

    aes_decryption_wrapper(block_data, bytes_read);

    //if((last_aes_block - 1) == (block * (file_cache->block_size/AES_BLOCK_SIZE))+aes_block_i) {
    //#if(block == (first_block + n_blocks - 1)) {
    if(block == (first_block + n_blocks)) {
      debug_print("[fuse] read: removing pad=%d bytes at disk block=%d\n", pad, block);
      bytes_read-=pad;
    }

    //memcpy(block_data+(aes_block_i*AES_BLOCK_SIZE), aes_block, AES_BLOCK_SIZE-bytes_to_remove);

    // copy data to buf

    block_idx_start = 0;
    block_idx_end   = bytes_read;

    if(block == first_block) {//first block is special because I may want to skip some bytes at the beggining of the block
      block_idx_start = offset % file_cache->block_size;
      debug_print("[FUSE] read: first block only starting reading at %d\n", block_idx_start);
    }

    if(block == (first_block + n_blocks - 1)) { // last block is special because I may want to skip some bytes at the end of the block
      block_idx_end = (block*file_cache->block_size + bytes_read) % file_cache->block_size;

      if(block_idx_end == 0) block_idx_end = bytes_read;

      debug_print("[FUSE] read: last block, only reading until byte %d\n", block_idx_end);
    }

    block_length = block_idx_end - block_idx_start;

    memcpy(buf+total_bytes_read, block_data+block_idx_start, block_length);

    total_bytes_read += block_length;
  }

  debug_print("[FUSE] read: total_read=%jd\n",total_bytes_read);

  free(block_range);
  free(block_data);
  return total_bytes_read;
}//gipherfs_read

int gipherfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

  unsigned char *block_data;
  char *back_path;
  size_t bytes_written, total_bytes_written, bytes_read;
  //int block, n_blocks, first_block, block_idx_start, block_idx_end, aux_fd, pad, block_length, n_aes_blocks, last_block_bytes, aes_block_i, additional_bytes;
  int block, n_blocks, first_block, block_idx_start, block_idx_end, aux_fd, pad, block_length, n_aes_blocks, last_block_bytes, additional_bytes;
  int* block_range;

  off_t current_file_size = get_file_size(fi->fh);

  char* buffer = (char*) malloc(sizeof(char)*size);
  memcpy(buffer, buf, size);

  int last_file_block = floor((current_file_size != 0 ? (current_file_size - AES_BLOCK_SIZE) : current_file_size) / file_cache->block_size);

  debug_print("[FUSE] write: fd=%jd size=%jd block_size=%d offset=%jd flags=%d\n",fi->fh, size, file_cache->block_size, offset, fi->flags);

  debug_print("[FUSE] write: current_file_size=%jd\n", current_file_size);

  additional_bytes = 0;
  pad = 0;

  block_range = get_block_range(offset, size, file_cache->block_size);
  first_block = block_range[0];
  n_blocks    = block_range[1];

  debug_print("[FUSE] write: first_block=%d\n", first_block);
  debug_print("[FUSE] write: n_blocks=%d\n", n_blocks);

  debug_print("[FUSE] write: last_file_block=%d\n", last_file_block);

  if(first_block+n_blocks-1 >= last_file_block) {
    debug_print("%s\n", "[FUSE] write: there will be a new last block");

    last_block_bytes = size % AES_BLOCK_SIZE;
    pad = AES_BLOCK_SIZE - last_block_bytes;

    if(pad != 0)
      additional_bytes = pad;
    else
      additional_bytes = AES_BLOCK_SIZE;

    debug_print("[FUSE] write: last_block_bytes=%d\n", last_block_bytes);
    debug_print("[FUSE] write: I'll add %d bytes of the %d value\n", additional_bytes, pad);

    char* aux_buffer = (char*) malloc(sizeof(char)*size);
    memcpy(aux_buffer,buffer,size*sizeof(char));

    if((buffer = (char*)realloc(buffer, sizeof(char)*(size+additional_bytes))) == NULL) {
      perror("");
      return -errno;
    }

    memcpy(buffer,aux_buffer,sizeof(char)*size);
    memset(buffer+size, (char) pad, additional_bytes);

    free(aux_buffer);

    size+=additional_bytes;
  }

  debug_print("[FUSE] write: additional_bytes=%d\n", additional_bytes);
  debug_print("[FUSE] write: new size=%jd\n", size);

  block_range = get_block_range(offset, size, file_cache->block_size);

  bytes_read = 0; // initialized to zero allow max(bytes_read, block_idx_end) to always work correctly

  first_block = block_range[0];
  n_blocks    = block_range[1];

  debug_print("[FUSE] write: writing %d disk block(s) starting at %d\n", n_blocks, first_block);

  block_data = (unsigned char*) malloc(sizeof(unsigned char)*file_cache->block_size);

  for(block=first_block, total_bytes_written=0; block<first_block + n_blocks; block+=1) {
    debug_print("[FUSE] write: writing disk block %d\n", block);

    memset(block_data, 0, sizeof(unsigned char)*file_cache->block_size);

    block_idx_start = 0;
    block_idx_end   = file_cache->block_size;

    if(block == first_block) {   //first block is special because I may want to skip some bytes at the beggining of the block
      block_idx_start = offset % file_cache->block_size;
      debug_print("[FUSE] write: first block only start writing at %d\n", block_idx_start);
    }

    if(block == (first_block + n_blocks - 1)) { // last block is special because I may want to skip some bytes at the end of the block
      block_idx_end = (offset + size) % file_cache->block_size;

      if(block_idx_end == 0) block_idx_end = file_cache->block_size;

      debug_print("[FUSE] write: last block, only writing until byte %d \n", block_idx_end);
    }


    debug_print("[FUSE] current_file_size=%jd, (block*file_cache->block_size + block_idx_end)=%d\n", current_file_size, (block*file_cache->block_size + block_idx_end));
    if(block_idx_start!=0 || (block_idx_end!=file_cache->block_size && current_file_size > block*file_cache->block_size + block_idx_end))  {
      // Data must first be read from disk

      // Reopen file in read-only mode to make sure it can be read
      // maybe there's a way to test if file is in a readable mode before reopening it
      debug_print("[FUSE] write: reading block=%d from disk\n", block);
      back_path = get_back_path(file_cache, path);
      aux_fd = open(back_path, O_RDONLY);

      struct fuse_file_info* aux_fi = (struct fuse_file_info*)malloc(sizeof(struct fuse_file_info));
      memcpy(aux_fi, fi, sizeof(struct fuse_file_info));

      aux_fi->fh = aux_fd;

      char* aux_block_data;
      aux_block_data = (char*) malloc(sizeof(char)*file_cache->block_size);

      if(((int) (bytes_read = gipherfs_read(path, aux_block_data, file_cache->block_size, block*file_cache->block_size, aux_fi))) < 0) {
        perror("");
        return -errno;
      }

      memcpy(block_data, aux_block_data, file_cache->block_size);

      debug_print("[FUSE] write: read %jd bytes from existing block=%d\n", bytes_read, block);

      free(aux_fi);
      close(aux_fd);
      free(back_path);
      free(aux_block_data);
    }

    memcpy(block_data+block_idx_start, buffer+total_bytes_written, block_idx_end - block_idx_start);

    /*
     *
     * cipher data
     *
     */

    block_length = block_idx_end;

   // unsigned char* aes_block = (unsigned char*) malloc(sizeof(unsigned char) * AES_BLOCK_SIZE);

    n_aes_blocks     = block_length / AES_BLOCK_SIZE;
    last_block_bytes = block_length % AES_BLOCK_SIZE;

    debug_print("block_length=%d\n", block_length);
    debug_print("n_aes_blocks=%d\n", n_aes_blocks);

    aes_encryption_wrapper(block_data, block_length);

    if(((int) (bytes_written = pwrite(fi->fh, block_data, block_length, block*file_cache->block_size))) < 0) {
      perror("");
      return -errno;
    }

    debug_print("[FUSE] tried to write %d actually wrote %jd\n", block_length, bytes_written);

    total_bytes_written += (block_idx_end - block_idx_start);
    current_file_size += (block_idx_end - block_idx_start);
  }

  debug_print("[FUSE] write: total_writen=%jd\n",total_bytes_written);
  debug_print("[FUSE] write: will return total_writen=%jd\n",total_bytes_written-additional_bytes);
  
  free(block_data);
  free(block_range);
  free(buffer);

  gipherfs_flush(path, fi);

  return total_bytes_written - additional_bytes;
}//gipherfs_write

#if INSTR
static int gipherfs_read_instrumented(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  double ts_i, ts_f;
  int ret;

  FILE *fp;
  fp = fopen(file_read_intrumentation_path, "a");

  ts_i = get_current_timestamp();

  ret = gipherfs_read(path, buf, size, offset, fi);

  ts_f = get_current_timestamp();

  double delta_t = ts_f - ts_i;
  double bandwidth = size/delta_t;

  fprintf(fp, "%lf;%lf;%jd\n", bandwidth, delta_t, size);

  fflush(fp);
  fclose(fp);

  return ret;
}

static int gipherfs_write_instrumented(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  double ts_i, ts_f;
  int ret;

  FILE *fp;

  fp = fopen(file_write_intrumentation_path, "a");

  ts_i = get_current_timestamp();

  ret = gipherfs_write(path, buf, size, offset, fi);

  ts_f = get_current_timestamp();

  double delta_t = ts_f - ts_i;
  double bandwidth = size / delta_t;

  fprintf(fp, "%lf;%lf;%jd\n", bandwidth, delta_t, size);

  fflush(fp);
  fclose(fp);

  return ret;
}
#endif


int gipherfs_release(const char *path, struct fuse_file_info *fi) {
  (void) path;

  debug_print("[FUSE] release: path=%s fd=%jd\n", path, fi->fh);

  if(close(fi->fh)) {
    debug_print("%s\n", "FUSE close error");
    perror("");
    return -errno;
  }

  return 0;
}

/*
static int cache_mknod(const char *path, mode_t mode, dev_t dev) {

  debug_print("FUSE: mknod(%s,%d)\n",path,mode);

  int res;
  char* translated_path;

  if(cache_contains_entry(file_cache,(char *) path)) {
    debug_print("File in cache\n");
    translated_path  = (char*) malloc(sizeof(char) * (strlen(file_cache->cache_dir)+strlen(path)+1));
    strcpy(translated_path,file_cache->cache_dir);
  }
  else {
    debug_print("File not in cache\n");
    translated_path  = (char*) malloc(sizeof(char) * (strlen(file_cache->backing_dir)+strlen(path)+1));
    strcpy(translated_path,file_cache->backing_dir);
  }

  strcat(translated_path,path);

  debug_print("FUSE: will mknod %s\n",translated_path);  
  res = mknod(translated_path, mode, dev);

  if(res == -1) {
    perror("");
    return -errno;
  }
  
  return res;

  / On Linux this could just be 'mknod(path, mode, rdev)' but this
     is more portable 
  if (S_ISREG(mode)) {
    res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (res >= 0)
      res = close(res);
  } else if (S_ISFIFO(mode))
    res = mkfifo(path, mode);
  else
    res = mknod(path, mode, rdev);
  if (res == -1)
    return -errno;/
}
*/

int gipherfs_chmod(const char *path, mode_t mode) {

  debug_print("[FUSE] chmod path=%s mode=%d)\n", path, mode);

  int res;

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] chmod: back_path=%s\n",back_path);  

  res = chmod(back_path, mode);
  free(back_path);
  if(res == -1) {
    perror("");
    return -errno;
  }
  return res;
}

int gipherfs_chown(const char *path, uid_t uid, gid_t gid) {

  debug_print("[FUSE] chown: path=%s uid=%d gid=%d\n", path, uid, gid);

  int res;

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] chown: back_path=%s\n",back_path);  

  res = lchown(back_path, uid, gid);
  free(back_path);
  if (res == -1) {
    perror("");
    return -errno;
  }
  return res;
}

int gipherfs_utimens(const char *path, const struct timespec ts[2]) {
  
  debug_print("[FUSE] utimens: path=%s\n", path);

  int res;

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] utimens: back_path=%s\n",back_path);  

  struct timeval tv[2];

  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  res = utimes(back_path, tv);
  free(back_path);
  if(res == -1) {
    perror("");
    return -errno;
  }

  return res;
}

int gipherfs_truncate(const char *path, off_t size) {

  int res;

  debug_print("[FUSE] truncate: path=%s size=%jd)\n", path, size);

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] truncate: back_path=%s\n", back_path);

  res = truncate(back_path, size);
  free(back_path);
  if (res == -1) {
    perror("");
    return -errno;
  }

  return 0;
}

int gipherfs_mkdir(const char *path, mode_t mode) {

  debug_print("[FUSE] mkdir: path=%s mode=%d\n", path, mode);

  int res;

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] mkdir: back_path=%s\n", back_path);
  
  res = mkdir(back_path, mode);
  free(back_path);
  if(res) {
    perror("");
    return -errno;
  }

  return 0;
}

int gipherfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

  //for now will create both on back and on cache
  //maybe can only create on cache
  debug_print("[FUSE] create path=%s mode=%d\n",path,mode);

  int fd;

  char* back_path = get_back_path(file_cache, path);

  debug_print("[FUSE] create:  path=%s\n",back_path);
  fd = creat(back_path, mode);
  if(fd==-1) {
    debug_print("FUSE: error creating on %s fd=%d\n",back_path,fd);
    return -errno;
  }

  debug_print("[FUSE] create: path=%s back_path=%s fd=%d\n",path, back_path, fd);

  free(back_path);
  fi->fh = fd;

  return 0;
}


int gipherfs_unlink(const char *path) {
  int res;

  char* back_path = get_back_path(file_cache, path);

  debug_print("FUSE unlink %s\n",back_path);
  res = unlink(back_path);
  free(back_path);
  if(res) {
    perror("");
    return -errno;
  }
  
  return 0;
}

int gipherfs_access(const char *path, int mask) {

  int res;

  char* back_path = get_back_path(file_cache, path);
  
  debug_print("[FUSE] access: back_path=%s\n", back_path);

  res = access(back_path, mask);

  free(back_path);

  if(res) {
    perror("");
    return -errno;
  }

  return 0;
}

static int gipherfs_rename(const char *oldpath, const char *newpath) { 

  int res;

  char* back_oldpath = get_back_path(file_cache, oldpath);
  char* back_newpath = get_back_path(file_cache, newpath);
  
  debug_print("[FUSE] rename: oldpath=%s, newpath=%s\n", back_oldpath, back_newpath);

  res = rename(back_oldpath, back_newpath);

  free(back_oldpath);
  free(back_newpath);

  return res;
}

static struct fuse_operations gipherfs_oper = {
  .getattr  = gipherfs_getattr,
  .getxattr = gipherfs_getxattr,
  .readdir  = gipherfs_readdir,
  .rmdir    = gipherfs_rmdir,
  //.mknod	= cache_mknod,
  .open     = gipherfs_open,
#if INSTR
  .read     = gipherfs_read_instrumented,
  .write    = gipherfs_write_instrumented,
#else
  .read     = gipherfs_read,
  .write    = gipherfs_write,
#endif
  .chmod    = gipherfs_chmod,
  .chown    = gipherfs_chown,
  .utimens  = gipherfs_utimens,
  .truncate = gipherfs_truncate,
  .mkdir    = gipherfs_mkdir,
  .unlink   = gipherfs_unlink,
  .flush    = gipherfs_flush,
  .fsync    = gipherfs_fsync,
  .access   = gipherfs_access,
  .create   = gipherfs_create,
  .rename   = gipherfs_rename,
  .release  = gipherfs_release
};

int main(int argc, char *argv[]) {

  //int i,j;
  if(INSTR) {
    char *pwd = get_current_dir_name();
    file_read_intrumentation_path  = get_instrumentation_path("read", pwd);
    file_write_intrumentation_path = get_instrumentation_path("write", pwd);
    free(pwd);

    debug_print("file_read_intrumentation_path=%s\n", file_read_intrumentation_path);
    debug_print("file_write_intrumentation_path=%s\n", file_write_intrumentation_path);
  }


  file_cache = (cache*) malloc(sizeof(cache));
  // char* config_path = "config/cache.yaml";
  char** fuse_argv;
  //if(init_cache(file_cache, config_path)) {
  if(init_cache(file_cache, BACK_DIR, MOUNT_DIR)) {
    debug_print("%s\n", "unable to init cache");
    return -1;
  }

  fuse_argv = generate_fuse_argv(file_cache, argv);

  // debug_print("cache_total_size %d\n", file_cache->cache_total_size);
  // debug_print("block_size %d\n", file_cache->block_size);
  debug_print("backing_dir %s\n", file_cache->backing_dir);
  // debug_print("cache_dir %s\n", file_cache->cache_dir);
  debug_print("mount_dir %s\n", file_cache->mount_dir);

  //debug_print("starting...\nusing %s\t as back store\nusing %s\t to store cached files\n",file_cache->backing_dir, file_cache->cache_dir);
  debug_print("starting...\nusing %s\t as back store\n",file_cache->backing_dir);
  debug_print("mounting to %s\t\n",fuse_argv[1]);

  // AES code
  aes_keyschedule();
  debug_print("%s\n", "init_aes_data_on_device");
  init_aes_data_on_device();
  debug_print("%s\n", "finished init_aes_data_on_device");


/*
  for(i = 0; i < 11; i++) {
    debug_print("RoundKey %i\t", i);

    for(j = 0; j < 16; j++) {
      debug_print("%02X ", SubKeys[(16 * i) + j]);
    }

    debug_print("%s", "\n");
  }*/

  return fuse_main(FUSE_ARGC + DEBUG + (BIG_WRITES*2), fuse_argv, &gipherfs_oper, &file_cache);
}

char** generate_fuse_argv(cache* file_cache, char** argv) {
  char** fuse_argv = (char**) malloc((FUSE_ARGC + DEBUG + (BIG_WRITES*2))* sizeof(char*));

  int i = 0;

  fuse_argv[i++] = argv[0];
  fuse_argv[i++] = file_cache->mount_dir;

  if(DEBUG)
    fuse_argv[i++] = FUSE_DEBUG;

  if(BIG_WRITES) {
    fuse_argv[i++] = "-o";
    fuse_argv[i++] = "big_writes";
  }
  //fuse_argv[3] = FUSE_SINGLETHREADED;//file_cache ? FUSE_DEBUG : "";

  return fuse_argv;
}

/*
void fuse_teardown(struct fuse* fuse, char * mountpoint) {
  debug_print("tearing fuse down\n");
}
*/

