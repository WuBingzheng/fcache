CFLAGS  = -g -O2 -Wall

SOURCES = $(wildcard *.c)
OBJS    = $(patsubst %.c,%.o,$(SOURCES))

TARGET : $(OBJS)

clean :
	rm -f $(OBJS) depends

depends : $(SOURCES)
	$(CC) -MM *.c > depends

include depends
