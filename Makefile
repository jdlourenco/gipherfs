EXECUTABLE=gipherfs
FUSE_VERSION=2.9.7
CUDA_LIBS=-L/opt/cuda/lib64 -lcuda -lcudart -lstdc++
FUSE_DIR=/home/jdpl/fuse_latest/fake/fuse-$(FUSE_VERSION)
HEADERS=-I$(FUSE_DIR)/include/ -Iinclude
LIBS=-L$(FUSE_DIR)/lib/.libs -Llib/.libs -lfuse -lrt -ldl -lm
GCC_OPTIONS=-Wall -std=c99
FUSE_OPTIONS=-D_FILE_OFFSET_BITS=64

gipherfs: aes_wrapper_gpu.o fs_util.o gipherfs.o
	g++ -g $(GCC_OPTIONS) $(FUSE_OPTIONS) $(HEADERS) $(LIBS) $(CUDA_LIBS) -pthread  $^ -o $@

timed_gipherfs: timed_aes_wrapper_gpu.o fs_util.o gipherfs.o
	g++ -g $(GCC_OPTIONS) $(FUSE_OPTIONS) $(HEADERS) $(LIBS) $(CUDA_LIBS) -pthread $^ -o $@

gipherfs.o: gipherfs.c
	gcc -g -Wall -std=gnu99 $(FUSE_OPTIONS) $(HEADERS) $(LIBS) $(CUDA_LIBS) -pthread -c $^

fs_util.o: fs_utils/fs_util.c
	gcc -g -Wall -c $^

aes_wrapper_gpu.o:	aes_wrapper_gpu.cu
	nvcc -g -G -c -lcuda $^

timed_aes_wrapper_gpu.o:	timed_aes_wrapper_gpu.cu
	nvcc -g -G -c -lcuda $^

clean:
	rm -f *.o $(EXECUTABLE)
