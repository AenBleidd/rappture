CC = gcc
CFLAGS = -g

rptest: test.o rp_optimizer.o
	$(CC) $(CFLAGS) -o $@ test.o rp_optimizer.o -lm

test.o: rp_optimizer.h test.c
	$(CC) -c $(CFLAGS) test.c -o $@

rp_optimizer.o: rp_optimizer.h rp_optimizer.c
	$(CC) -c $(CFLAGS) rp_optimizer.c -o $@

clean:
	rm -f *.o rptest
