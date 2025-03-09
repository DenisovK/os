#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <csignal>
#include <cmath>

#include "myMQ.h"

// Размер игрового поля
const int BOARD_SIZE = 10;

/*
 * Класс, описывающий игрока и его поле
 */
class Player {
public:
    std::vector<std::vector<char>> board;
    int num;

    Player() {
        board = std::vector<std::vector<char>>(BOARD_SIZE, std::vector<char>(BOARD_SIZE, ' '));
        num = 0;
    }

    void placeShips(zmq::socket_t& player_socket, pid_t first_player_pid, pid_t second_player_pid) {
        bool alive;
        std::vector<int> shipSizes = {4,3,3,2,2,2,1,1,1,1};

        for (int size : shipSizes) {
            while (true) {
                std::string orientationMsg = "Введите ориентацию (V/H) для корабля на " + std::to_string(size) + " палуб(ы)";
                send_message(player_socket, orientationMsg);
                std::string orientation = receive_message(player_socket);
                {
                    // Удалим лишние пробелы
                    std::stringstream tmpSS(orientation);
                    tmpSS >> orientation;
                }

                std::string placeMsg = "Разместите " + std::to_string(size) + "-палубный корабль (укажите начальные координаты x y)";
                send_message(player_socket, placeMsg);
                std::string resp = receive_message(player_socket);

                alive = try_recv(first_player_pid, second_player_pid);
                if (!alive) {
                    std::cout << "[Server] Игра прервана из-за смерти процесса\n";
                    kill(first_player_pid, SIGTERM);
                    kill(second_player_pid, SIGTERM);
                    exit(0);
                }

                int startX = -1, startY = -1;
                {
                    std::stringstream ss(resp);
                    std::string tmp;
                    std::getline(ss, tmp, ':'); // coords
                    std::getline(ss, tmp, ':'); // x
                    startX = std::stoi(tmp);
                    std::getline(ss, tmp, ':'); // y
                    startY = std::stoi(tmp);
                }

                // Проверим корректность ориентации
                if (orientation != "V" && orientation != "H") {
                    send_message(player_socket, "Error: Неверная ориентация (V/H)");
                    receive_message(player_socket);
                    continue;
                }

                // Проверим, можно ли разместить корабль
                if (checkShipPlacementOk(startX, startY, orientation, size)) {
                    markShip(startX, startY, orientation, size);
                    send_message(player_socket, "board" + getBoard());
                    receive_message(player_socket);
                    break;
                } else {
                    send_message(player_socket, "Error: Неверное расположение корабля. Повторите ввод");
                    receive_message(player_socket);
                }
            }
        }
    }

    /*
     * Возвращает поле игрока в виде строки (с кораблями)
     */
    std::string getBoard() const {
        std::stringstream ss;
        ss << "\n  0 1 2 3 4 5 6 7 8 9\n";
        for (int i = 0; i < BOARD_SIZE; ++i) {
            ss << i << " ";
            for (int j = 0; j < BOARD_SIZE; ++j) {
                ss << board[i][j] << " ";
            }
            ss << "\n";
        }
        ss << "\n";
        return ss.str();
    }

    std::string getClearBoard() const {
        std::stringstream ss;
        ss << "\n  0 1 2 3 4 5 6 7 8 9\n";
        for (int i = 0; i < BOARD_SIZE; ++i) {
            ss << i << " ";
            for (int j = 0; j < BOARD_SIZE; ++j) {
                if (board[i][j] == 'O') {
                    ss << "  ";
                } else {
                    ss << board[i][j] << " ";
                }
            }
            ss << "\n";
        }
        ss << "\n";
        return ss.str();
    }

private:
    bool checkShipPlacementOk(int startX, int startY, const std::string& orientation, int size) const {
        std::vector<std::pair<int,int>> cells;
        cells.reserve(size);

        for (int k = 0; k < size; k++) {
            int xx = startX;
            int yy = startY;
            if (orientation == "V") {
                xx += k;
            } else {
                yy += k;
            }
            if (xx < 0 || xx >= BOARD_SIZE || yy < 0 || yy >= BOARD_SIZE) {
                return false;
            }
            cells.emplace_back(xx, yy);
        }

        for (auto &c : cells) {
            int x = c.first;
            int y = c.second;

            for (int i = x-1; i <= x+1; i++) {
                for (int j = y-1; j <= y+1; j++) {
                    if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
                        if (board[i][j] == 'O') {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    void markShip(int startX, int startY, const std::string& orientation, int size) {
        for (int k = 0; k < size; k++) {
            if (orientation == "V") {
                board[startX + k][startY] = 'O';
            } else {
                board[startX][startY + k] = 'O';
            }
        }
    }
};
