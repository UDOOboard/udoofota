/*
 * udooclient.c - client for remoting host arduino ide
 *
 * Copyright (C) 2015-2016 Seco S.r.L. All Rights Reserved.
 * 
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//#define PORT 					5152
#define BUF_SIZE 				256
#define FW_UPLOADER_CMD 		"FW_UPLOADER\n"
#define GET_UPLOADER_STATUS_CMD	"GET_UPLOADER_STATUS\n"
#define VERSION					"1.1"			// GET_UPLOADER_STATUS
#define ACK						'*'
#define NACK					'?'
#define NAME_OF_BOARD			"UDOONeo"

#define RETURN_CODE_OK					0
#define RETURN_CODE_ARGUMENTS_ERROR		1
#define RETURN_CODE_M4STOP_FAILED		2
#define RETURN_CODE_M4START_FAILED		3

// sent to server after firmware file
const unsigned char strEOF[] = "********????????@@@@@@@@++++++++";

int client(const char* filename, const char* remoteSocket)
{

	unsigned char buff[BUF_SIZE]={0};

	unsigned char *zz =	strchr(remoteSocket,':');
	if (zz != NULL) *zz++ = 0;

    /* Create a socket first */
    int sockfd = 0;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("\n Error: Could not create socket \n");
        return 1;
    }

    /* Initialize sockaddr_in data structure */
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(zz));
    serv_addr.sin_addr.s_addr = inet_addr(remoteSocket);
    //serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Attempt a connection */
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

	/* sending FW_UPLOADER command to server */
    if (write(sockfd, FW_UPLOADER_CMD, sizeof(FW_UPLOADER_CMD)) < 0) {
        printf("Send command message to server: error");
        return 1;
	}

	/* waiting for server acknowlege */
	char ack=0;
    read(sockfd, &ack, 1);
	if (ack != ACK) return (1);

    /* Open the file that we wish to transfer */
    FILE *fp = fopen(filename,"rb");
    if(fp==NULL)
    {
        printf("File open error");
		close (sockfd);
        return 1;
    }
 
    /* Read data from file and send it */
	int sizeOf_FW = 0;
    for (;;)
    {
        /* First read file in chunks of BUF_SIZE bytes */
        int nread = fread(buff,1,BUF_SIZE,fp);
        //printf("Bytes read %d \n", nread);

        /* If read was success, send data. */
        if(nread > 0)
        {
            //printf("Sending \n");
            write(sockfd, buff, nread);
			sizeOf_FW += nread;
        }

        /*
         * There is something tricky going on with read ..
         * Either there was error, or we reached end of file.
         */
        if (nread < BUF_SIZE)
        {
            if (ferror(fp)) {
                printf("Error reading\n");
				return (1);
			}
            break;
        }
    }

    printf("%s %d bytes sent\n", NAME_OF_BOARD, sizeOf_FW);
	fclose (fp);

//#define WAIT_ACK
#ifdef WAIT_ACK	
	/* waiting for server acknowlege */
    printf("waiting for %s upload....\n", NAME_OF_BOARD);
	ack=0;
    read(sockfd, &ack, 1);
	if (ack == ACK)
	    printf("%s upload done\n", NAME_OF_BOARD);
	else
	    printf("%s upload error !!\n", NAME_OF_BOARD);
#endif


	// -------------------------------------------------------------------
	// Get uploader status from server
	// -------------------------------------------------------------------
	sleep(1);	// for read strEOF string with one read instruction from udooserver 
	strcpy(buff, strEOF);
	write(sockfd, buff, strlen(buff));

	/* sending GET_UPLOADER_STATUS command to server */
	if (write(sockfd, GET_UPLOADER_STATUS_CMD, sizeof(GET_UPLOADER_STATUS_CMD)) < 0) {
		printf("Send command message to server: error");
		return 1;
	}

	/* waiting for server acknowlege */
	ack=0;
	read(sockfd, &ack, 1);
	if (ack != ACK) return (1);

	// get uploader status
	char uploaderStatus = 0;
	read(sockfd, &uploaderStatus, 1);
	switch (uploaderStatus) {
	case RETURN_CODE_OK:
		printf("%s M4 Sketch is running\n", NAME_OF_BOARD);
		break;
	case RETURN_CODE_M4STOP_FAILED:
		printf("%s M4 Sketch STOP failed: reboot system !\n", NAME_OF_BOARD);
		break;
	case RETURN_CODE_M4START_FAILED:
		printf("%s M4 Sketch START failed: reboot system !\n", NAME_OF_BOARD);
		break;
	}
	// ----------------------------------------------------------------------

	close (sockfd);
    return 0;
}

int main(int argc, char** argv)
{

	if (argc == 3)
	{
		const char* remoteSocket = argv[1];
        const char* filename = argv[2];
        client(filename, remoteSocket);
	}
	else {
		if (argc == 2) {
			if (strcmp(argv[1], "-v") == 0) {
				printf ("%s %s\n", argv[0], VERSION);
				exit (EXIT_SUCCESS);
			}
		}

		printf ("Usage: %s <%s tcpaddress>:<%s tcpport> <%s M4 firmware file>\n", argv[0], NAME_OF_BOARD, NAME_OF_BOARD, NAME_OF_BOARD);
		exit (EXIT_SUCCESS);
	}
		
	exit (EXIT_SUCCESS);
}

