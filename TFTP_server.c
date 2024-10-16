#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024
#define PORT 69
#define BLOCK_SIZE 512

#define ERROR_FILE_NOT_FOUND 1
#define ACK 4
#define MAX_TRIES 10

void die(char *s) {
    perror(s);
    exit(1);
}

void ack(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, int block_number) {
    char ack[4];
    ack[0] = 0; 
    ack[1] = ACK;  
    ack[2] = (block_number >> 8) & 0xFF;  //1 byte
    ack[3] = block_number & 0xFF; //1 byte
    sendto(sockfd, ack, 4, 0, (struct sockaddr *) client_addr, client_len);
    printf("ACK sent for block %d\n", block_number);
}

// Used by RRQ function
void send_file(int sockfd, struct sockaddr_in *client_addr, socklen_t client_length, char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("File not found: %s\n", filename);
        char* message = "File not found";
        char error_msg[1024];
        error_msg[0] = 0; error_msg[1] = 5; error_msg[2] = 0; error_msg[3] = 1;
        strcpy(error_msg + 4, message);
        sendto(sockfd, error_msg, strlen(message) + 4, 0, (struct sockaddr *) client_addr, client_length);
        printf("Exiting child process\n");
        close(sockfd);
        exit(1);
    }

    printf("Transfer started for %s\n", filename);
    int block_number = 1;
    char buffer[BUFFER_SIZE];
    int nread;
    int retry_count;
    fd_set read_fds;
    struct timeval timeout;

    do {
        nread = fread(buffer + 4, 1, BLOCK_SIZE, file);
        buffer[0] = 0;
        buffer[1] = 3; // Data opcode
        buffer[2] = (block_number >> 8) & 0xFF;
        buffer[3] = block_number & 0xFF;

        retry_count = 0;
        while (retry_count < MAX_TRIES) {
            sendto(sockfd, buffer, nread + 4, 0, (struct sockaddr *) client_addr, client_length);
            printf("Data packet %d sent, size %d\n", block_number, nread);

            FD_ZERO(&read_fds);
            FD_SET(sockfd, &read_fds);
            timeout.tv_sec = 2;  // 2 seconds timeout
            timeout.tv_usec = 0;

            int sel = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
            if (sel > 0) {
                char ack_buffer[4];
                int len = recvfrom(sockfd, ack_buffer, 4, 0, (struct sockaddr *) client_addr, &client_length);
                if (len >= 4 && ack_buffer[1] == ACK && 
                    ((ack_buffer[2] << 8) | ack_buffer[3]) == block_number) {
                    break; // ACK received, break out of retry loop
                }
            } else if (sel == 0) {
                printf("Timeout, retrying %d...\n", block_number);
                retry_count++;
            } else {
                perror("select error");
                break;
            }
        }

        
        if (retry_count == MAX_TRIES) {
            printf("Max retries reached\n Closing child process\n");
            fclose(file);
            close(sockfd);
            exit(1);
        }
        block_number++;
    } while (nread == BLOCK_SIZE);

    // To handle file size of 2048 Bytes. Send NULL
    if (nread == 0) {
        buffer[0] = 0;
        buffer[1] = 3;
        buffer[2] = (block_number >> 8) & 0xFF;
        buffer[3] = block_number & 0xFF;
        sendto(sockfd, buffer, 4, 0, (struct sockaddr *) client_addr, client_length);
        printf("Final empty data packet sent to indicate completion.\n");
    }

    fclose(file);
    close(sockfd);
    printf("Transfer completed for %s\n", filename);
}

// Used by WRQ function
void receive_file(int sockfd, struct sockaddr_in *client_addr, socklen_t client_length, char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Unable to open file: %s\n", filename);
        char* message="Unable to create file";
        char error_msg[1024];
        error_msg[0] = 0; error_msg[1] = 5; error_msg[2] = 0; error_msg[3] = 1;
        strcpy(error_msg+4, message);
        sendto(sockfd, error_msg, strlen(message) + 4, 0, (struct sockaddr *) client_addr, client_length);
        printf("Exiting child process\n");
        close(sockfd);
        exit(1);
    }

    printf("Receiving file: %s\n", filename);
    ack(sockfd, client_addr, client_length, 0); // Send ACK for WRQ (block number 0)

    int expected_block = 1;
    while (1) {
        char buffer[1024];
        int len = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)client_addr, &client_length);
        if (len < 0) {
            perror("recvfrom error");
            break;
        }

        int block_number = (buffer[2] << 8) | buffer[3];
        if ((buffer[1] == 3) && (block_number == expected_block)) {
            fwrite(buffer + 4, 1, len - 4, file);
            printf("acknowledging with block number: %d\n", block_number);
            ack(sockfd, client_addr, client_length, block_number);
            if (len - 4 < BLOCK_SIZE) {
                break;
            }
            expected_block++;
        }
    }

    fclose(file);
    close(sockfd);
    printf("File received: %s\n", filename);
}

int main(int argument_number, char** argv) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_length = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) die("socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));//htons(PORT);
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      // send input IP Adress
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        printf("Invalid address / Address not supported\n");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        die("bind");

    printf("TFTP server started...\n");

    while (1) {
        char buffer[BUFFER_SIZE];
        if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &client_length) < 0)
            die("recvfrom");

        if (fork() == 0) {
            close(sockfd); // Execution to be continued by the child process

            int new_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (new_sockfd < 0) die("socket");

            struct sockaddr_in new_addr;
            memset(&new_addr, 0, sizeof(new_addr));
            new_addr.sin_family = AF_INET;
            new_addr.sin_port = htons(0); // Use an ephemeral port. As mentioned in MP3.pdf
            new_addr.sin_addr.s_addr = htonl(INADDR_ANY);

            if (bind(new_sockfd, (struct sockaddr *)&new_addr, sizeof(new_addr)) < 0)
                die("bind");

            if (buffer[1] == 1) { // RRQ
                printf("Received RRQ for file: %s\n", buffer + 2);
                send_file(new_sockfd, &client_addr, client_length, buffer + 2);
            } else if (buffer[1] == 2) { // WRQ
                printf("Received WRQ for file: %s\n", buffer + 2);
                receive_file(new_sockfd, &client_addr, client_length, buffer + 2);
            }
            exit(0);
        }
    }
    close(sockfd);
    return 0;
}

