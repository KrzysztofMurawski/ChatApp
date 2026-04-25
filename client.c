#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <signal.h>


#define SERVER_IP_ADDRESS "127.0.0.1"
#define PORT 8000

struct client {
    int fd;
    char username[24];
};

void * handle_sending_messages(void *args);
void close_connection(int sock_fd);


int main(int argc, char *argv[]){

    char server_ip[] = SERVER_IP_ADDRESS;

    int client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_fd == -1) return 10;


    // Save name and fd to a client stuct
    struct client *client_info = malloc(sizeof(struct client));
    client_info->fd = client_socket_fd;
    printf("cmd argument : %s\n", argv[1]);
    if (argc > 1) strcpy(client_info->username, argv[1]);
    else strcpy(client_info->username, "No_name");


    struct sockaddr_in * server_address_pt = malloc(sizeof(struct sockaddr_in));
    server_address_pt->sin_family = AF_INET;
    server_address_pt->sin_port = htons(PORT);
    inet_pton(AF_INET, server_ip, &server_address_pt->sin_addr.s_addr);


    int res_connect = connect(client_socket_fd, 
        (struct sockaddr *) server_address_pt, 
        sizeof(*server_address_pt));
    if (res_connect == -1) return 11;


    pthread_t thread_id;
    pthread_create(&thread_id, NULL, handle_sending_messages, client_info);
    
    // receiving loop
    bool server_active = true;
    while (server_active){
        char recv_buffer[1024] = {0};   
        int res_recv = recv(client_socket_fd, &recv_buffer, sizeof(recv_buffer), 0);
        if (res_recv <= 0) {
            server_active = false;
        }
        else printf("Received: %d %s\n", res_recv, recv_buffer);
    }
    
    pthread_cancel(thread_id);
    close_connection(client_socket_fd);
    return 0;
}


void * handle_sending_messages(void *args){
    struct client *client_info = args;

    printf("Usrname: %s\n", client_info->username);
    int res_send = send(client_info->fd, &client_info->username, sizeof(client_info->username), 0); 
    if (res_send <= 0) {
        close_connection(client_info->fd);
        return NULL;
    }

    char *line = NULL;
    size_t line_size = 0;

    while (true){
        ssize_t char_count = getline(&line, &line_size, stdin);
        printf("You entered: %d %s", char_count, line);
    
        int res_send = send(client_info->fd, line, char_count, 0); 
        if (res_send <= 0) {
            close_connection(client_info->fd);
            return NULL;
        }
    }
}



void close_connection(int sock_fd){
    printf("Closing connection...");
    int shut_res = shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);
    if (shut_res > -1) printf("Connection closed successfully...");
    return;
}