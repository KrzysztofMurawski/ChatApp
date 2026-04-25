#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>

#include <pthread.h>


#define PORT 8000
#define MAX_ACTIVE_USERS 5

struct client {
    int fd;
    char username[24];
};

pthread_mutex_t mutex;
int active_clients_fds_pt[MAX_ACTIVE_USERS]; 
struct client active_clients[MAX_ACTIVE_USERS];
int client_cnt = 0;


void * handle_client_communication(void * sock_fd);
int close_client_socket(int socket_fd);



int main(){
    pthread_mutex_init(&mutex, NULL);

    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1) return 10;


    struct sockaddr_in * server_address_pt = malloc(sizeof(struct sockaddr_in));
    
    server_address_pt->sin_family = AF_INET;
    server_address_pt->sin_port = htons(PORT);
    server_address_pt->sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int res_bind = bind(server_socket_fd, 
        (struct sockaddr *) server_address_pt,
        sizeof(struct sockaddr));
    if (res_bind == -1) return 11;


    int res_listen = listen(server_socket_fd, MAX_ACTIVE_USERS );
    if (res_listen == -1) return 12;


    while (true){

        struct sockaddr * client_address_pt = malloc(sizeof(struct sockaddr));
        socklen_t * client_address_len_pt = malloc(sizeof(socklen_t));

        int client_socket_fd = accept(server_socket_fd, 
            (struct sockaddr *)client_address_pt, 
            client_address_len_pt);
        if (client_socket_fd < 0) return 13;

        
        pthread_t communication_thread_id;
        int res_threading = pthread_create(&communication_thread_id, NULL, 
            handle_client_communication, &client_socket_fd);        
        if (res_threading == -1) return 14;
    }


    int res_shutdown = shutdown(server_socket_fd, SHUT_RDWR);
    if (res_shutdown == -1) return 19;

    pthread_mutex_destroy(&mutex);

    return 0;
}


void * handle_client_communication(void * sock_fd){
    int client_socket_fd = *(int *)sock_fd;

    // Get client's username
    char username[24] = "Tempname";

    char username_buffer[1024] = {0};
    ssize_t res_recv = recv(client_socket_fd, &username_buffer, sizeof(username_buffer), 0);
    if (res_recv <= 0) {
        close_client_socket(client_socket_fd);
        return NULL;
    } else {
        strcpy(username, username_buffer);
    }

    
    // Update active clients list
    pthread_mutex_lock(&mutex);
    
    active_clients[client_cnt].fd = client_socket_fd;
    
    snprintf(active_clients[client_cnt].username, sizeof(username), username);
    client_cnt++;
    pthread_mutex_unlock(&mutex);

    // Send welcome message

    printf("New client on socket: %d\n", client_socket_fd);
    char welcome_msg[] = "Welcome to server!";
    int res_send = send(client_socket_fd, &welcome_msg, sizeof(welcome_msg), 0);
        if (res_send <= 0) {
            close_client_socket(client_socket_fd);
            return NULL;
        }


    while (true){
        
        // Receive message

        char recv_buffer[1024] = {0};
        ssize_t res_recv = recv(client_socket_fd, &recv_buffer, sizeof(recv_buffer), 0);
        if (res_recv <= 0) {
            close_client_socket(client_socket_fd);
            return NULL;
        }
        else {
            int i = 0;
            while (i < client_cnt && active_clients[i].fd != client_socket_fd) ++i;
            printf("Received from client %s: %d %s\n", active_clients[i].username, res_recv, recv_buffer);
        }
        // Broadcast message

        for (int i = 0; i < client_cnt; i++){
            if (active_clients[i].fd != client_socket_fd){

                int res_send = send(active_clients[i].fd, &recv_buffer, res_recv, 0);
                if (res_send <= 0) {
                    close_client_socket(client_socket_fd);
                    return NULL;
                }
            }
        }
    }
}


int close_client_socket(int socket_fd){
    printf("Closing socket %d\n", socket_fd);
    for (int i = 0; i < MAX_ACTIVE_USERS; i++){
        if (active_clients[i].fd == socket_fd){

            pthread_mutex_lock(&mutex);

            for (int j = i; j < MAX_ACTIVE_USERS-1; j++){
                active_clients[j] = active_clients[j+1];
            }
            client_cnt--;

            pthread_mutex_unlock(&mutex);

            shutdown(socket_fd, SHUT_RDWR);
            break;
        }
    }
}