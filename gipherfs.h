#ifndef GIPHERFS_H_
#define GIPHERFS_H_

#define _GNU_SOURCE

#include <math.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define FUSE_USE_VERSION 30

#define _XOPEN_SOURCE 700

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <dirent.h>

#include "aes_wrapper_gpu.cuh"

//#include "../fs_utils/include/attr/xattr.h"
//#include "../fs_utils/fs_util.h"

#include "fs_utils/include/attr/xattr.h"
#include "fs_utils/fs_util.h"

/*
#define MOUNT_DIR "/tmp/gipherfs"
#define BACK_DIR "/tmp/gipherfs_back"
*/

#define MOUNT_DIR "/homelocal/tmp/gipherfs"
#define BACK_DIR "/homelocal/tmp/gipherfs_back"

//#define INSTR 1
#define INSTR 0
#define DEBUG 0
#define BIG_WRITES 1

#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

int gipherfs_getattr(const char *path, struct stat *stbuf);
int gipherfs_getxattr(const char *path, const char *name, char *value, size_t size);
int gipherfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int gipherfs_rmdir(const char *path);
int gipherfs_open(const char *path, struct fuse_file_info *fi);
int gipherfs_flush(const char *path, struct fuse_file_info *fi);
int gipherfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
int gipherfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int gipherfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int gipherfs_release(const char *path, struct fuse_file_info *fi);
int gipherfs_chmod(const char *path, mode_t mode);
int gipherfs_chown(const char *path, uid_t uid, gid_t gid);
int gipherfs_utimens(const char *path, const struct timespec ts[2]);
int gipherfs_truncate(const char *path, off_t size);
int gipherfs_mkdir(const char *path, mode_t mode);
int gipherfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int gipherfs_unlink(const char *path);
int gipherfs_access(const char *path, int mask);

char** generate_fuse_argv(cache* file_cache, char** argv);

#endif
