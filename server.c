#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
#define st_mtim st_mtimespec
#endif

#define DELIM 0x20

const char* HOST = "127.0.0.1";
const int PORT = 2000;
char *USB[] = {"USB1/", "USB2/"};

void read_file(const char* file_path, char* server_message);
void info_file(char const* file_path, char* server_message);
void put_file(char const* file_path, char* put_content, char* server_message);
void create_dir(char const* folder_name, char* server_message);
void remove_file(char const* path, char* server_message);

int main(void) {
    int socket_desc, client_sock;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;
    char server_message[8196], client_message[8196];
    char command[5];
    char file_path[100];
    char put_content[1024];
    char delim[2];

    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
    memset(command, '\0', sizeof(command));
    memset(file_path, '\0', sizeof(file_path));
    memset(put_content, '\0', sizeof(put_content));
    memset(delim, '\0', sizeof(delim));

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0) {
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(HOST);

    // Bind to the set port and IP:
    if (bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
        0) {
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");

    // Listen for clients:
    if (listen(socket_desc, 10) < 0) {
        printf("Error while listening\n");
        return -1;
    }

    char mount_command[100];
    int error = 0;

    printf("\nPlease enter the USB name for first storage: E.g. /dev/sdb1;\
    \nFail to enter the right device will result write to local:\n");

    char location[50];
    scanf("%s", location);
    strcpy(mount_command, "sudo mount ");
    strcat(mount_command, location);
    strcat(mount_command, " ./USB1");
    error = system(mount_command);
    if (error == 0) {
        printf("\nSuccessfully mounted first USB.\n");
    } else{
        printf("\nFail mounted first USB\n");
    }
    memset(location, '\0', sizeof(location));
    memset(mount_command, '\0', sizeof(mount_command));

    printf("Please enter the USB name for second storage: E.g. /dev/sdb1;\
    \nfail to enter the right device will result write to local:\n");

    scanf("%s", location);
    strcpy(mount_command, "sudo mount ");
    strcat(mount_command, location);
    strcat(mount_command, " ./USB2");
    error = system(mount_command);
    if (error == 0) {
        printf("Successfully mounted second USB.\n");
    } else{
        printf("Fail mounted second USB");
    }

    printf("\nListening for incoming connections.....\n");

    while (1) {
        // Accept an incoming connection:
        client_size = sizeof(client_addr);
        client_sock =
            accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);

        if (client_sock < 0) {
            return -1;
        }
        printf("Client connected at IP: %s and port: %i\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // get params
        
        if (fork() == 0) {
            // Receive client's message:
            if (recv(client_sock, client_message, sizeof(client_message), 0) <
                0) {
                printf("Couldn't receive\n");
                return -1;
            }
            printf("Msg from client: %s\n", client_message);
            // command switch
            sprintf(delim, "%c", DELIM);
            strcpy(command, strtok(client_message, delim));
            strcpy(file_path, strtok(NULL, delim));
        
            if (strcmp(command, "GET") == 0) {
                read_file(file_path, server_message);
            } else if (strcmp(command, "INFO") == 0) {
                info_file(file_path, server_message);
            } else if (strcmp(command, "MD") == 0) {
                create_dir(file_path, server_message);
            } else if (strcmp(command, "PUT") == 0) {
                strcpy(put_content, strtok(NULL, delim));
                put_file(file_path, put_content, server_message);
            } else if (strcmp(command, "RM") == 0) {
                remove_file(file_path, server_message);
            } else {
                strcpy(server_message, "Error command");
            }

            // Respond to client:
            if (send(client_sock, server_message, strlen(server_message),
                        0) < 0) {
                printf("Can't send\n");
                return -1;
            }

            // Clean buffers:
            memset(server_message, '\0', sizeof(server_message));
            memset(client_message, '\0', sizeof(client_message));
            memset(command, '\0', sizeof(command));
            memset(file_path, '\0', sizeof(file_path));
            memset(put_content, '\0', sizeof(put_content));
            memset(delim, '\0', sizeof(delim));
            

            // Closing the socket:
            close(socket_desc);
            close(client_sock);
        } else {
            close(client_sock);
            memset(server_message, '\0', sizeof(server_message));
            memset(client_message, '\0', sizeof(client_message));
            memset(command, '\0', sizeof(command));
            memset(file_path, '\0', sizeof(file_path));
            memset(put_content, '\0', sizeof(put_content));
            memset(delim, '\0', sizeof(delim));
        }
    }
    close(socket_desc);
    close(client_sock);
    

    return 0;
}

// GET
void read_file(char const* file_path, char* server_message) {
    char* file_content;
    char usb_chosen_path[strlen(file_path) + strlen(USB[0]) + 10];
    // concatenates str1 and str2
   // the resultant string is stored in str1.
    strcat(usb_chosen_path, USB[rand() % 2]);
    strcat(usb_chosen_path, file_path);

    FILE* pfile = fopen(usb_chosen_path, "rb");
    if (pfile == NULL) {
        sprintf(server_message, "open file %s failed!\n", file_path);
        return;
    }
    // fseek() is used to move file pointer associated with a given file to a specific position.
    fseek(pfile, 0, SEEK_END);

    // ftell() in C is used to find out the position of file pointer in the file with respect to starting of the file.
    int file_length = ftell(pfile);

    // The rewind() function repositions the file pointer associated with stream to the beginning of the file.
    rewind(pfile);
    
    //  The fread function reads up to count items of size bytes from the input stream and stores them in buffer 
    int read_length = fread(server_message, 1, file_length, pfile);
    if (read_length != file_length) {
        sprintf(server_message, "read file %s failed!\n", file_path);
        return;
    }

    fclose(pfile);
    pfile = NULL;
}

// INFO
void info_file(char const* file_path, char* server_message) {
    char permisson[20] = {0};
    char info_chosen_path[strlen(file_path) + strlen(USB[0]) + 10];
    memset(info_chosen_path, '\0', sizeof(info_chosen_path));
    // concatenates str1 and str2
   // the resultant string is stored in str1.
    strcat(info_chosen_path, USB[rand() % 2]);
    strcat(info_chosen_path, file_path);

    sprintf(permisson, "%s%s%s", !access(info_chosen_path, R_OK) ? "read " : "",
            !access(info_chosen_path, W_OK) ? "write " : "",
            !access(info_chosen_path, X_OK) ? "execute " : "");

    // The stat() function gets status information about a specified file and places it in the area of memory pointed to by the buf argument.
    struct stat sta;
    stat(info_chosen_path, &sta);


    sprintf(server_message,
            "User ID of the file's owner: %d\n"
            "Group ID of the file's group:%d\n"
            "Time of last modification: %s"
            "Size of file: %ld bytes\n"
            "Permisson: %s\n",
            sta.st_uid, sta.st_gid, ctime(&sta.st_mtim.tv_sec), sta.st_size,
            permisson);
}

// PUT
void put_file(char const* file_path, char* put_content, char* server_message) {
    char* file_content;
    char usb_file_path[strlen(file_path) + strlen(USB[0]) + 10];
    for (int i = 0; i < 2; i++) {
        memset(usb_file_path, '\0', sizeof(usb_file_path));
        strcat(usb_file_path, USB[i]);
        strcat(usb_file_path, file_path);
        printf("%s\n", usb_file_path);
        FILE* pfile = fopen(usb_file_path, "wb+");
        if (pfile == NULL) {
            strcpy(server_message, "open file failed!\n");
            return;
        }
        int content_length = strlen(put_content);

        int write_length = fwrite(put_content, 1, content_length, pfile);
        if (write_length != content_length) {
            sprintf(server_message, "write file %s failed!\n", usb_file_path);
            return;
        }

        fclose(pfile);
        pfile = NULL;
    }
    
    sprintf(server_message, "write file %s Done!\n", file_path);
}

// MD
void create_dir(char const* folder_name, char* server_message) {
    char usb_folder_name[strlen(folder_name) + strlen(USB[0]) + 10];
    
    for (int i = 0; i < 2; i++) {
        memset(usb_folder_name, '\0', sizeof(usb_folder_name));
        strcat(usb_folder_name, USB[i]);
        strcat(usb_folder_name, folder_name);
        if (!access(usb_folder_name, F_OK)) {
            sprintf(server_message + strlen(server_message), "this folder %s exist!\n", folder_name);
            return;
        }

        mkdir(usb_folder_name, 0777);
    }
    
    sprintf(server_message + strlen(server_message), "create folder %s done!\n", folder_name);
}

// RM
void remove_file(char const* path, char* server_message) {

    char removed_file_path[strlen(path) + strlen(USB[0]) + 10];

    for (int i = 0; i < 2; i++) {
        memset(removed_file_path, '\0', sizeof(removed_file_path));
        strcat(removed_file_path, USB[i]);
        strcat(removed_file_path, path);

        if (access(removed_file_path, F_OK)) {
            sprintf(server_message, "%s not exists!\n", path);
            return;
        }

        remove(removed_file_path);
    }

    
    sprintf(server_message, "remove %s done!\n", path);
}

