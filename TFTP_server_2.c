#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include<sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024
#define PORT 69
#define BLOCK_SIZE 512

#define ERROR_FILE_NOT_FOUND 1
#define ACK 4

void die(char *s) {
    perror(s);
    exit(1);
}

void ack(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, int block_number) {
    char ack[4];
    ack[0] = 0; 
    ack[1] = ACK;  
    ack[2] = (block_number >> 8) & 0xFF;
    ack[3] = block_number & 0xFF;
    sendto(sockfd, ack, 4, 0, (struct sockaddr *) client_addr, client_len);
}

char* error_message(int sockfd,struct sockaddr_in *client_addr,socklen_t client_length,char* message, int error_code)
{
    printf("message: %s\n",message);
    char* buffer=(char*)malloc(BUFFER_SIZE);
    buffer[0] = 0; // Opcode for DATA
    buffer[1] = 5;

    if(error_code==ERROR_FILE_NOT_FOUND)
    {
        buffer[2]=0;
        buffer[3]=(char)1;
    }
    strcpy(buffer+4,message);
    //buffer[4+strlen(message)]=0;
    sendto(sockfd,buffer,strlen(message)+4,0, (struct sockaddr *) client_addr, client_length);

    return buffer;
}
void send_file(int sockfd, struct sockaddr_in *client_addr, socklen_t client_length, char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("File not found: %s\n", filename);
        char* message="File not found";
        //char* error_buffer = error_message(message,ERROR_FILE_NOT_FOUND);
        error_message(sockfd,client_addr,client_length,message,ERROR_FILE_NOT_FOUND);
        //printf("Size of buffer: %ld\n",strlen(error_buffer));
        //sendto(sockfd, error_msg, strlen(error_msg), 0, (struct sockaddr *) client_addr, client_length);
        //sendto(sockfd,error_buffer,strlen(message)+4,0, (struct sockaddr *) client_addr, client_length);
        printf("Exiting child process\n");
        close(sockfd);
        return;
    }

    int block_number = 1;
    char buffer[BUFFER_SIZE], ack_buffer[BUFFER_SIZE];
    int nread, retry_count, ack_len;
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);

    while ((nread = fread(buffer + 4, 1, BLOCK_SIZE, file)) > 0) {
        buffer[0] = 0; // Opcode for DATA
        buffer[1] = 3;
        buffer[2] = (block_number >> 8) & 0xFF; // Block Number. This will be incrementing
        buffer[3] = block_number & 0xFF;

        retry_count = 0;
        while (retry_count < 5) {
            if (sendto(sockfd, buffer, nread + 4, 0, (struct sockaddr *) client_addr, client_length) < 0) {
                perror("sendto failed");
                break;
            }

            fd_set read_fds;
            struct timeval timeout;

            FD_ZERO(&read_fds);
            FD_SET(sockfd, &read_fds);

            timeout.tv_sec = 2;  // Set timeout (e.g., 2 seconds)
            timeout.tv_usec = 0;

            int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
            if (activity < 0 && errno != EINTR) {
                perror("select error");
                break;
            } else if (activity > 0) {
                ack_len = recvfrom(sockfd, ack_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from, &from_len);
                if (ack_len >= 4 && ack_buffer[1] == 4 &&  // Opcode for ACK
                    (((ack_buffer[2] << 8) | ack_buffer[3]) == block_number)) {
                    break;  // Correct ACK received
                }
            }

            retry_count++;
            printf("Retrying block %d\n", block_number);
        }
        if(retry_count>=5)
        {
        printf("Client is disconnected. File transfer failed\n");
        printf("Closing child process\n");
        close(sockfd);
        exit(0);
        }

        block_number++;
    }
    
    printf("Transfer completed for %s\n", filename);
    fclose(file);
}

//To Do:
void receive_file(int sockfd, struct sockaddr_in *client_addr, socklen_t client_length, char *filename)
{
    FILE *file=fopen(filename,"w");
    if(!file)
    {
        printf("Unable to open file\n");
        //char* error_send=error_message("Unable to create file");
        error_message(sockfd,client_addr,client_length,"Unable to create file",ERROR_FILE_NOT_FOUND);
        //to do: send error ack
    }
    //recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &client_length);
// Send ACK for WRQ (block number 0)
    ack(sockfd, client_addr, client_length, 0);

    int expected_block = 1;

    while (1) {
        char buffer[1024];
        int len = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)client_addr, &client_length);
        if (len < 0) {
            perror("recvfrom error");
            break;
        }

        int block_number = (buffer[2] << 8) | buffer[3];

        // The block number should match expected block to receive blocks in order
        if ((buffer[1] == 3) && (block_number == expected_block)) {  // Data packet
            fwrite(buffer + 4, 1, len - 4, file);
            printf("acknowledging with block number :%d\n",block_number);
            ack(sockfd, client_addr, client_length, block_number);
            if (len - 4 < 512) {  // Last packet
                break;
            }
            expected_block++;
        }
    }
    printf("File received: %s\n",filename);
    fclose(file);
}


int client_number=0;
int main() {
    int sockfd;
    pid_t child_id;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_length = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        die("socket");

    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        die("bind");

    printf("TFTP server started...\n");

    while (1) {
        if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &client_length) < 0)
        break;
          //  die("recvfrom");
        if((child_id = fork())==0)
        {
        //    printf("Child PID: %d\n",child_id);
        //close(sockfd);
        printf("Buffer obtained from client %d: \n",client_number);
        printf("Decoding\n");
       // printf("total Buffer: %s\n",total_buff);
       int buf_0=buffer[0];
       int buf_1=buffer[1];
       //printf("Buffer0: %d\n",buf_0);
       //printf("Buffer1: %d\n",buf_1);
        if (buffer[1] == 1) { // RRQ
        char *filename = &buffer[2];  // Filename starts at buffer[2]
        char *mode = filename + strlen(filename) + 1;  // Mode follows the filename

        printf("Received RRQ for file: %s\n", filename);
        printf("Mode: %s\n", mode);
            send_file(sockfd, &client_addr, client_length, buffer + 2);
        }
        else if(buffer[1]==2)  //WRQ
        {
        char *filename = &buffer[2];  // Filename starts at buffer[2]
        char *mode = filename + strlen(filename) + 1;  // Mode follows the filename
        printf("Received WRQ for file: %s\n", filename);
        printf("Mode: %s\n", mode);
        receive_file(sockfd, &client_addr, client_length, buffer + 2);
        }
        else if(buffer[1] == 4)
        {
            printf("Received ACK from client\n");
        }
        close(sockfd);
       }
        // Implement WRQ needed
    }
    close(sockfd);
    return 0;
}
