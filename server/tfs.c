#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void error(char *msg) {
  perror(msg);
  exit(1);
}
 
int main(int argc,char **argv)
{
    if(argc != 3){
        printf("Error: Input an IP and Port number as arguments\n");
        exit(EXIT_FAILURE);
    }
    char ip[10];
    int port;
    strcpy(ip, argv[1]);
    sscanf(argv[2], "%d", &port);
 
    char sendline[1024];
    char recvline[1024];
    int listen_fd, comm_fd;
    struct sockaddr_in servaddr;
 
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) error("ERROR opening socket");

    /* Eliminates "ERROR on binding: Address already in use" error. */
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
 
    bzero(&servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);
 
    if(bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) error("ERROR on binding");;
 
    if (listen(listen_fd, 10) < 0) error("ERROR on listen");

    int file_block_size = 0;
    int write_sz = 0;
    char* zip_file_name = "clientFile.zip";
    char* file_name = "z.txt";
    while(1)
    { 
        /* Accept connections */
        printf("\nReady to accept connections.\n");
        comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

        /* Create new zip file */
        FILE *zip_file = fopen(zip_file_name, "w");

        /* Receive file packets from client and write to the new zip file */
        bzero(recvline, 1024); 
        file_block_size = read(comm_fd, recvline, 1024);
        write_sz = 0;
        while(file_block_size > 0) 
        {
            printf("Received bytes: %d\n", file_block_size);
            
            write_sz = fwrite(recvline, sizeof(char), file_block_size, zip_file);
            if(write_sz < file_block_size)
            {
                fprintf(stderr, "File write failed on server.\n");
                break;
            }
            bzero(recvline, 1024);

            /* Break look if this is the final packet */
            if (file_block_size < 1024)
            {
                printf("File has been received.\n");
                break;
            }
            file_block_size = read(comm_fd, recvline, 1024);
        }
        fclose(zip_file);

        /* Unzip file */
        system("unzip -j 'clientFile'");

        /* Open unzipped file from client */
        printf("Sending file: %s\n", file_name);
        FILE *file = fopen(file_name, "r");
        if(file == NULL)
        {
            printf("Error: File %s not found.\n", file_name);
            exit(1);
        }

        /* Send unzipped file to client */
        bzero(sendline, 1024); 
        file_block_size = fread(sendline, sizeof(char), 1024, file);
        while(file_block_size > 0)
        {
            if(write(comm_fd, sendline, file_block_size) < 0)
            {
                error("Error sending file to client");
                // printf("Error: Failed to send file %s.\n", file_name);
                break;
            }
            printf("Sent bytes: %d\n", file_block_size);
            bzero(sendline, 1024);
            file_block_size = fread(sendline, sizeof(char), 1024, file);
        }
        printf("File send completed: %s \n", file_name);
        fclose(file);

        /* Close socket */
        if(close(comm_fd) < 0) error("Close socket error");
    }

}