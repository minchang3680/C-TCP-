#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

void showMainMenu() {
    std::cout << "\n==== ���� �޴� ====\n";
    std::cout << "1. ȸ������\n";
    std::cout << "2. �α���\n";
    std::cout << "3. ����\n> ";
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup Failed\n"; return 1;
    }

    while (true) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n"; return 1;
        }

        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port = htons(12345);
        server.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            std::cerr << "���� ���� ����\n"; return 1;
        }

        int choice = 0;
        showMainMenu();
        std::cin >> choice;
        std::cin.ignore(); // ���� ����

        switch (choice) {
        case 1: { // ȸ������
            std::string username, password;
            std::cout << "����� �̸�: "; std::getline(std::cin, username);
            std::cout << "��й�ȣ: "; std::getline(std::cin, password);

            std::string msg = "SIGNUP:" + username + ":" + password;
            send(sock, msg.c_str(), msg.length(), 0);

            char buffer[1024] = {};
            int recvLen = recv(sock, buffer, sizeof(buffer), 0);
            if (recvLen > 0) {
                std::cout << "[���� ����] " << std::string(buffer, recvLen) << "\n";
            }
            break;
        }

        case 2: {
            std::cout << "�α��� ����� ���� �ܰ迡�� �����˴ϴ�.\n";
            break;
        }

        case 3:
            std::cout << "���α׷��� �����մϴ�.\n";
            closesocket(sock);
            WSACleanup();
            return 0;

        default:
            std::cout << "�ùٸ� ��ȣ�� �����ϼ���.\n";
            break;
        }

        closesocket(sock);
    }

    WSACleanup();
    return 0;
}
