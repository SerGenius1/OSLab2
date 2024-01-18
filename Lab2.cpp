#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_CLIENTS 10


volatile wasSigHup = 0;

void sigHupHandler(int r)
{
    wasSigHup = 1;
}

int main() {
    struct sockaddr_in server_addr;
    int socketTemp, master_socket, socketClient[MAX_CLIENTS];
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        socketClient[i] = -1;
    }

    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket == 0) {
        perror("error_in_socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(1234);


    int err;

    err = bind(master_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)); 
    

    if (err == -1) {
        perror("error_in_bind");
        exit(EXIT_FAILURE);
    }

    err = listen(master_socket, MAX_CLIENTS);  
    if (err == -1) {
        perror("error_in_listen");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);

    sa.sa_handler = sigHupHandler;

    sa.sa_flags |= SA_RESTART;
                               

    sigaction(SIGHUP, &sa, NULL);

    sigset_t blockedMask, origMask; 
    sigemptyset(&blockedMask);     
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    fd_set fds; 

    while (1) { 
        FD_ZERO(&fds);
        FD_SET(master_socket, &fds);

        



        int max_descr = master_socket;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (socketClient[i] != -1) {
                FD_SET(socketClient[i], &fds); 
                if (socketClient[i] > max_descr) {
                    max_descr = socketClient[i];
                }
            }
        }

        err = pselect(max_descr + 1, &fds, NULL, NULL, NULL, &origMask); 
        
        if (err < 0) {
            if (errno == EINTR) {
                printf("SIGHUB");
                break;
            }
            else {
                perror("error_in_pselect");
                break;
            }
        }

        if (FD_ISSET(master_socket, &fds)) {
            if ((socketTemp = accept(master_socket, NULL, NULL)) < 0) {
                perror("error_accept");
                exit(EXIT_FAILURE);
            }


            for (i = 0; socketClient[i] != -1 && i < MAX_CLIENTS; i++);
            if (i < MAX_CLIENTS) {
                printf("Новое соединение\n");
                socketClient[i] = socketTemp;
            }
            else {
                close(socketTemp);
            }


        }
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (socketClient[i] != -1 && FD_ISSET(socketClient[i], &fds)) {
                char buffer[1024];
                ssize_t bytes_received = recv(socketClient[i], buffer, sizeof(buffer), 0);

                if (bytes_received > 0) {
                    printf("Сообщение от клиента: %s\n", buffer);
                }
                else if (bytes_received == 0) {
                    printf("Соединение разорвано\n");
                    close(socketClient[i]);
                    socketClient[i] = -1;
                }
                else {
                    perror("error_recv");
                }
            }
        }
    }

    close(master_socket);
    for (i = 0; i < MAX_CLIENTS; i++) {
        close(socketClient[i]);
    }

    return 0;
}
