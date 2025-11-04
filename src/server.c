#include "server.h"

socket_type server_setup(const char *ip, int port) {
    socket_type server_fd;
    struct sockaddr_in address;
    int opt = 1;

    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
            fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
            exit(EXIT_FAILURE);
        }
    #endif

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        #ifdef _WIN32
            fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
            WSACleanup();
        #else
            perror("Socket creation failed");
        #endif
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *)&opt, sizeof(opt)) == SOCKET_ERROR) {
        #ifdef _WIN32
            fprintf(stderr, "Set socket options failed: %d\n", WSAGetLastError());
            closesocket(server_fd);
            WSACleanup();
        #else
            perror("Set socket options failed");
            close(server_fd);
        #endif
        exit(EXIT_FAILURE);
    }

    // Setup address (IPv4)
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        #ifdef _WIN32
            fprintf(stderr, "Binding failed: %d\n", WSAGetLastError());
            closesocket(server_fd);
            WSACleanup();
        #else
            perror("Binding failed");
            close(server_fd);
        #endif
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) == SOCKET_ERROR) {
        #ifdef _WIN32
            fprintf(stderr, "Listening failed: %d\n", WSAGetLastError());
            closesocket(server_fd);
            WSACleanup();
        #else
            perror("Listening failed");
            close(server_fd);
        #endif
        exit(EXIT_FAILURE);
    }
    printf("Server listening on %s:%d\n", ip, port);
    return server_fd;
}

ssize_t read_sck(int fd, void *buf, size_t len) {
    #ifdef _WIN32
        return recv((socket_type)fd, (char *)buf, (int)len, 0);
    #else
        return read(fd, buf, len);
    #endif
}

void cleanup_socket(socket_type sock) {
    #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
    #else
        close(sock);
    #endif
}