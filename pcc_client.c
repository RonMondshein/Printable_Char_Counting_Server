#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <endian.h>

#define MAX_BUFF_SIZE 1024

/*Write to a socket (server) from a buffer*/
void write_to_server(int sockfd, void* buffer, int buffer_size){
    int bytes_written = 0; /*Specific written in a loop*/
    int total_bytes_written = 0; /*Total bytes that were written*/

    while(total_bytes_written < buffer_size){ /*Write the entire buffer to the socket*/
        bytes_written = write(sockfd, buffer + total_bytes_written, buffer_size - total_bytes_written);/*Write some byte to a socket*/
        if(bytes_written < 0){
            perror("Error: writing to server failed\n");
            exit(1);
        }
        total_bytes_written += bytes_written;
    }
}

/*Read from a socket to a buffer*/
void read_from_server(int sockfd, void* buffer, int buffer_size){
    int bytes_read = 0; /*Specific read in a loop*/
    int total_bytes_read = 0; /*Total bytes that were read*/

    while (total_bytes_read < buffer_size) /*Read the entire buffer into the buffer*/
    {
        bytes_read = read(sockfd, buffer + total_bytes_read, buffer_size - total_bytes_read); /*Read some bytes from a socket*/
        if(bytes_read < 0){
            perror("Error: reading from file failed\n");
            exit(1);
        }
        total_bytes_read += bytes_read;
    }
}

/*
Send the contents of the file to the server over the TCP connection
Send N, the file size, to the server. N is a 16-bit unsigned integer.
Send the file to the server. The file is sent in chunks of size MAX_BUFF_SIZE.
get back C, the number of prinable characters in the file. C is a 16-bit unsigned integer.
*/
void server_communication(int sockfd, int file_to_send){
    uint16_t file_size = 0; /*The size of the file*/
    uint16_t printable_chars = 0; /*The number of printable characters in the file*/
    int bytes_read; /*The number of bytes read from the file*/
    char send_buff[MAX_BUFF_SIZE]; /*Buffer to send to the server*/

    /*Get the size of the file*/
    file_size = (uint16_t)lseek(file_to_send, 0, SEEK_END); /*Move the file pointer to the end of the file*/
    file_size = htons((uint16_t)file_size); /*Get the size of the file*/
    lseek(file_to_send, 0, SEEK_SET); /*Move the file pointer to the beginning of the file*/

    /*Send the size of the file to the server*/
    write_to_server(sockfd, &file_size, sizeof(uint16_t));

    /*Send the file to the server*/
    while((bytes_read = read(file_to_send, send_buff, sizeof(send_buff))) > 0){
        write_to_server(sockfd, send_buff, bytes_read);
    }
    
    /*Get the number of printable characters in the file*/
    
    read_from_server(sockfd, &printable_chars, sizeof(uint16_t));
    int16_t p_c = ntohs(printable_chars); /*Convert the number of printable characters to host byte order*/
    printf("# of printable characters: %hu\n", p_c);
}


int main(int argc, char const *argv[])
{
    /*command line arguments*/
    const char* server_IP_address;
    uint16_t server_port; /*Assumed a 16-bit unsigned int*/
    const char* path_of_file; 
    int sockfd = -1; /*Socket file descriptor*/
    struct sockaddr_in serv_addr;  /*Where we want to get to*/

    if(argc != 4) /*Check the number of arguments*/
    {
        perror("wrong number of arguments\n");
        exit(1);
    }
    server_IP_address = argv[1];
    server_port = (uint16_t)atoi(argv[2]);
    path_of_file = argv[3];

    /*Open the specified file to reading*/
    int file_to_send = open(path_of_file, O_RDONLY);
    if(file_to_send == -1)
    {
        perror("Error: file not found\n");
        exit(1);
    }

    /*Create a TCP connection to the specified server port on the specified server IP*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) <  0){ /*Create a socket. returns a socket descriptor. Using TCP connection.*/
        perror("Error: socket creation failed\n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr)); /*Clear the serv_addr struct*/
    serv_addr.sin_family = AF_INET; /*Set the address family to IPv4*/
    serv_addr.sin_port = htons(server_port); /*Set the port number to the specified port number*/
    inet_pton(AF_INET, server_IP_address, &serv_addr.sin_addr.s_addr);/*Set the IP address to the specified IP address*/

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) /*Connect to the server*/
    {
        perror("Error: connection failed\n");
        exit(1);
    }

    /*Transfer the contents of the file to the server over the TCP connection*/
    server_communication(sockfd, file_to_send);

    /*Close the file and the socket*/
    close(file_to_send);
    close(sockfd);

    /*Exit the program*/
    return 0;
}
