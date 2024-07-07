#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 6855
#define BUFFER_SIZE 1024

#define OUTPUT_DIR "/home/patel449/w24project/"

// Function prototypes
void send_command(int sock, const char *command, struct sockaddr_in serverAddr);
void receive_response(int sock, const char *command, int client_number);
void receive_file_info(int sock, const char *command, int client_number);

int main(int argc, char const *argv[]) {
    // Client socket variables
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Buffer for user command
    char command[BUFFER_SIZE];

    // Create the client socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Set up the server address structure
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    // Client number exchange with the server
    char client_number_msg[BUFFER_SIZE];
    int valread = read(sock, client_number_msg, BUFFER_SIZE);
    client_number_msg[valread] = '\0';
    int client_number;
    sscanf(client_number_msg, "Client number: %d", &client_number);

    // Create client directory
    char client_dir[BUFFER_SIZE];
    sprintf(client_dir, "%sc%d", OUTPUT_DIR, client_number);
    mkdir(client_dir, 0777);
    chdir(client_dir);

    

    // Main interaction loop
    while (1) {
        // User input for command
        printf("\n----------------------------------------------------------------------------");
        printf("\nEnter command: ");
        fgets(command, BUFFER_SIZE, stdin);                     // Read command from stdinput and stores it in command 
        command[strcspn(command, "\n")] = '\0';                 //find position of first line of command 

        // Check if command is 'quitc'
        if (strcmp(command, "quitc") == 0) {
            send_command(sock, command, serv_addr);
            break;
        }

        // Check and send command
        if (strcmp(command, "dirlist -a") == 0 || strcmp(command, "dirlist -t") == 0 || strncmp(command, "w24fn ", 6) == 0 || strncmp(command, "w24fz ", 6) == 0 || strncmp(command, "w24ft ", 6) == 0 || strncmp(command, "w24fdb ", 7) == 0 || strncmp(command, "w24fda ", 7) == 0) {
            send_command(sock, command, serv_addr);                    //if commands are proper than client will send it to server 
            if (strncmp(command, "w24fn ", 6) == 0) {
                receive_file_info(sock, command, client_number);
            } else {
                receive_response(sock, command, client_number);
            }
        } else {
            printf("Invalid command.\n");
        }
    }

    // Close socket
    close(sock);
    return 0;
}

// Function to send command to server
void send_command(int sock, const char *command, struct sockaddr_in serverAddr) {
    send(sock, command, strlen(command), 0);
    printf("Command sent: %s\n", command);
}

// Function to receive response from server
void receive_response(int sock, const char *command, int client_number) {
    char buffer[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    int valread = read(sock, buffer, BUFFER_SIZE);
    buffer[valread] = '\0';


    // Store the output in a text file in the client's directory
    if (strcmp(command, "dirlist -a") == 0 || strcmp(command, "dirlist -t") == 0 || strncmp(command, "w24fn ", 6) == 0) {
      printf("\nServer response:\n%s\n", buffer);
      sprintf(filename, "%sc%d/%s.txt", OUTPUT_DIR, client_number, command);
      FILE *file = fopen(filename, "w");
      if (file == NULL) {
          perror("Error creating file");
          return;
      }
      fprintf(file, "%s", buffer);
      fclose(file);
      printf("Server response saved in %s\n", filename);
      printf("----------------------------------------------------------------------------\n\n");
    } else if (strncmp(command, "w24fz ", 6) == 0 || strncmp(command, "w24ft ", 6) == 0 || strncmp(command, "w24fdb ", 7) == 0 || strncmp(command, "w24fda ", 7) == 0)  {
        if (strcmp(buffer, "File not found") == 0) {
          printf("%s\n", buffer);
          printf("----------------------------------------------------------------------------\n\n");
        } else {
            printf("Server response saved in %stemp.tar.gz\n", OUTPUT_DIR);
            printf("----------------------------------------------------------------------------\n\n");
        }
    }
}

// Function to receive file info
void receive_file_info(int sock, const char *command, int client_number) {
    char buffer[BUFFER_SIZE];
    int valread = read(sock, buffer, BUFFER_SIZE);
    buffer[valread] = '\0';
    
    printf("\nServer response:\n%s\n", buffer);
    
    if (strcmp(buffer, "File not found") == 0) {
        printf("%s", buffer);
    } else {
        char filename_buffer[BUFFER_SIZE];
        sprintf(filename_buffer, "%sc%d/w24fn.txt", OUTPUT_DIR, client_number);
        FILE *output_file = fopen(filename_buffer, "w");
        if (output_file != NULL) {
            fprintf(output_file, "%s", buffer);
            fclose(output_file);
            printf("Server response saved in %s\n", filename_buffer);
        } else {
            printf("Error: Unable to save response\n");
        }
    }
    printf("----------------------------------------------------------------------------\n\n");
}