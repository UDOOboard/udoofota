# UDOO Firmware Over the Air Uploader

## How to compile:
    make clean
    make all

For windows platform (only `udooclient.c`):
- use cygwin console for linux-like compiling
- add cygwin1.dll on the same folder where is located `udooclient.exe`

## How to test:
### On UDOONeo:
Open a terminal and run:

     ./udooserver

### On HOST Windows or Linux:
Open a terminal and run:

     ./udooclient <tcpaddress>:<tcpport> <M4 firmware file>

