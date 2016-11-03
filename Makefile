all: udooclient udooserver

udooclient: udooclient.c
	gcc -Wall udooclient.c -o udooclient

udooserver: udooserver.c
	gcc -Wall udooserver.c -o udooserver

clean:
	rm -rf udooclient udooserver
