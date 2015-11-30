/*
 * udooserver.c - server for remoting host arduino ide
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

#define UDOONEO_SERVER_PORT	    5152
#define BUF_SIZE                256
#define NMAX_REMOTE_CMD         3
#define MAX_SIZE_REMOTE_CMD			32
#define VERSION	                "1.0"
#define ACK	                    "*"
#define NACK                    "?"
#define NAME_OF_BOARD	          "UDOONeo"
#define M4_FIRMWARE_RECEIVED_FILE	"M4_file.fw"
#define M4_UPLOADER             "udooneo_m4uploader"


enum {
	TEST = 1,
	FW_UPLOADER
}eRemoteCommandCode;

const char *tabRemoteCmd [NMAX_REMOTE_CMD] = {
	"NO_CMD",
	"TEST",
	"FW_UPLOADER"
};


int savefile(int connfd, const char * filename)
{
    /* Create file where data will be stored */
    FILE *fp = fopen(filename, "wb");
    if(NULL == fp)
    {
       printf("Error opening file %s\n", filename);
       return 1;
    }

    /* Receive data in chunks of BUF_SIZE bytes */
    int bytesReceived = 0;
    int totBytesReceived = 0;
    char buff[BUF_SIZE];
    memset(buff, '0', sizeof(buff));
    while((bytesReceived = read(connfd, buff, BUF_SIZE)) > 0)
    {
		//printf("FW uploader task: bytes received %d\n",bytesReceived);
        fwrite(buff, 1, bytesReceived, fp);
		if (bytesReceived > 0) totBytesReceived += bytesReceived;
    }

    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }

    fclose(fp);
    return (totBytesReceived);
}

int GetRemoteCommand (int connfd)
{
	char tBuffer[MAX_SIZE_REMOTE_CMD];
	char cmdBuffer[MAX_SIZE_REMOTE_CMD];
	int msgLen = 0;
	int bytesReceived = 0;
	int cmd = 0;

	cmdBuffer[0] = 0;
	do {
		bytesReceived = read(connfd, tBuffer, BUF_SIZE);
		//printf ("bytesReceived = %d\r\n", bytesReceived);
		if (bytesReceived > 0) strcat (cmdBuffer, tBuffer);
		if ((strchr(tBuffer,0x0D) != NULL) || (strchr(tBuffer,0x0A) != NULL)) break;
	}while (bytesReceived > 0);
	fsync (connfd);

	if (bytesReceived > 0)
    {
		int i;
		for (i=0; i<NMAX_REMOTE_CMD; i++) {
			if(strstr(cmdBuffer, tabRemoteCmd[i]) != NULL) {
				cmd = i;
				write(connfd,ACK,1);
				break;
			}
		}
    }
	else cmd = -1;

	return (cmd);
}

int upload (void)
{
	return (1);
}

int DoRemoteCommand (int connfd, int cmd)
{
	const char * filename = M4_FIRMWARE_RECEIVED_FILE;
	const char * uploader = M4_UPLOADER;
	int fsz;
	int retValue = 0;

	switch (cmd) {
	case FW_UPLOADER:
		printf("Uploader command received\n");
		fsz=savefile(connfd, filename);
		if (fsz > 0) {
			printf("Received fileSize = %d\n", fsz);
      
      char command[256];
      strcat(command, uploader);
      strcat(command," ");
      strcat(command, filename);
      
      int status = system(command);
			printf ("status = %d\n", status);
/*
		    printf("waiting for %s upload....\n", NAME_OF_BOARD);
			if (upload())
				write(connfd,ACK,1);
			else
				write(connfd,NACK,1);
*/
		}
		else {
			write(connfd,NACK,1);
			printf("Received file error!!\n");
			retValue = 1;
		}
		break;

	case TEST:
		printf("Test command received\n");
		break;

	default:
		retValue = 2;
		break;
	}
	
	return (retValue);
}

int main(int argc, char** argv)
{
	int serv_port = UDOONEO_SERVER_PORT;
	int pp;

    if (argc == 2)
    {
		if (strcmp(argv[1], "-h") == 0) {
			printf ("Usage: %s [server port]\n", argv[0]);
			exit (EXIT_SUCCESS);
		} else {
			if (strcmp(argv[1], "-v") == 0) {
				printf ("%s %s\n", argv[0], VERSION);
				exit (EXIT_SUCCESS);
			}
			else {
				pp = atoi(argv[1]);
				if (pp > 0) {
					serv_port = pp;
				}
				else {
					printf ("bad server port number !!\n");
					exit (EXIT_SUCCESS);
				}
			}
		}
    }
 
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if (listenfd < 0) {
		perror("Socket open");
		exit (EXIT_FAILURE);
	}

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(serv_port);

	int reuse = 1;
	if ( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1 )
	{
		perror("setsockopt");
		exit (EXIT_FAILURE);
	}

    if (bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0) {
		perror("bind");
		exit (EXIT_FAILURE);
	}

    if (listen(listenfd, 10) == -1)
    {
		perror("listen");
		exit (EXIT_FAILURE);
    }

    for (;;)
    {
        int connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL);
		printf("Connection accepted on %d port\n", serv_port);

		int rCmd;
		do {
			rCmd = GetRemoteCommand (connfd);
			if (rCmd > 0) {
				if (DoRemoteCommand(connfd, rCmd) == 0)
					printf ("Remote Command [%d] done\n", rCmd);
				else
					printf ("Remote Command [%d] failed\n", rCmd);
			}
		} while (rCmd >= 0);
		//int tt=savefile(connfd, filename);
		printf("Close connection on %d port\n", serv_port);
        close(connfd);
        sleep(1);
    }

	exit (EXIT_FAILURE);
}

