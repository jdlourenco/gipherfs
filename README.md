A GPU-accelerated ciphered file system


Running gipherfs:

This guide assumes no root permissions on the target machine

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

Edit Makefile by setting FUSE_DIR to the directory returned by the `pwd` command by replacing by replacing INSERT_FUSE_LIB_PATH_HERE. In this case it would be

`
FUSE_DIR=/home/jdpl/fuse-2.9.7
`

Compile the code:
```shell
make
```

And finally run it:
```shell
./gipherfs
```

