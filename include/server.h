#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
    typedef SOCKET socket_type;
    #define CLOSESOCKET closesocket
    #define THREAD_TYPE HANDLE
    #define THREAD_FUNC DWORD WINAPI
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <pthread.h>
    typedef int socket_type;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR   (-1)
    #define THREAD_TYPE pthread_t
    #define THREAD_FUNC void *
    #define CLOSESOCKET close
#endif

socket_type server_setup(const char *ip, int port);

ssize_t read_sck(int fd, void *buf, size_t len);

void cleanup_socket(socket_type sock);

#endif