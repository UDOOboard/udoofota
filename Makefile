all: udooclient udooserver

udooclient: udooclient.c
	$(CC) -Wall udooclient.c -o udooclient

udooserver: udooserver.c
	$(CC) -Wall udooserver.c -o udooserver

clean:
	rm -rf udooclient udooserver
