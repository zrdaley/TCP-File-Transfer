#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
 
int main(int argc,char **argv)
{
    if(argc != 4){
        printf("Error: Input an IP, Port, and Filename as arguments\n");
        exit(EXIT_FAILURE);
    }
    char filename[50];
    char ip[10];
    int port;
    
    strcpy(ip, argv[1]);
    sscanf(argv[2], "%d", &port);
    strcpy(filename, argv[3]);

    int sockfd,n;
    char sendline[1024];
    char recvline[1024];
    struct sockaddr_in servaddr;
 
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof servaddr);
 
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(port);
    inet_pton(AF_INET, ip, &(servaddr.sin_addr));
 
    connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    /* Create timer */
    printf("Starting timer.");
    clock_t start = clock(), diff;
 
    /* Send zip file to Server */
    printf("Sending file: %s\n", filename);
    FILE *zip_file = fopen(filename, "r");
    if(zip_file == NULL)
    {
        printf("Error: File %s not found.\n", filename);
        exit(1);
    }

    bzero(sendline, 1024); 
    int file_block_size; 
    int write_sz;
    while((file_block_size = fread(sendline, sizeof(char), 1024, zip_file))>0)
    {
        if(write(sockfd, sendline, file_block_size) < 0)
        {
            printf("Error: Failed to send file %s.\n", filename);
            break;
        }
        bzero(sendline, 1024);

        /* Break look if this is the final packet */
        if (file_block_size < 1024)
        {
            printf("File sent: %s \n", filename);
            break;
        }
    }

    /* Create new file */
    bzero(filename, 50); 
    strcpy(filename, "file.txt");
    FILE *file = fopen(filename, "w");

    /* Receive file packets from client and write to the new file */
    bzero(recvline, 1024); 
    file_block_size = read(sockfd, recvline, 1024);
    write_sz = 0;
    printf("Receiving file: %s\n", filename);

    while(file_block_size > 0) 
    {
        printf("Received bytes: %d\n", file_block_size);
            
        write_sz = fwrite(recvline, sizeof(char), file_block_size, file);
        if(write_sz < file_block_size)
        {
            fprintf(stderr, "File write failed on server.\n");
            break;
        }
        bzero(recvline, 1024);

        /* Break look if this is the final packet */
        if (file_block_size < 1024)
        {
            printf("File has been received!\n");
            break;
        }
        file_block_size = read(sockfd, recvline, 1024);
    }
    fclose(file);
    close(sockfd);

    diff = clock();
    printf("**Time taken: %d milliseconds**\n", (int)diff);
 
}