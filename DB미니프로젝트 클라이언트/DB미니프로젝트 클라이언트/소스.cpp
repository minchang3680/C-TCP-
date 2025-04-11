#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <io.h>
#include <fcntl.h>

#pragma comment(lib, "ws2_32.lib")

// UTF-16 → UTF-8
std::string toUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return "";
    std::string strTo(size_needed - 1, 0); // -1 to remove null terminator
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

// UTF-8 → UTF-16
std::wstring fromUTF8(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size_needed <= 0) return L"";
    std::wstring wstrTo(size_needed - 1, 0); // -1 to remove null terminator
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstrTo[0], size_needed);
    return wstrTo;
}

void showMainMenu() {
    std::wcout << L"\n==== 메인 메뉴 ====\n";
    std::wcout << L"1. 회원가입\n2. 로그인\n3. 종료\n> ";
}

void showUserMenu() {
    std::wcout << L"\n==== 유저 메뉴 ====\n";
    std::wcout << L"1. 채팅하기\n2. 로그아웃\n3. 종료\n> ";
}

bool connectToServer(SOCKET& sock) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return false;
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(12345);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    return connect(sock, (sockaddr*)&server, sizeof(server)) != SOCKET_ERROR;
}

int main() {
    // 콘솔 UTF-8 모드
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::wcerr << L"WSAStartup 실패\n";
        return 1;
    }

    bool running = true;
    bool loggedIn = false;
    std::string currentUsername;

    while (running) {
        if (!loggedIn) {
            showMainMenu();
            int choice;
            std::wcin >> choice;
            std::wcin.ignore();

            switch (choice) {
            case 1: {
                std::wstring wuser, wpass;
                std::wcout << L"사용자 이름: ";
                std::getline(std::wcin, wuser);
                std::wcout << L"비밀번호: ";
                std::getline(std::wcin, wpass);

                std::string user = toUTF8(wuser);
                std::string pass = toUTF8(wpass);
                std::string msg = "SIGNUP:" + user + ":" + pass;

                SOCKET sock;
                if (!connectToServer(sock)) {
                    std::wcerr << L"서버 연결 실패\n"; break;
                }

                send(sock, msg.c_str(), msg.size(), 0);
                char buffer[1024] = {};
                int len = recv(sock, buffer, sizeof(buffer), 0);
                if (len > 0) std::wcout << L"[서버 응답] " << fromUTF8(std::string(buffer, len)) << L"\n";
                closesocket(sock);
                break;
            }
            case 2: {
                std::wstring wuser, wpass;
                std::wcout << L"사용자 이름: ";
                std::getline(std::wcin, wuser);
                std::wcout << L"비밀번호: ";
                std::getline(std::wcin, wpass);

                std::string user = toUTF8(wuser);
                std::string pass = toUTF8(wpass);
                std::string msg = "LOGIN:" + user + ":" + pass;

                SOCKET sock;
                if (!connectToServer(sock)) {
                    std::wcerr << L"서버 연결 실패\n"; break;
                }

                send(sock, msg.c_str(), msg.size(), 0);
                char buffer[1024] = {};
                int len = recv(sock, buffer, sizeof(buffer), 0);
                if (len > 0) {
                    std::string resp(buffer, len);
                    std::wstring wresp = fromUTF8(resp);
                    std::wcout << L"[서버 응답] " << wresp << L"\n";
                    if (resp == "Login Success") {
                        loggedIn = true;
                        currentUsername = user;
                    }
                }
                closesocket(sock);
                break;
            }
            case 3:
                running = false;
                break;
            default:
                std::wcout << L"올바른 번호를 선택하세요.\n";
            }
        }
        else {
            showUserMenu();
            int choice;
            std::wcin >> choice;
            std::wcin.ignore();

            switch (choice) {
            case 1: {
                std::wcout << L"[채팅 시작] 메시지를 입력하세요. ('exit' 입력 시 종료)\n";
                while (true) {
                    std::wstring wmsg;
                    std::wcout << L">> ";
                    std::getline(std::wcin, wmsg);

                    if (wmsg == L"exit") {
                        std::wcout << L"[채팅 종료] 유저 메뉴로 돌아갑니다.\n";
                        break;
                    }

                    std::string utf8msg = toUTF8(wmsg);
                    std::string sendmsg = "CHAT:" + currentUsername + ":" + utf8msg;

                    SOCKET sock;
                    if (!connectToServer(sock)) {
                        std::wcerr << L"서버 연결 실패\n"; break;
                    }

                    send(sock, sendmsg.c_str(), sendmsg.size(), 0);
                    char buffer[1024] = {};
                    int len = recv(sock, buffer, sizeof(buffer), 0);
                    if (len > 0) {
                        std::wstring reply = fromUTF8(std::string(buffer, len));
                        std::wcout << L"[서버 응답] " << reply << L"\n";
                    }
                    closesocket(sock);
                }
                break;
            }
            case 2:
                std::wcout << fromUTF8(currentUsername) << L" 로그아웃 되었습니다.\n";
                currentUsername = "";
                loggedIn = false;
                break;
            case 3:
                running = false;
                break;
            default:
                std::wcout << L"올바른 번호를 선택하세요.\n";
            }
        }
    }

    WSACleanup();
    return 0;
}
