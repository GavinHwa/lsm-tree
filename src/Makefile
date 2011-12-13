DEBUG =	-g -ggdb -DDEBUG
CC = gcc
CFLAGS =  -c -std=c99 -Wall $(DEBUG)	

LIB_OBJS =     \
	./lsmtree/index.o  \
	./lsmtree/log.o	   \
	./lsmtree/sst.o \
	./lsmtree/buffer.o    \
	./lsmtree/meta.o	\
	./lsmtree/util.o	\
	./lsmtree/skiplist.o

TEST_OBJS =\
	./test/test_skiplist.o \
	./test/test_sst.o	\
	./test/test_meta.o	\
	./test/test_buffer.o \
	./bench.o 

TESTS = \
	./test-skiplist \
	./test-sst		\
	./test-meta		\
	./test-buffer	\
	./bench

LIBRARY = liblsmtree.a

all: $(LIBRARY)

clean:
	-rm -f $(LIBRARY)  
	-rm -f $(LIB_OBJS)
	-rm -f $(TEST_OBJS)
	-rm -f $(TESTS)

cleanall:
	-rm -f $(LIBRARY)  
	-rm -f $(LIB_OBJS)
	-rm -f $(TEST_OBJS)
	-rm -f $(TESTS)
	
	-rm -rf dbs

$(LIBRARY): $(LIB_OBJS)
	rm -f $@
	$(AR) -rs $@ $(LIB_OBJS)

bench: ./bench.o $(TEST_OBJS) $(LIB_OBJS)
	$(CC) 	 ./bench.o $(SVR_OBJS) $(LIB_OBJS) -o $@

test-skiplist: ./test/test_skiplist.o  $(TEST_OBJS) $(LIB_OBJS)
	$(CC)	./test/test_skiplist.o $(SVR_OBJS) $(LIB_OBJS) -o $@

test-buffer: ./test/test_buffer.o $(TEST_OBJS) $(LIB_OBJS)
	$(CC) 	 ./test/test_buffer.o $(SVR_OBJS) $(LIB_OBJS) -o $@

test-sst: ./test/test_sst.o $(TEST_OBJS) $(LIB_OBJS)
	$(CC) 	 ./test/test_sst.o $(SVR_OBJS) $(LIB_OBJS) -o $@

test-meta: ./test/test_meta.o $(TEST_OBJS) $(LIB_OBJS)
	$(CC) 	 ./test/test_meta.o $(SVR_OBJS) $(LIB_OBJS) -o $@
