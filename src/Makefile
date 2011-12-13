DEBUG =	-g -ggdb -DDEBUG
CC = gcc
CFLAGS =  -c -std=c99 -Wall $(DEBUG)	

LIB_OBJS =     \
	./index.o  \
	./log.o	   \
	./sst.o \
	./buffer.o    \
	./meta.o	\
	./util.o	\
	./skiplist.o

TEST_OBJS =\
	./test_skiplist.o \
	./test_index.o    \
	./test_sst.o	\
	./test_meta.o	\
	./test_buffer.o

TESTS = \
	./test-skiplist \
	./test-index    \
	./test-sst		\
	./test-meta		\
	./test-buffer

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

test-skiplist: ./test_skiplist.o  $(TEST_OBJS) $(LIB_OBJS)
	$(CC)	./test_skiplist.o $(SVR_OBJS) $(LIB_OBJS) -o $@

test-index: ./test_index.o $(TEST_OBJS) $(LIB_OBJS)
	$(CC) 	 ./test_index.o $(SVR_OBJS) $(LIB_OBJS) -o $@

test-buffer: ./test_buffer.o $(TEST_OBJS) $(LIB_OBJS)
	$(CC) 	 ./test_buffer.o $(SVR_OBJS) $(LIB_OBJS) -o $@

test-sst: ./test_sst.o $(TEST_OBJS) $(LIB_OBJS)
	$(CC) 	 ./test_sst.o $(SVR_OBJS) $(LIB_OBJS) -o $@

test-meta: ./test_meta.o $(TEST_OBJS) $(LIB_OBJS)
	$(CC) 	 ./test_meta.o $(SVR_OBJS) $(LIB_OBJS) -o $@
