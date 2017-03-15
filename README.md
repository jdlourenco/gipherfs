A GPU-accelerated ciphered file system


Running gipherfs:

This guide assumes no root permissions and that CUDA 7.5 development kit is already installed on the target machine. 

Download and compile FUSE 2.9.7 from https://github.com/libfuse/libfuse

```shell
$ cd ~
$ wget https://github.com/libfuse/libfuse/releases/download/fuse-2.9.7/fuse-2.9.7.tar.gz
$ tar xvf fuse-2.9.7.tar.gz
$ cd fuse-2.9.7
$ ./configure
$ make -j8
$ pwd
/home/jdpl/fuse-2.9.7
```

Take note of the path returned by calling `pwd` in order to use it to edit the project Makefile.



```shell
cd ~
git clone https://github.com/jdlourenco/gipherfs.git
cd gipherfs
edit Makefile
```

Edit Makefile and set FUSE_DIR to the directory returned by the `pwd` command by replacing INSERT_FUSE_LIB_PATH_HERE.
In this example:

`
FUSE_DIR=/home/jdpl/fuse-2.9.7
`

gipherfs's uses the following defaults for its mount and backing directories

```c
#define MOUNT_DIR "/tmp/gipherfs"
#define BACK_DIR "/tmp/gipherfs_back"
```

These can be changed by editing `src/gipherfs.h`


Run the following command to compile the code:
```shell
make
```

And finally run it using:
```shell
./gipherfs
```

