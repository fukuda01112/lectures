CC = gcc
MPICC = mpicc
CXX = gxx

CFLAGS = -Wall -g -fopenmp 
CFLAGS = -fopenmp -O3 
CXXFLAGS = -Wall -g -fopenmp

OBJS = matrix.o multiply_matrix.o
MAT_TEST_OBJS = mat_test.o matrix.o

.SUFFIXES: .o .c .cpp .cxx

.cpp.o:
	$(MPICC) $(CXXFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) -c $<

.cxx.o:
	$(CXX) $(CXXFLAGS) -c $<


.PHONY: all clean
#all:    multiply_matrix
all: multiply_matrix
#all: mul_matrix_from_file

clean:
	$(RM) *.o

test_matrix: test_matrix

mul_matrix_from_file: mul_matrix_from_file.c
#	$(CC) $(CFLAGS) -o $@ $^ $(BLASLIB)
	$(MPICC) $(CFLAGS) -o multiply_matrix $^

multiply_matrix: $(OBJS)
	$(MPICC) $(CFLAGS) -o multiply_matrix $(OBJS)
	
test_matrix: $(MAT_TEST_OBJS)
	$(CC) -o test_matrix $(MAT_TEST_OBJS)

%.o: %.c
	$(MPICC) $(CFLAGS) -c $^