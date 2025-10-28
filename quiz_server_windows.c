#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 5555
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Quiz structure
struct Quiz {
    char question[256];
    char answer[100];
};

// Leaderboard structure
struct Leaderboard {
    char name[100];
    int score;
};

// Quiz data
struct Quiz quiz[] = {
    {"What is the capital of France?", "paris"},
    {"how many colors in rainbow?", "seven"},
    {"What is 5 + 7?", "12"},
    {"what is full form of URL", "Uniform Resource Locator"},
    {"What is port number of http?", "80"}
};

int num_questions = sizeof(quiz) / sizeof(quiz[0]);
struct Leaderboard leaderboard[MAX_CLIENTS];
int leaderboard_count = 0;
HANDLE leaderboard_lock;

// Send message to client
void send_msg(SOCKET sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

// Thread function for each client
DWORD WINAPI handle_client(LPVOID client_socket) {
    SOCKET sock = *(SOCKET *)client_socket;
    free(client_socket);

    char buffer[BUFFER_SIZE];
    char name[100];
    int score = 0;

    send_msg(sock, "Enter your name: ");
    int len = recv(sock, name, sizeof(name), 0);
    if (len <= 0) {
        closesocket(sock);
        return 0;
    }
    name[strcspn(name, "\r\n")] = 0;

    char welcome[200];
    snprintf(welcome, sizeof(welcome), "\nWelcome %s! Let's start the quiz.\n", name);
    send_msg(sock, welcome);

    for (int i = 0; i < num_questions; i++) {
        char question_msg[512];
        snprintf(question_msg, sizeof(question_msg), "\nQ%d: %s\nYour answer: ", i + 1, quiz[i].question);
        send_msg(sock, question_msg);

        memset(buffer, 0, sizeof(buffer));
        recv(sock, buffer, sizeof(buffer), 0);
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (_stricmp(buffer, quiz[i].answer) == 0) {
            score++;
            send_msg(sock, " Correct!\n");
        } else {
            char wrong[200];
            snprintf(wrong, sizeof(wrong), " Wrong! Correct answer: %s\n", quiz[i].answer);
            send_msg(sock, wrong);
        }
    }

    WaitForSingleObject(leaderboard_lock, INFINITE);
    strcpy(leaderboard[leaderboard_count].name, name);
    leaderboard[leaderboard_count].score = score;
    leaderboard_count++;
    ReleaseMutex(leaderboard_lock);

    char result[200];
    snprintf(result, sizeof(result), "\nQuiz Over! Your score: %d/%d\n", score, num_questions);
    send_msg(sock, result);

    send_msg(sock, "\n Leaderboard \n");
    WaitForSingleObject(leaderboard_lock, INFINITE);
    for (int i = 0; i < leaderboard_count; i++) {
        char entry[200];
        snprintf(entry, sizeof(entry), "%s: %d\n", leaderboard[i].name, leaderboard[i].score);
        send_msg(sock, entry);
    }
    ReleaseMutex(leaderboard_lock);

    closesocket(sock);
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    WSAStartup(MAKEWORD(2, 2), &wsa);

    leaderboard_lock = CreateMutex(NULL, FALSE, NULL);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 5) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("[SERVER STARTED] Listening on port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET) {
            printf("Accept failed\n");
            continue;
        }

        printf("[NEW CONNECTION] Client connected.\n");

        SOCKET *client_sock = malloc(sizeof(SOCKET));
        *client_sock = new_socket;

        CreateThread(NULL, 0, handle_client, client_sock, 0, NULL);
    }

    CloseHandle(leaderboard_lock);
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
