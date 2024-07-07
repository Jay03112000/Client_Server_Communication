#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>

#define PORT 4535
#define MIRROR1_IP "127.0.0.1"

#define BUFFER_SIZE 4096


#define HOME_DIR "/home/patel449"
#define OUTPUT_DIR "/home/patel449/w24project"

// Compare function for sorting strings
int compare_strings(const void *a, const void *b) {
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

// Compare function for sorting directories by creation time
int compare_creation_time(const void *a, const void *b) {
    const char *dir1 = *(const char **)a;
    const char *dir2 = *(const char **)b;

    struct stat stat1, stat2;
    stat(dir1, &stat1);
    stat(dir2, &stat2);

    return stat1.st_atime - stat2.st_atime;
}

void handle_command(int client_socket, char *command, int client_number) {
    char buffer[BUFFER_SIZE]; // For communication
    char filename[BUFFER_SIZE]; // For constructing file paths
    struct stat stat_buffer; // For checking file information
    char tar_command[BUFFER_SIZE];
    
    
    if (strcmp(command, "dirlist -a") == 0) {
        // Print command received
        printf("\n----------------------------------------------------------------------------");
        printf("\nCommand received from Client-%d: %s\n", client_number, command);

        // Open the home directory
        DIR *dir = opendir(HOME_DIR);
        if (dir) {
            // Read each entry in the directory
            struct dirent *entry;
            char **dir_names = malloc(sizeof(char *) * BUFFER_SIZE);
            int count = 0;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    // If it's a directory and not "." or "..", add it to the dir_names array
                    dir_names[count] = strdup(entry->d_name);
                    count++;
                }
            }
            closedir(dir);

            // Sort the directory names in alphabetical order
            qsort(dir_names, count, sizeof(char *), compare_strings);

            // Construct the buffer with sorted directory names
            buffer[0] = '\0'; // Clear buffer
            for (int i = 0; i < count; i++) {
                strcat(buffer, dir_names[i]);
                strcat(buffer, "\n");
                free(dir_names[i]); // Free memory allocated by strdup
            }
            free(dir_names); // Free memory allocated for the array of pointers

            // Send the directory list to the client
            send(client_socket, buffer, strlen(buffer), 0);
            printf("Command output sent to Client-%d.\n", client_number);
        } else {
            // Failed to open directory, send error message
            sprintf(buffer, "Error: Unable to open directory: %s", strerror(errno));
            send(client_socket, buffer, strlen(buffer), 0);
        }
        printf("----------------------------------------------------------------------------\n\n");
    } else if (strcmp(command, "dirlist -t") == 0) {
        // Print command received
        printf("\n----------------------------------------------------------------------------");
        printf("\nCommand received from Client-%d: %s\n", client_number, command);

        // Open the home directory
        DIR *dir = opendir(HOME_DIR);
        if (dir) {
            // Read each entry in the directory
            struct dirent *entry;
            char **dir_names = malloc(sizeof(char *) * BUFFER_SIZE);
            int count = 0;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    // If it's a directory and not "." or "..", add it to the dir_names array
                    dir_names[count] = strdup(entry->d_name);
                    count++;
                }
            }
            closedir(dir);

            // Sort the directory names by creation time
            qsort(dir_names, count, sizeof(char *), compare_creation_time);

            // Construct the buffer with sorted directory names
            buffer[0] = '\0'; // Clear buffer
            for (int i = 0; i < count; i++) {
                strcat(buffer, dir_names[i]);
                strcat(buffer, "\n");
                free(dir_names[i]); // Free memory allocated by strdup
            }
            free(dir_names); // Free memory allocated for the array of pointers

            // Send the directory list to the client
            send(client_socket, buffer, strlen(buffer), 0);
            printf("Command output sent to Client-%d.\n", client_number);
        } else {
            // Failed to open directory, send error message
            sprintf(buffer, "Error: Unable to open directory: %s", strerror(errno));
            send(client_socket, buffer, strlen(buffer), 0);
        }
        printf("----------------------------------------------------------------------------\n\n");
    } else if (strncmp(command, "w24fn ", 6) == 0) {
        // Handle the w24fn command
        printf("\n----------------------------------------------------------------------------");
        printf("\nCommand received from Client-%d: %s\n", client_number, command);
    
        // Extract filename from command
        char *filename = command + 6;
    
        // Search for the file using find command
        char find_command[BUFFER_SIZE];
        sprintf(find_command, "find %s -type f -name '%s'", HOME_DIR, filename);
    
        FILE *find_output = popen(find_command, "r");
        if (find_output != NULL) {
            char file_path[BUFFER_SIZE];
            // Read the file path from the find command output
            if (fgets(file_path, sizeof(file_path), find_output) != NULL) {
                // Trim the newline character
                file_path[strcspn(file_path, "\n")] = '\0';
    
                // Get file information using stat
                struct stat file_stat;
                if (stat(file_path, &file_stat) == 0) {
                    // Prepare response
                    sprintf(buffer, "File: %s\nSize: %ld bytes\nDate created: %sPermissions: %o\n",
                            filename, file_stat.st_size, ctime(&file_stat.st_atime), file_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
                    
                    // Send response to client
                    send(client_socket, buffer, strlen(buffer), 0);
                    printf("Command output sent to Client-%d.\n", client_number);
                } else {
                    // Failed to get file information, send error message
                    sprintf(buffer, "Error: Unable to get file information: %s", strerror(errno));
                    send(client_socket, buffer, strlen(buffer), 0);
                }
            } else {
                // File not found, send error message
                sprintf(buffer, "File not found");
                send(client_socket, buffer, strlen(buffer), 0);
            }
            pclose(find_output);
        } else {
            // Failed to execute find command, send error message
            sprintf(buffer, "Error: Unable to execute find command");
            send(client_socket, buffer, strlen(buffer), 0);
        }
        printf("----------------------------------------------------------------------------\n\n");
    } else if (strncmp(command, "w24fz ", 6) == 0) {
        // Handle the w24fz command
        printf("\n----------------------------------------------------------------------------");
        printf("\nCommand received from Client-%d: %s\n", client_number, command);

        // Extract size parameters from command
        long size1, size2;
        sscanf(command + 6, "%ld %ld", &size1, &size2);

        // Create a tar file containing files within the specified size range
        char tar_command[BUFFER_SIZE];
        snprintf(tar_command, BUFFER_SIZE, "find %s -type f -size +%ldc -size -%ldc -print0 | tar --null -T - -czf - > %s/c%d/temp.tar.gz 2>/dev/null", HOME_DIR, size1, size2, OUTPUT_DIR, client_number);
        system(tar_command);

        // Check if the tar file is created
        char filename[BUFFER_SIZE];
        snprintf(filename, BUFFER_SIZE, "%s/c%d/temp.tar.gz", OUTPUT_DIR, client_number);
        struct stat stat_buffer;
        if (stat(filename, &stat_buffer) == 0) {
            // Send the tar file to the client
            FILE *tar_file = fopen(filename, "rb");
            if (tar_file != NULL) {
                fseek(tar_file, 0, SEEK_END);
                long tar_size = ftell(tar_file);
                rewind(tar_file);
                char *tar_data = malloc(tar_size);
                fread(tar_data, 1, tar_size, tar_file);
                fclose(tar_file);
                send(client_socket, tar_data, tar_size, 0);
                free(tar_data);
                printf("temp.tar.gz file sent to Client-%d.\n", client_number);
            }
        } else {
            // No files found within the specified size range
            send(client_socket, "No file found", strlen("No file found"), 0);
        }
        printf("----------------------------------------------------------------------------\n\n");
    } else if (strncmp(command, "w24ft ", 6) == 0) {
        // Handle the w24ft command
        printf("\n----------------------------------------------------------------------------");
        printf("\nCommand received from Client-%d: %s\n", client_number, command);

        // Extract extension list from command
        char ext1[10], ext2[10], ext3[10];
        int num_matched = sscanf(command + 6, "%9s %9s %9s", ext1, ext2, ext3);

        // Construct the find command to search for files with the specified extensions
        char command_buffer[BUFFER_SIZE];
        if (num_matched == 1) {
            sprintf(command_buffer, "find %s -type f -name '*.%s' -exec tar -cf %s/c%d/temp.tar {} + 2>/dev/null", HOME_DIR, ext1, OUTPUT_DIR, client_number);
        } else if (num_matched == 2) {
            sprintf(command_buffer, "find %s -type f \\( -name '*.%s' -o -name '*.%s' \\) -exec tar -cf %s/c%d/temp.tar {} + 2>/dev/null", HOME_DIR, ext1, ext2, OUTPUT_DIR, client_number);
        } else if (num_matched == 3) {
            sprintf(command_buffer, "find %s -type f \\( -name '*.%s' -o -name '*.%s' -o -name '*.%s' \\) -exec tar -cf %s/c%d/temp.tar {} + 2>/dev/null", HOME_DIR, ext1, ext2, ext3, OUTPUT_DIR, client_number);
        }

        // Execute the find command
        //printf("Command buffer: %s\n", command_buffer);
        int system_result = system(command_buffer);
        //printf("System result: %d\n", system_result);

        // Check if the temporary tar file is created
        char temp_tar_path[BUFFER_SIZE];
        sprintf(temp_tar_path, "%s/c%d/temp.tar", OUTPUT_DIR, client_number);
        if (access(temp_tar_path, F_OK) == 0) {
            //printf("Temporary tar file created: %s\n", temp_tar_path);
            //printf("temp.tar.gz file sent to Client-%d.\n", client_number);

            // Compress the tar file to tar.gz
            char gzip_command[BUFFER_SIZE];
            sprintf(gzip_command, "gzip -f %s", temp_tar_path);
            int gzip_result = system(gzip_command);
            //printf("Gzip result: %d\n", gzip_result);

            // Send success message to the client
            printf("temp.tar.gz file sent to Client-%d.\n", client_number);
        } else {
            //printf("Temporary tar file does not exist\n");
            strcpy(buffer, "No file found");
        }

        // Send response to the client
        send(client_socket, buffer, strlen(buffer), 0);
        
        printf("----------------------------------------------------------------------------\n\n");
    } else if (strncmp(command, "w24fdb ", 7) == 0) {
        printf("\n----------------------------------------------------------------------------");
        printf("\nCommand received from Client-%d: %s\n", client_number, command);
    
        // Extract date from command
        char date_str[20];
        sscanf(command + 7, "%s", date_str);
    
        // Convert date string to time
        struct tm tm_date = {0};
        if (strptime(date_str, "%Y-%m-%d", &tm_date) == NULL) {
            sprintf(buffer, "Error: Date format is incorrect.\n");
            send(client_socket, buffer, strlen(buffer), 0);
            return;
        }
        tm_date.tm_mday += 1; // Include the entire day
        mktime(&tm_date); // Normalize tm_date, though not necessary as we won't use it further
    
        // Create a tar file containing files created before the target date
        char find_command[BUFFER_SIZE];
        snprintf(find_command, BUFFER_SIZE, 
                 "find %s -type f ! -newerat '%s' -printf '%%p\\n'",
                 HOME_DIR, date_str);
    
        FILE *fp = popen(find_command, "r");
        if (fp == NULL) {
            sprintf(buffer, "Error: Failed to execute find command.\n");
            send(client_socket, buffer, strlen(buffer), 0);
            return;
        }
    
        //char filepath[BUFFER_SIZE];
        //while (fgets(filepath, BUFFER_SIZE, fp)) {
        //    if (filepath[strlen(filepath) - 1] == '\n') {
        //        filepath[strlen(filepath) - 1] = '\0'; // Strip newline character
        //    }
        //    printf("File checked: %s\n", filepath); // Print the file path
        //}
        //pclose(fp);
    
        // Assuming no errors from find, we proceed to create the tar file
        snprintf(tar_command, BUFFER_SIZE, 
                 "find %s -type f ! -newerat '%s' -print0 | tar --null -T - -czf %s/c%d/temp.tar.gz 2>/dev/null",
                 HOME_DIR, date_str, OUTPUT_DIR, client_number);
    
        int tar_status = system(tar_command);
        if (tar_status != 0) {
            sprintf(buffer, "Error: Failed to create tar file.\n");
            send(client_socket, buffer, strlen(buffer), 0);
            return;
        }
    
        // Check if the tar file is created
        snprintf(filename, BUFFER_SIZE, "%s/c%d/temp.tar.gz", OUTPUT_DIR, client_number);
        if (stat(filename, &stat_buffer) == 0) {
            sprintf(buffer, "Archive created successfully.\n");
        } else {
            sprintf(buffer, "Error: Archive file was not created.\n");
        }
    
        send(client_socket, buffer, strlen(buffer), 0);
        printf("temp.tar.gz sent to Client-%d.\n", client_number);
        printf("----------------------------------------------------------------------------\n\n");
    } else if (strncmp(command, "w24fda ", 7) == 0) {
        printf("\n----------------------------------------------------------------------------");
        printf("\nCommand received from Client-%d: %s\n", client_number, command);
    
        // Extract date from command
        char date_str[20];
        sscanf(command + 7, "%s", date_str);
    
        // Convert date string to time
        struct tm tm_date = {0};
        if (strptime(date_str, "%Y-%m-%d", &tm_date) == NULL) {
            sprintf(buffer, "Error: Date format is incorrect.\n");
            send(client_socket, buffer, strlen(buffer), 0);
            return;
        }
        tm_date.tm_mday += 1; // Include the entire day
        mktime(&tm_date); // Normalize tm_date, though not necessary as we won't use it further
    
        // Create a tar file containing files created before the target date
        char find_command[BUFFER_SIZE];
        snprintf(find_command, BUFFER_SIZE, 
                 "find %s -type f -newerat '%s' -printf '%%p\\n'",
                 HOME_DIR, date_str);
    
        FILE *fp = popen(find_command, "r");
        if (fp == NULL) {
            sprintf(buffer, "Error: Failed to execute find command.\n");
            send(client_socket, buffer, strlen(buffer), 0);
            return;
        }
    
        //char filepath[BUFFER_SIZE];
        //while (fgets(filepath, BUFFER_SIZE, fp)) {
        //    if (filepath[strlen(filepath) - 1] == '\n') {
        //        filepath[strlen(filepath) - 1] = '\0'; // Strip newline character
        //    }
        //    printf("File checked: %s\n", filepath); // Print the file path
        //}
        //pclose(fp);
    
        // Assuming no errors from find, we proceed to create the tar file
        snprintf(tar_command, BUFFER_SIZE, 
                 "find %s -type f -newerat '%s' -print0 | tar --null -T - -czf %s/c%d/temp.tar.gz 2>/dev/null",
                 HOME_DIR, date_str, OUTPUT_DIR, client_number);
    
        int tar_status = system(tar_command);
        if (tar_status != 0) {
            sprintf(buffer, "Error: Failed to create tar file.\n");
            send(client_socket, buffer, strlen(buffer), 0);
            return;
        }
    
        // Check if the tar file is created
        snprintf(filename, BUFFER_SIZE, "%s/c%d/temp.tar.gz", OUTPUT_DIR, client_number);
        if (stat(filename, &stat_buffer) == 0) {
            sprintf(buffer, "Archive created successfully.\n");
        } else {
            sprintf(buffer, "Error: Archive file was not created.\n");
        }
    
        send(client_socket, buffer, strlen(buffer), 0);
        printf("temp.tar.gz sent to Client-%d.\n", client_number);
        printf("----------------------------------------------------------------------------\n\n");
    } else {
        // Unknown command
        strcpy(buffer, "Unknown command.");
        send(client_socket, buffer, strlen(buffer), 0);
    }
    //printf("Command output sent to Client-%d.\n\n", client_number);
}

void crequest(int client_socket, int client_number) {
    char buffer[BUFFER_SIZE];
    int valread;

    // Send the client number to the client
    //char client_number_msg[BUFFER_SIZE];
    //snprintf(client_number_msg, BUFFER_SIZE, "Client number: %d", client_number);
    //send(client_socket, client_number_msg, strlen(client_number_msg), 0);
    //printf("\n%s - sent to client from mirror\n", client_number_msg);

    while (1) {
        // Receive command from the client
        valread = read(client_socket, buffer, BUFFER_SIZE);
        buffer[valread] = '\0';
        

        // Handle the command
        handle_command(client_socket, buffer, client_number);

        // Check if the client wants to quit
        if (strcmp(buffer, "quitc") == 0) {
            printf("Client-%d disconnected.\n", client_number);
            break;
        }
    }
    close(client_socket);
}



int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("\nMirror is running and waiting for connections.\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(MIRROR1_IP);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-]Bind error");
        exit(1);
    }

    listen(serverSocket, 5);

    while (1) {
        addr_size = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
        if (clientSocket < 0) {
            perror("[-]Accept error");
            continue;
        }

        //printf("Connection accepted.\n");

        // Read the client number from the server
        char client_number_msg[BUFFER_SIZE];
        int valread = read(clientSocket, client_number_msg, BUFFER_SIZE - 1);
        
        if (valread > 0) {
            client_number_msg[valread] = '\0'; // Null-terminate the message
            int client_number;
            sscanf(client_number_msg, "Client number: %d", &client_number);
            printf("Client-%d connectec.\n",client_number);
            // You can now handle this client with their specific client number
            crequest(clientSocket, client_number);
        } else {
            if (valread == 0) {
                // The server closed the connection
                printf("Server closed the connection.\n");
            } else {
                perror("Error reading from socket");
            }
        }
        close(clientSocket);
    }
    return 0;
}

