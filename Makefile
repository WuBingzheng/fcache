CFLAGS  = -g -O2 -Wall
LDLIBS  = -lpthread

SOURCES = $(wildcard *.c)
OBJS    = $(patsubst %.c,%.o,$(SOURCES))

TARGET  = fcache

$(TARGET) : $(OBJS)
	make -C utils
	$(CC) -o $@ $^ utils/*.o $(LDLIBS)
	# install:
	mkdir -p output
	cp fcache output/
	test -f output/fcache.conf || cp fcache.conf.sample output/fcache.conf

clean :
	make -C utils clean
	rm -f $(OBJS) depends fcache

depends : $(SOURCES)
	$(CC) -MM *.c > depends

include depends
