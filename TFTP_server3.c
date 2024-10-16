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

void die(const char *s) {
    perror(s);
    exit(1);
}

void ack(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, int block_number) {
    char ack[4] = {0, 4, (block_number >> 8) & 0xFF, block_number & 0xFF};
    sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *) client_addr, client_len);
}

void error_message(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, const char *message) {
    char buffer[BUFFER_SIZE] = {0, 5, 0, 1};  // Error opcode and Not found error code
    strcpy(buffer + 4, message);
    sendto(sockfd, buffer, 4 + strlen(message) + 1, 0, (struct sockaddr *) client_addr, client_len);
}

void send_file(int new_sockfd, struct sockaddr_in *client_addr, socklen_t client_len, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        error_message(new_sockfd, client_addr, client_len, "File not found");
        return;
    }

    int block_number = 0; // Start from zero for proper wraparound calculation
    char data_buffer[BUFFER_SIZE];
    int read_bytes, attempts;

    while ((read_bytes = fread(data_buffer + 4, 1, BLOCK_SIZE, file)) > 0) {
        block_number++;
        if (block_number > 65535) {  // Handle wraparound
            block_number = 1;
        }

        data_buffer[0] = 0; // Opcode for DATA
        data_buffer[1] = 3; // DATA opcode
        data_buffer[2] = (block_number >> 8) & 0xFF;
        data_buffer[3] = block_number & 0xFF;

        attempts = 0;
        while (attempts < 5) {  // Retry logic
            if (sendto(new_sockfd, data_buffer, read_bytes + 4, 0, (struct sockaddr *) client_addr, client_len) < 0) {
                perror("sendto");
                continue;
            }

            char ack_buffer[4];
            socklen_t len = client_len;
            if (recvfrom(new_sockfd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *) client_addr, &len) < 0) {
                perror("recvfrom");
                attempts++;
                continue;
            }

            if (ack_buffer[1] == 4 && (((ack_buffer[2] << 8) | ack_buffer[3]) == block_number)) {
                break; // Correct ACK received
            }
            attempts++;
        }
        if (attempts == 5) {
            printf("Failed to receive ACK for block %d, aborting transfer\n", block_number);
            break;
        }
    }
    fclose(file);
}


void receive_file(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, char *filename)
{
    FILE *file=fopen(filename,"w");
    if(!file)
    {
        printf("Unable to open file\n");
        //char* error_send=error_message("Unable to create file");
        error_message(sockfd,client_addr,client_len,"Unable to create file");
        //to do: send error ack
    }
    //recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &client_length);
// Send ACK for WRQ (block number 0)
    ack(sockfd, client_addr, client_len, 0);

    int expected_block = 1;

    while (1) {
        char buffer[1024];
        int len = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)client_addr, &client_len);
        if (len < 0) {
            perror("recvfrom error");
            break;
        }

        int block_number = (buffer[2] << 8) | buffer[3];

        // The block number should match expected block to receive blocks in order
        if ((buffer[1] == 3) && (block_number == expected_block)) {  // Data packet
            fwrite(buffer + 4, 1, len - 4, file);
            printf("acknowledging with block number :%d\n",block_number);
            ack(sockfd, client_addr, client_len, block_number);
            if (len - 4 < 512) {  // Last packet
                break;
            }
            expected_block++;
        }
    }
    printf("File received: %s\n",filename);
    fclose(file);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        die("socket failed");

    struct sockaddr_in server_addr = {0}, client_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        die("bind failed");

    printf("TFTP server started on port %d...\n", PORT);

    while (1) {
        char buffer[BUFFER_SIZE];
        socklen_t client_len = sizeof(client_addr);
        int len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
        if (len < 0)
            die("recvfrom failed");

        if (fork() == 0) {
            close(sockfd);  // Close the parent socket

            int new_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (new_sockfd < 0)
                die("socket creation failed in child");

            struct sockaddr_in new_addr = {0};
            new_addr.sin_family = AF_INET;
            new_addr.sin_port = htons(0);  // Ephemeral port
            new_addr.sin_addr.s_addr = htonl(INADDR_ANY);

            if (bind(new_sockfd, (struct sockaddr *)&new_addr, sizeof(new_addr)) < 0)
                die("bind failed in child");

            // Process the request
            if (buffer[1] == 1) { // RRQ
            printf("Received RRQ request\n");
                send_file(new_sockfd, &client_addr, client_len, buffer + 2);
            }
            else if(buffer[1]==2)  //WRQ
        {
        char *filename = &buffer[2];  // Filename starts at buffer[2]
        char *mode = filename + strlen(filename) + 1;  // Mode follows the filename
        printf("Received WRQ for file: %s\n", filename);
        printf("Mode: %s\n", mode);
        receive_file(sockfd, &client_addr, client_len, buffer + 2);
        }
        else if(buffer[1] == 4)
        {
            printf("Received ACK from client\n");
        }
            // Additional code to handle WRQ and other cases can be added here

            close(new_sockfd);
            exit(0);
        }
    }
    close(sockfd);
    return 0;
}
