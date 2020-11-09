obj = test.o
bin = test

CFLAGS = -pedantic -Wall -g -I../../src
LDFLAGS = ../../libspnavdev.a

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(obj) $(bin)
