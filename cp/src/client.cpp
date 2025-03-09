
#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <chrono>

#include "myMQ.h"

int PORT_ITER = 0; 

static zmq::socket_t* gPlayerSocket = nullptr;

void gameLoop() {
    while (true) {
        std::string msg = receive_message(*gPlayerSocket);
        if (msg.empty()) {
            std::cout << "[Client] Пустое сообщение, завершаем игровой режим.\n";
            return; 
        }

        if (msg == "your_turn") {
            send_message(*gPlayerSocket, "ok");
        }
        else if (msg == "not_your_turn") {
            send_message(*gPlayerSocket, "ok");
        }
        else if (msg == "shoot") {
            int x, y;
            std::cout << "[Client] Ваш ход! Введите x y: ";
            std::cin >> x >> y;
            std::stringstream ss;
            ss << "coords:" << x << ":" << y;
            send_message(*gPlayerSocket, ss.str());
        }
        else if (msg == "shooted") {
            std::cout << "[Client] Попадание!\n";
            send_message(*gPlayerSocket, "ok");
        }
        else if (msg == "miss") {
            std::cout << "[Client] Промах!\n";
            send_message(*gPlayerSocket, "ok");
        }
        else if (msg.rfind("board", 0) == 0) {
            std::string boardStr = msg.substr(5);
            std::cout << "[Client] Текущее поле:\n" << boardStr << std::endl;
            send_message(*gPlayerSocket, "ok");
        }
        else if (msg == "win") {
            std::cout << "[Client] Вы выиграли!\n";
            send_message(*gPlayerSocket, "ok");
            return;
        }
        else if (msg == "lose") {
            std::cout << "[Client] Вы проиграли!\n";
            send_message(*gPlayerSocket, "ok");
            return;
        }
        else if (msg.rfind("Введите ориентацию", 0) == 0) {
            std::cout << msg << std::endl;
            std::string orientation;
            std::cin >> orientation;
            send_message(*gPlayerSocket, orientation);
        }
        else if (msg.rfind("Разместите", 0) == 0) {
            std::cout << msg << std::endl;
            int x, y;
            std::cin >> x >> y;
            std::stringstream ss;
            ss << "coords:" << x << ":" << y;
            send_message(*gPlayerSocket, ss.str());
        }
        else if (msg.rfind("Error", 0) == 0) {
            std::cout << "[Client] Ошибка: " << msg << std::endl;
            send_message(*gPlayerSocket, "ok");
        }
        else {
            std::cout << "[Client] Неизвестное сообщение: " << msg << std::endl;
        }
    }
}

int main() {
    zmq::context_t context(2);

    zmq::socket_t main_socket(context, ZMQ_REQ);
    main_socket.connect(GetConPort(5555)); 

    std::cout << "Добро пожаловать в Морской бой!\n";

    // Авторизация
    std::string login;
    while (true) {
        std::cout << "Введите ваш логин: ";
        std::cin >> login;
        pid_t pid = getpid();

        std::stringstream ss;
        ss << "login:" << login << ":" << pid;
        send_message(main_socket, ss.str());
        std::string resp = receive_message(main_socket);

        if (resp.rfind("Ok:", 0) == 0) {
            auto colPos = resp.find(':');
            int socketIndex = std::stoi(resp.substr(colPos+1));
            gPlayerSocket = new zmq::socket_t(context, ZMQ_REP);
            gPlayerSocket->connect(GetConPort(5556 + socketIndex));

            std::cout << "[Client] Авторизация прошла успешно. Игровой порт: "
                      << (5556 + socketIndex) << std::endl;
            break;
        } 
        else if (resp.find("Error:NameAlreadyExist") == 0) {
            std::cout << "Ошибка: логин уже существует, попробуйте другой.\n";
        } 
        else if (resp.find("Error:NoFreeSockets") == 0) {
            std::cout << "Ошибка: на сервере нет свободных слотов.\n";
            return 0;
        } 
        else {
            std::cout << "Ошибка: " << resp << std::endl;
        }
    }

    // Цикл команд
    while (true) {
        std::cout << "\n=========================================\n"
                  << "Доступные команды:\n"
                  << "  create <имя_игры>  - создать комнату\n"
                  << "  join <имя_игры>    - присоединиться к комнате\n"
                  << "  stats              - показать свою статистику\n"
                  << "  exit               - выйти\n"
                  << "=========================================\n"
                  << "Введите команду: ";

        std::string command;
        std::cin >> command;
        if (command == "create") {
            std::string roomName;
            std::cin >> roomName;
            std::stringstream ss;
            ss << "create:" << login << ":" << roomName;
            send_message(main_socket, ss.str());
            std::string resp = receive_message(main_socket);
            std::cout << "[Client] Сервер ответил: " << resp << std::endl;

            if (resp.rfind("Ok:RoomCreated", 0) == 0) {
                std::cout << "[Client] Ожидаем второго игрока...\n";
                gameLoop(); 
            }
        }
        else if (command == "join") {
            std::string roomName;
            std::cin >> roomName;
            std::stringstream ss;
            ss << "join:" << login << ":" << roomName;
            send_message(main_socket, ss.str());
            std::string resp = receive_message(main_socket);
            std::cout << "[Client] Сервер ответил: " << resp << std::endl;

            if (resp.rfind("Ok:RoomJoined", 0) == 0) {
                gameLoop(); 
            }
        }
        else if (command == "stats") {
            std::stringstream ss;
            ss << "stats:" << login;
            send_message(main_socket, ss.str());
            std::string resp = receive_message(main_socket);
            std::cout << "[Client] Статистика: " << resp << std::endl;
        }
        else if (command == "exit") {
            std::cout << "[Client] Завершение работы.\n";
            return 0;
        }
        else {
            std::cout << "[Client] Неизвестная команда.\n";
        }
    }

    return 0;
}
