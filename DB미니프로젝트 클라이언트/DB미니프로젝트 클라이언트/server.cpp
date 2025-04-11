#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

void showMainMenu() {
    std::cout << "\n==== 메인 메뉴 ====\n";
    std::cout << "1. 회원가입\n";
    std::cout << "2. 로그인\n";
    std::cout << "3. 종료\n> ";
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
            std::cerr << "서버 연결 실패\n"; return 1;
        }

        int choice = 0;
        showMainMenu();
        std::cin >> choice;
        std::cin.ignore(); // 개행 제거

        switch (choice) {
        case 1: { // 회원가입
            std::string username, password;
            std::cout << "사용자 이름: "; std::getline(std::cin, username);
            std::cout << "비밀번호: "; std::getline(std::cin, password);

            std::string msg = "SIGNUP:" + username + ":" + password;
            send(sock, msg.c_str(), msg.length(), 0);

            char buffer[1024] = {};
            int recvLen = recv(sock, buffer, sizeof(buffer), 0);
            if (recvLen > 0) {
                std::cout << "[서버 응답] " << std::string(buffer, recvLen) << "\n";
            }
            break;
        }

        case 2: {
            std::cout << "로그인 기능은 다음 단계에서 구현됩니다.\n";
            break;
        }

        case 3:
            std::cout << "프로그램을 종료합니다.\n";
            closesocket(sock);
            WSACleanup();
            return 0;

        default:
            std::cout << "올바른 번호를 선택하세요.\n";
            break;
        }

        closesocket(sock);
    }

    WSACleanup();
    return 0;
}
