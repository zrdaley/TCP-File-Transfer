#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
 
int main(int argc,char **argv) {
    /* Read input */
    if(argc != 4) {
        printf("Error: Input an IP, Port, and Filename as arguments\n");
        exit(EXIT_FAILURE);
    }
    char* ip = argv[1];
    char* filename = argv[3];
    int port;
    sscanf(argv[2], "%d", &port);

    int sockfd,n;
    char sendline[1024];
    char recvline[1024];
    struct sockaddr_in servaddr;
 
    /* Initialize socket */
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd < 0) {
        printf("Error opening socket\n");
        exit(EXIT_FAILURE);
    }
    bzero(&servaddr,sizeof servaddr);
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(port);
    inet_pton(AF_INET, ip, &(servaddr.sin_addr));

    /* Check for zip file */
    FILE *zip_file = fopen(filename, "r");
    if(zip_file == NULL)
    {
        printf("Error: File %s not found.\n", filename);
        exit(1);
    }
 
    /* Connect to server */
    connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    /* Create timer */
    printf("Starting timer\n");
    clock_t start = clock(), diff;
 
    /* Start send with zip file name */
    printf("Sending file: %d // %s\n", (int)strlen(argv[3]), filename);
    if((write(sockfd, filename, strlen(argv[3]))) < 0) {
            printf("Error: Failed to send file %s.\n", filename);
            exit(EXIT_FAILURE);
    }   

    /* Clear send buffer, initialize variables */
    bzero(sendline, 1024); 
    int file_block_size = fread(sendline, sizeof(char), 1024, zip_file); 
    int write_sz;
   
    /* Send all file packets */
    while(file_block_size > 0) {
        /* Break if write to server fails */
        if(write(sockfd, sendline, file_block_size) < 0) {
            printf("Error: Failed to send file %s.\n", filename);
            exit(EXIT_FAILURE);
        }
        printf("Sent bytes: %d\n", file_block_size);

        /* Clear send buffer */
        bzero(sendline, 1024);

        /* Break look if this is the final packet */
        if (file_block_size < 1024) {
            printf("File successfully sent: %s \n", filename);
            break;
        }
        file_block_size = fread(sendline, sizeof(char), 1024, zip_file); 
    }
    fclose(zip_file);

    /* Receive file name */
    file_block_size = read(sockfd, recvline, 1024);
    filename = malloc(file_block_size);
    filename = recvline;
    printf("Receiving file: %s\n", filename);

    /* Create new file */
    FILE *file = fopen(filename, "w");

    /* Receive file */
    bzero(recvline, 1024); 
    file_block_size = read(sockfd, recvline, 1024);
    write_sz = 0;
    while(file_block_size > 0) { 
        printf("Received bytes: %d\n", file_block_size);

        /* Write to file */
        write_sz = fwrite(recvline, sizeof(char), file_block_size, file);
        if(write_sz < file_block_size) {
            fprintf(stderr, "File write failed on server.\n");
            exit(EXIT_FAILURE);
        }

        /* Clear recv buffer */
        bzero(recvline, 1024);

        /* Break look if this is the final packet */
        if (file_block_size < 1024) {
            printf("File has been received.\n");
            break;
        }
        file_block_size = read(sockfd, recvline, 1024);
    }
    fclose(file);
    close(sockfd);

    diff = clock();
    printf("**Time taken: %d milliseconds**\n", (int)diff);
 
}