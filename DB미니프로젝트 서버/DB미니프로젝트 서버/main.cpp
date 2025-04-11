#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <io.h>
#include <fcntl.h>

#include <mysql/jdbc.h>
#pragma comment(lib, "ws2_32.lib")

const std::string DB_HOST = "tcp://127.0.0.1:3306";
const std::string DB_USER = "root";

std::string g_dbPass;
std::string g_dbName;

std::wstring utf8ToWstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

void handleClient(SOCKET clientSocket) {
    std::vector<char> buffer(1024);
    int recvLen = recv(clientSocket, buffer.data(), buffer.size(), 0);
    if (recvLen <= 0) { closesocket(clientSocket); return; }

    std::string msg(buffer.data(), recvLen);
    std::wcout << L"[RECV] " << utf8ToWstring(msg) << L"\n";

    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect(DB_HOST, DB_USER, g_dbPass));
        conn->setSchema(g_dbName);

        conn->setClientOption("characterEncoding", "utf8mb4");
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        stmt->execute("SET NAMES utf8mb4");

        if (msg.rfind("LOGIN:", 0) == 0) {
            size_t pos1 = msg.find(':', 6);
            std::string username = msg.substr(6, pos1 - 6);
            std::string password = msg.substr(pos1 + 1);

            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->prepareStatement("SELECT 1 FROM users WHERE username=? AND password=?"));
            pstmt->setString(1, username);
            pstmt->setString(2, password);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

            std::string resp = res->next() ? "Login Success" : "Login Failed";
            send(clientSocket, resp.c_str(), resp.length(), 0);
        }
        else if (msg.rfind("SIGNUP:", 0) == 0) {
            size_t pos1 = msg.find(':', 7);
            std::string username = msg.substr(7, pos1 - 7);
            std::string password = msg.substr(pos1 + 1);

            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->prepareStatement("INSERT INTO users (username, password) VALUES (?, ?)"));
            pstmt->setString(1, username);
            pstmt->setString(2, password);
            pstmt->execute();

            std::string resp = "Signup Success";
            send(clientSocket, resp.c_str(), resp.length(), 0);
        }
        else if (msg.rfind("CHAT:", 0) == 0) {
            size_t pos1 = msg.find(':', 5);
            std::string username = msg.substr(5, pos1 - 5);
            std::string message = msg.substr(pos1 + 1);

            std::wcout << L"[CHAT] " << utf8ToWstring(username) << L": " << utf8ToWstring(message) << L"\n";

            std::unique_ptr<sql::PreparedStatement> findStmt(
                conn->prepareStatement("SELECT user_id FROM users WHERE username=?"));
            findStmt->setString(1, username);
            std::unique_ptr<sql::ResultSet> userRes(findStmt->executeQuery());

            if (userRes->next()) {
                int userId = userRes->getInt("user_id");

                std::unique_ptr<sql::PreparedStatement> chatStmt(
                    conn->prepareStatement("INSERT INTO message_log (sender_id, content) VALUES (?, ?)"));
                chatStmt->setInt(1, userId);
                chatStmt->setString(2, message);
                chatStmt->execute();

                std::unique_ptr<sql::PreparedStatement> timeStmt(
                    conn->prepareStatement("SELECT sent_at FROM message_log WHERE sender_id=? ORDER BY message_id DESC LIMIT 1"));
                timeStmt->setInt(1, userId);
                std::unique_ptr<sql::ResultSet> timeRes(timeStmt->executeQuery());

                std::string response = message;
                if (timeRes->next()) {
                    std::string timestamp = timeRes->getString("sent_at");
                    response += " (" + timestamp + ")";
                }

                send(clientSocket, response.c_str(), response.length(), 0);
            }
            else {
                std::string resp = "User Not Found";
                send(clientSocket, resp.c_str(), resp.length(), 0);
            }
        }
        else {
            std::string err = "Unknown Command";
            send(clientSocket, err.c_str(), err.length(), 0);
        }
    }
    catch (sql::SQLException& e) {
        std::wcerr << L"MySQL Error: " << utf8ToWstring(e.what()) << L"\n";
        std::string err = "DB Error";
        send(clientSocket, err.c_str(), err.length(), 0);
    }

    closesocket(clientSocket);
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U8TEXT);

    std::wstring wpass, wdb;
    std::wcout << L"[Server start]\n";
    std::wcout << L"Password : ";
    std::getline(std::wcin, wpass);
    std::wcout << L"DB Name  : ";
    std::getline(std::wcin, wdb);

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    g_dbPass = conv.to_bytes(wpass);
    g_dbName = conv.to_bytes(wdb);

    std::wcout << L"[DB Connect]\n";

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::wcerr << L"WSAStartup failed\n"; return 1;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::wcerr << L"Socket failed\n"; return 1;
    }

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(12345);
    serv.sin_addr.s_addr = INADDR_ANY;
    bind(serverSock, (sockaddr*)&serv, sizeof(serv));
    listen(serverSock, SOMAXCONN);

    // ✅ 삭제된 메시지 ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
    // std::wcout << L"Server Listen in 12345\n";

    while (true) {
        SOCKET cli = accept(serverSock, nullptr, nullptr);
        if (cli != INVALID_SOCKET) handleClient(cli);
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
