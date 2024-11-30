CC = gcc
CFLAGS = -Wall -g
TARGETS = oss user

all: $(TARGETS)

oss: oss.o
	$(CC) $(CFLAGS) -o oss oss.o

user: user.o
	$(CC) $(CFLAGS) -o user user.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS) *.o
