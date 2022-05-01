CC = gcc
CFLAGS = -Wall -Wextra -Werror
DFLAGS = -g
DEPENDENCIES.C = read_ext2.c
EXEC = runscan
MAIN.C = runscan.c

default: main

clean:
	rm -f $(EXEC)
	rm -f runscan2
	rm -f runscan3

main: $(MAIN.C)
	$(CC) $(CFLAGS) $(DFLAGS) $(MAIN.C) $(DEPENDENCIES.C) -o $(EXEC)

runscan2: runscan2.c
	$(CC) $(CFLAGS) $(DFLAGS) runscan2.c $(DEPENDENCIES.C) -o runscan2

runscan3: runscan3.c
	$(CC) $(CFLAGS) $(DFLAGS) runscan3.c $(DEPENDENCIES.C) -o runscan3