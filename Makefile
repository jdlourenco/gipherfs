EXECUTABLE=gipherfs
FUSE_VERSION=2.9.7
CUDA_LIBS=-L/opt/cuda/lib64 -lcuda -lcudart -lstdc++
CUDA_HEADERS=-I/usr/local/cuda/samples/common/inc/
HEADERS=-I/home/jdpl/fuse_latest/fuse-$(FUSE_VERSION)/include/ -Iinclude
LIBS=-L/home/jdpl/fuse_latest/fuse-$(FUSE_VERSION)/lib/.libs -Llib/.libs -lfuse -lrt -ldl -lm
GCC_OPTIONS=-Wall -std=c99
FUSE_OPTIONS=-D_FILE_OFFSET_BITS=64

gipherfs: aes_wrapper_gpu.o fs_util.o gipherfs.o
	g++ -g $(GCC_OPTIONS) $(FUSE_OPTIONS) $(HEADERS) $(LIBS) $(CUDA_LIBS) -pthread  $^ -o $@

timed_gipherfs: timed_aes_wrapper_gpu.o fs_util.o gipherfs.o
	g++ -g $(GCC_OPTIONS) $(FUSE_OPTIONS) $(HEADERS) $(LIBS) $(CUDA_LIBS) -pthread $^ -o $@

gipherfs.o: gipherfs.c
	gcc -g -Wall -std=gnu99 $(FUSE_OPTIONS) $(HEADERS) $(LIBS) $(CUDA_LIBS) -pthread -c $^

fs_util.o: ../fs_utils/fs_util.c
	gcc -g -Wall -c $^

aes_wrapper_gpu.o:	aes_wrapper_gpu.cu
	nvcc -g -G -c -lcuda $^ $(CUDA_HEADERS)

timed_aes_wrapper_gpu.o:	timed_aes_wrapper_gpu.cu
	nvcc -g -G -c -lcuda $^ $(CUDA_HEADERS)

clean:
	rm -f *.o $(EXECUTABLE)
