all: oss user_proc

oss: oss.c clock.c frame.c memory.c process_table.c
	gcc -Wall -g -o oss oss.c clock.c frame.c memory.c process_table.c -lrt

user_proc: user_proc.c
	gcc -Wall -g -o user_proc user_proc.c -lrt

clean:
	rm -f *.o oss user_proc *.txt logfile
