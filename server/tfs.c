#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>  

void error(char *msg) {
  perror(msg);
  exit(1);
}
 
int main(int argc,char **argv) {
    if(argc != 3) {
        printf("Error: Input an IP and Port number as arguments\n");
        exit(EXIT_FAILURE);
    }

    char* ip = argv[1];
    int port;
    sscanf(argv[2], "%d", &port);
 
    char sendline[1024];
    char recvline[1024];
    int listen_fd, comm_fd;
    struct sockaddr_in servaddr;
    
    /* Compile regex to remove file extensions */
    regex_t regex;
    int file_ext_match = regcomp(&regex, "^a[[:alnum:]]", 0);

    int file_block_size = 0;
    int write_sz = 0;
    int zip_name_length;
    int file_name_length;
    char* zip_file_name;
    char* file_name;
    char* unzip_command;
 
    /* Initialize socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        error("Error opening socket");
        exit(EXIT_FAILURE);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);

    /* Eliminates "ERROR on binding: Address already in use" error. */
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
    
    /* Bind */
    if(bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) error("ERROR on binding");
 
    /* Listen */
    if (listen(listen_fd, 10) < 0) error("ERROR on listen");
    
    while(1) { 

        /* Accept connections */
        printf("\nReady to accept connections.\n");
        comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

        /* Read filename */
        bzero(recvline, 1024); 
        file_block_size = read(comm_fd, recvline, 1024);
        zip_file_name = malloc(file_block_size);
        strcpy(zip_file_name, recvline);
        zip_name_length = file_block_size;
        printf("Receiving file: %s\n", zip_file_name);

        /* Create new zip file */
        FILE *zip_file = fopen(zip_file_name, "w");

        /* Receive file packets from client and write to the new zip file */
        bzero(recvline, 1024); 
        file_block_size = read(comm_fd, recvline, 1024);
        write_sz = 0;
        while(file_block_size > 0) {
            printf("Received bytes: %d\n", file_block_size);
            
            /* Exit if write to file fails */
            write_sz = fwrite(recvline, sizeof(char), file_block_size, zip_file);
            if(write_sz < file_block_size) {
                fprintf(stderr, "File to write failed.\n");
                exit(EXIT_FAILURE);
            }

            /* Clear buffer */
            bzero(recvline, 1024);

            /* Break look if this is the final packet */
            if (file_block_size < 1024) {
                printf("File has been received.\n");
                break;
            }
            file_block_size = read(comm_fd, recvline, 1024);
        }
        fclose(zip_file);

        /* Remove .zip extention from sent file name */
        file_name = malloc(zip_name_length);
        strncpy(file_name, zip_file_name, zip_name_length - 4);
        file_name_length = zip_name_length - 4;

        /* Unzip file */
        unzip_command = malloc(file_name_length + 9);
        strcpy(unzip_command, "unzip -j ");
        strcat(unzip_command, file_name);
        system(unzip_command);

        /* Add .txt extension to file name */
        strcat(file_name, ".txt");
        file_name_length = file_name_length + 4;

        /* Open unzipped file from client */
        FILE *file = fopen(file_name, "r");
        if(file == NULL) {
            printf("Error: File %s not found.\n", file_name);
            exit(EXIT_FAILURE);
        }

        /* Send .txt filename to client */
        printf("Sending file: %s\n", file_name);
        if(write(comm_fd, file_name, file_name_length) < 0) {
            error("Error sending file name to client");
            exit(EXIT_FAILURE);
        }

        /* Send unzipped file to client */
        bzero(sendline, 1024); 
        file_block_size = fread(sendline, sizeof(char), 1024, file);
        while(file_block_size > 0) {
            
            /* Exit if write to client fails */
            if(write(comm_fd, sendline, file_block_size) < 0) {
                error("Error sending file to client");
                exit(EXIT_FAILURE);
            }
            printf("Sent bytes: %d\n", file_block_size);
            
            /* Clear buffer */
            bzero(sendline, 1024);

            /* Break look if this is the final packet */
            if (file_block_size < 1024) {
                printf("File succesfully sent: %s \n", file_name);
                break;
            }
            file_block_size = fread(sendline, sizeof(char), 1024, file);
        }
        fclose(file);

        /* Close socket */
        if(close(comm_fd) < 0) error("Close socket error");
    }

}