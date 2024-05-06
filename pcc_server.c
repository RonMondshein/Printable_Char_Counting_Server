#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <endian.h>

#define MAX_BUFF_SIZE 1024
#define TRUE 1
/*Printable charcter is a byte b whose value is in [32, 126]*/
uint16_t pcc_total[127]; /*keep Printable charcter in [32,126]*/
int last_client = 1; /*1 didn't get signal, 0 got signal- used to handel the last client if necessary*/

/*Write to a socket (client) from a buffer  return 1 in error and 0 when it done*/
int write_to_client(int sockfd, void* buffer, int buffer_size){
    int bytes_written = 0; /*Specific written in a loop*/
    int total_bytes_written = 0; /*Total bytes that were written*/
    while(total_bytes_written < buffer_size){ /*Write the entire buffer to the socket*/
        bytes_written = write(sockfd, buffer + total_bytes_written, buffer_size - total_bytes_written);/*Write some byte to a socket*/
        if(bytes_written < 0){
            if(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE || errno == EOF){ /*Handel errors*/
                perror("Error: connection lost while writing to client\n");
                return 1;
            }  else if((errno == EINTR) && (last_client == 0)){ /*Handel it and then exit*/
                continue;
            } else{
                perror("Error: writing to server failed\n");
                exit(1);
            }
        }
        total_bytes_written += bytes_written;
    }
    return 0;
}

/*Read from a socket to a buffer return 1 in error and 0 when it done*/
int read_from_client(int sockfd, void* buffer, int buffer_size){
    int bytes_read = 0; /*Specific read in a loop*/
    int total_bytes_read = 0; /*Total bytes that were read*/
    while (total_bytes_read < buffer_size) /*Read the entire buffer into the buffer*/
    {
        bytes_read = read(sockfd, buffer + total_bytes_read, buffer_size - total_bytes_read); /*Read some bytes from a socket*/
        if(bytes_read < 0){
            if(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE || errno == EOF){ /*Handel errors*/
                perror("Error: connection lost while writing to client\n");
                return 1;
            }  else if(errno == EINTR && last_client == 0){ /*Handel it and then exit*/
                continue;
            } else{
            perror("Error: reading from server failed\n");
            exit(1);
            }
        }else if (bytes_read == 0) {
            /*Client has closed the connection gracefully*/
            perror("Client closed the connection");
            return 1; /*Return an error code to indicate client disconnection*/
        }
        
        total_bytes_read += bytes_read;
    }
    return 0;
}
/*1 if it prinable, 0 otherwise*/
int is_prinable(char c){
    if(c >= 32 && c <= 126) return 1;
    return 0;
}
/*
Function to handle communication with the client. 
Initializes variables for file size, printable characters, bytes read, and buffer size.
Keeps track of printable characters and their occurrences.
*/
void client_communication(int sockfd){
    uint16_t file_size = 0; /*The size of the file*/
    uint16_t printable_chars = 0; /*The number of printable characters in the file*/
    int bytes_read = 0; /*The number of bytes read from the file*/
    int left_to_read = 0;
    int current_size_to_read = 0;
    char send_buff[MAX_BUFF_SIZE]; /*Buffer to send to the server*/
    int done = -1;
    int i = 0;
    uint16_t current_pcc[127]; /*keep Printable charcter in [32,126]*/
    /*init current_pcc*/
    for(i = 0; i <= 126; i++){
        current_pcc[i] = 0;
    }
    /*Read from client to buffer the length of the information AKA N*/
    if(read_from_client(sockfd, &file_size, sizeof(file_size)) == 1){
        return; /*failed somewhere*/
    }
    file_size = ntohs((uint16_t)file_size); /*Get the size of the file*/
    /*Read from client to buffer the information that it's size is file_size using read_from_client*/
    while(bytes_read < file_size){
        left_to_read = file_size - bytes_read; /*shift what left*/
        /*adaject the amount of the bytes that we want*/
        if(left_to_read > MAX_BUFF_SIZE) current_size_to_read = MAX_BUFF_SIZE;
        else current_size_to_read = left_to_read;

        done = read_from_client(sockfd, send_buff, current_size_to_read); /*Read from client to buffer the information that it's size is file_size using read_from_client*/
        if(done == 1){
            return; /*failed somewhere*/
        }
        bytes_read += current_size_to_read;
        for(i = 0; i < current_size_to_read; i++) /*Update current_pcc with the current buffer*/
            if(is_prinable(send_buff[i])){
                printable_chars++;
                int place = (int)send_buff[i];
                current_pcc[place]++;
        }
    }
    printable_chars = htons(printable_chars); /*Get the number of printable characters*/

    if(write_to_client(sockfd, &printable_chars, sizeof(printable_chars))){ /*Send the number of printable characters to the client*/
        return; } /*failed somewhere*/
    /*Update pcc_total with current_pc*/
    for(int i = 32; i <= 126; i++){
        pcc_total[i] += current_pcc[i];
    }
}
 
/*Signal handel*/
void signal_handler(int signum){
    last_client = 0; /*signal that we got the signal*/
}
/*print according to the assignment*/
void print_pcc_total(){
        for(int i = 32; i <= 126; i++){
        printf("char '%c' : %hu times\n", (char)i, pcc_total[i]);
    }
}

int main(int argc, char const *argv[])
{
    uint16_t server_port = -1; /*The port of the server*/
    struct sockaddr_in serv_addr;  /*Where we want to get to*/
    if(argc != 2){
        perror("Error: invalid number of arguments\n");
        return 1;
    }
    server_port = (uint16_t)atoi(argv[1]); /*Get the port of the server*/
    /*init part*/
    /*init signal handler*/
    struct sigaction sig_act;
    sig_act.sa_handler = signal_handler;
    if (sigaction(SIGINT, &sig_act, NULL) < 0)
    {
        perror("Error: sigaction failed\n");
        return 1;
    }
    /*init pcc_total*/
    for(int i = 0; i < 127; i++){
        pcc_total[i] = 0;
    }
    /*Create a socket*/
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("Error: socket failed\n");
        return 1;
    }

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) { /*Set the socket option to reuse the address*/
        perror("Error: setsockopt failed\n");
        return 1;
    }
    memset(&serv_addr, 0, sizeof(serv_addr)); /*Clear the serv_addr struct*/
    serv_addr.sin_family = AF_INET; /*Set the address family to IPv4*/
    serv_addr.sin_port = htons(server_port); /*Set the port number to the specified port number*/
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*Set the IP address to the specified IP address*/
    /*Bind the socket to the specified port*/
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Error: bind failed\n");
        return 1;
    }
    /*Listen for incoming connections*/
    if(listen(sockfd, 10) < 0){
        perror("Error: listen failed\n");
        return 1;
    }
    int new_sockfd = -1; 
    /*Accept the incoming connection*/
    while(last_client){
         /*Accept new TCP connection*/
        new_sockfd = accept(sockfd, (struct sockaddr*)NULL, NULL); /*Accept the incoming connection*/
        if(new_sockfd < 0){
            if (last_client == 0 || errno == EINTR) /*if we got signal*/
            {
                break;   
            } else{
                perror("Error: accept failed\n");
                return 1;
            }
        }
        /*Receive the contents of the file from the client over the TCP connection*/
        client_communication(new_sockfd);
        /*Close the socket*/
        close(new_sockfd);
    }
    print_pcc_total();
    close(sockfd);
    return 0;
}
