#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 5555
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    int bytes;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // ✅ Use inet_addr (fully compatible with MinGW)
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid address\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Connection failed\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("[Connected to server]\n");

    // Communication loop
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0)
            break;

        buffer[bytes] = '\0';
        printf("%s", buffer);

        // Detect prompts that need user input
        if (strstr(buffer, "Your answer:") || strstr(buffer, "Enter your name")) {
            fgets(input, sizeof(input), stdin);
            send(sock, input, strlen(input), 0);
        }
    }

    printf("\n[Disconnected from server]\n");
    closesocket(sock);
    WSACleanup();
    return 0;
}
