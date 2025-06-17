#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_CLIENTS 10  //max number of client req served at a time


typedef struct cache_element cache_element;

struct cache_element{
    char* data; //data stores response
    int len;//length of data i.e sizeof(data)
    char* url; //url stores the request
    time_t lru_time_track;//lru_time_track stores the latest time the element is accesed
    cache_element* next;//pointer to next element
};

cache_element* find(char* url);
int add_cache_element(char* data, int size, char* url);
void remove_cache_element();

int port_number = 8080;// Default Port
int proxy_socketId;// socket descriptor of proxy server
pthread_t tid[MAX_CLIENTS];//array to store the thread ids of clients

sem_t semaphore; //if client requests exceeds the max_clients this seamaphore puts the waiting threads to sleep and wakes them when traffic on queue decreases

//sem_t cache_lock
pthread_mutex_t lock;//lock is used for locking the cache

cache_element* head;//head of the cache linkedlist
int cache_size;//size of the cache

int main(int argc, char* argv[]){
    int client_socketId, client_len;
    struct sockaddr_in server_addr, client_addr;
    sem_init(&semaphore, 0, MAX_CLIENTS); //initialize semaphore with max clients
    pthread_mutex_init(&lock, NULL); //initialize mutex lock
    if(argv == 2){
        port_number = atoi(argv[1]); //if port number is given as argument, set it
    }else{
        printf("Too few arguments\n");
        exit(1);
    }

    printf("Starting Proxy Server on port %d\n", port_number);
    // Create a socket
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_socketId < 0){
        perror("Error creating socket\n");
        exit(1);
    }
    int reuse = 1; // Enables the reuse of local addresses for the socket
    if(setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        perror("Error setting socket options - setsockopt failed\n");
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET; //set address family to IPv4
    server_addr.sin_port = htons(port_number); //set port number
    server_addr.sin_addr.s_addr = INADDR_ANY; //bind to any address

    if(bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Port is not available\n");
        exit(1);
    }
    printf("Binding on port %d\n", port_number);

    int listen_status = listen(proxy_socketId, MAX_CLIENTS);
    if(listen_status < 0){
        perror("Error listening on socket\n");
        exit(1);
    }

    int i = 0;
    int Connected_socketId[MAX_CLIENTS];

    while(1){
        bzero((char*)&client_addr, sizeof(client_addr));
        client_len = sizeof(client_addr);
        client_socketId = accept(proxy_socketId, (struct sockaddr*)&client_addr, (socklen_t*)&client_len);
        if(client_socketId < 0){
            perror("Not able to connect\n");
            exit(1);
        }
        else{
            Connected_socketId[i] = client_socketId;
        }

        struct sockaddr_in *client_ptr = (struct sockaddr_in*)&client_addr;
        struct in_addr ip_addr = client_ptr->sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN);
        printf("Client is connected with port number %d and IP address is %s\n", ntohs(client_addr.sin_port), str);
        pthread_create(&tid[i], NULL, thread_fn, (void*)&Connected_socketId[i]);
        i++;
    }
    close(proxy_socketId);
    return 0;

}