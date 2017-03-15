A GPU-accelerated ciphered file system


Running gipherfs:

This guide assumes no root permissions on the target machine

Download and compile FUSE 2.9.7 from https://github.com/libfuse/libfuse

`
$ cd ~
$ wget https://github.com/libfuse/libfuse/releases/download/fuse-2.9.7/fuse-2.9.7.tar.gz
$ tar xvf fuse-2.9.7.tar.gz
$ cd fuse-2.9.7
$ ./configure
$ make -j8
$ pwd
/home/jdpl/fuse-2.9.7
`

Take note of the path returned by calling `pwd` and use it to edit the project Makefile and set FUSE_DIR by replacing INSERT_FUSE_LIB_PATH_HERE
In this example it' would be:

FUSE_DIR=/home/jdpl/fuse-2.9.7/lib/.libs


`
cd ~
cd gipherfs

Edit Makefile by setting and set FUSE_DIR
