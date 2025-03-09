
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <csignal>
#include <unistd.h>
#include <sstream>

#include <zmq.hpp>

#include "myMQ.h"
#include "game.h"
#include "player.h"

int PORT_ITER = 0;

struct GameRoom {
    std::string name;
    pid_t playersPid[2];
    zmq::socket_t* playersSock[2];
    int filled = 0;
};

int main() {
    zmq::context_t context(3);

    zmq::socket_t main_socket(context, ZMQ_REP);
    main_socket.bind("tcp://*:5555");

    const int MAX_SOCKETS = 5;
    zmq::socket_t sockets[MAX_SOCKETS] = {
        zmq::socket_t(context, ZMQ_REQ),
        zmq::socket_t(context, ZMQ_REQ),
        zmq::socket_t(context, ZMQ_REQ),
        zmq::socket_t(context, ZMQ_REQ),
        zmq::socket_t(context, ZMQ_REQ),
    };
    for (int i = 0; i < MAX_SOCKETS; i++) {
        std::string bindStr = "tcp://*:" + std::to_string(5556 + i);
        sockets[i].bind(bindStr);
    }

    std::map<std::string, pid_t> loginMap;                      // login -> pid
    std::map<pid_t, int> pidToSocketIndex;                      // pid -> socket index
    std::unordered_map<std::string, std::pair<int,int>> stats;  // login -> (wins, loses)
    std::map<std::string, GameRoom> rooms;                      // roomName -> GameRoom

    std::cout << "Сервер запущен.\n";

    while (true) {
        std::string request = receive_message(main_socket);
        if (request.empty()) {
            send_message(main_socket, "Error:EmptyRequest");
            continue;
        }
        std::cout << "[SERVER] Получено сообщение: " << request << std::endl;

        std::stringstream ss(request);
        std::string cmd;
        std::getline(ss, cmd, ':');

        if (cmd == "login") {
            // "login:<login>:<pid>"
            std::string login;
            std::getline(ss, login, ':');
            std::string pidStr;
            std::getline(ss, pidStr, ':');
            pid_t p = (pid_t)std::stoi(pidStr);

            if (loginMap.find(login) != loginMap.end()) {
                send_message(main_socket, "Error:NameAlreadyExist");
                continue;
            }
            if (PORT_ITER >= MAX_SOCKETS) {
                send_message(main_socket, "Error:NoFreeSockets");
                continue;
            }

            loginMap[login] = p;
            pidToSocketIndex[p] = PORT_ITER;
            if (stats.find(login) == stats.end()) {
                stats[login] = {0, 0}; // wins=0, loses=0
            }

            std::string resp = "Ok:" + std::to_string(PORT_ITER);
            send_message(main_socket, resp);

            PORT_ITER++;
        }
        else if (cmd == "create") {
            // "create:<login>:<roomName>"
            std::string login, roomName;
            std::getline(ss, login, ':');
            std::getline(ss, roomName, ':');
            if (loginMap.find(login) == loginMap.end()) {
                send_message(main_socket, "Error:NeedLoginFirst");
                continue;
            }
            if (rooms.find(roomName) != rooms.end()) {
                send_message(main_socket, "Error:RoomAlreadyExist");
                continue;
            }

            GameRoom gr;
            gr.name = roomName;
            gr.filled = 1;
            gr.playersPid[0] = loginMap[login];
            gr.playersPid[1] = 0;
            gr.playersSock[0] = &sockets[ pidToSocketIndex[ loginMap[login] ] ];
            gr.playersSock[1] = nullptr;

            rooms[roomName] = gr;

            send_message(main_socket, "Ok:RoomCreated");
        }
        else if (cmd == "join") {
            // "join:<login>:<roomName>"
            std::string login, roomName;
            std::getline(ss, login, ':');
            std::getline(ss, roomName, ':');
            if (loginMap.find(login) == loginMap.end()) {
                send_message(main_socket, "Error:NeedLoginFirst");
                continue;
            }
            auto it = rooms.find(roomName);
            if (it == rooms.end()) {
                send_message(main_socket, "Error:RoomNotExist");
                continue;
            }
            GameRoom &gr = it->second;
            if (gr.filled >= 2) {
                send_message(main_socket, "Error:RoomIsFull");
                continue;
            }

            gr.playersPid[1] = loginMap[login];
            gr.playersSock[1] = &sockets[ pidToSocketIndex[ loginMap[login] ] ];
            gr.filled++;

            send_message(main_socket, "Ok:RoomJoined");

            if (gr.filled == 2) {
                pid_t pid1 = gr.playersPid[0];
                pid_t pid2 = gr.playersPid[1];
                zmq::socket_t &sock1 = *(gr.playersSock[0]);
                zmq::socket_t &sock2 = *(gr.playersSock[1]);

                std::cout << "[SERVER] Начинаем игру в комнате " << roomName << std::endl;
                Game game;
                game.play(sock1, sock2, pid1, pid2);

                // По окончании игры — определим, кто проиграл
                bool p1lost = false;
                bool p2lost = false;

                {
                    bool p1Dead = true;
                    for (auto &row : game.player1.board) {
                        for (auto c : row) {
                            if (c == 'O') {
                                p1Dead = false;
                                break;
                            }
                        }
                        if (!p1Dead) break;
                    }
                    p1lost = p1Dead;
                }
                {
                    bool p2Dead = true;
                    for (auto &row : game.player2.board) {
                        for (auto c : row) {
                            if (c == 'O') {
                                p2Dead = false;
                                break;
                            }
                        }
                        if (!p2Dead) break;
                    }
                    p2lost = p2Dead;
                }

                std::string login1, login2;
                for (auto &kv : loginMap) {
                    if (kv.second == pid1) login1 = kv.first;
                    if (kv.second == pid2) login2 = kv.first;
                }
                if (!login1.empty() && !login2.empty()) {
                    if (p1lost && !p2lost) {
                        stats[login1].second += 1; 
                        stats[login2].first += 1;
                    }
                    if (p2lost && !p1lost) {
                        stats[login2].second += 1; 
                        stats[login1].first += 1;
                    }
                }

                rooms.erase(roomName);
            }
        }
        else if (cmd == "stats") {
            // "stats:<login>"
            std::string login;
            std::getline(ss, login, ':');
            auto it = stats.find(login);
            if (it == stats.end()) {
                send_message(main_socket, "Error:NoStats");
            } else {
                auto [w, l] = it->second;
                std::stringstream out;
                out << "Stats: wins=" << w << ", loses=" << l;
                send_message(main_socket, out.str());
            }
        }
        else {
            send_message(main_socket, "Error:UnknownCommand");
        }
    }

    return 0;
}
