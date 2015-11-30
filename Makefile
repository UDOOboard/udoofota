all: udooclient udooserver

udooclient: udooclient.c
	gcc udooclient.c -o udooclient

udooserver: udooserver.c
	gcc udooserver.c -o udooserver

clean:
	rm udooclient udooserver
