#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include "player.h"

/*
 * Класс, управляющий логикой Морского боя между двумя игроками
 */
class Game {
public:
    Player player1;
    Player player2;

    void play(zmq::socket_t& player1_socket,
              zmq::socket_t& player2_socket,
              pid_t first_player_pid,
              pid_t second_player_pid)
    {
        std::cout << "[Server] Игра началась между (PID "
                  << first_player_pid << ") и (PID "
                  << second_player_pid << ")!\n";

        player1.num = 1;
        player2.num = 2;

        player1.placeShips(player1_socket, first_player_pid, second_player_pid);
        player2.placeShips(player2_socket, first_player_pid, second_player_pid);

        int turn = 0;
        while (!gameOver()) {
            bool alive = try_recv(first_player_pid, second_player_pid);
            if (!alive) {
                std::cout << "[Server] Игра прервана (один из процессов умер)\n";
                kill(first_player_pid, SIGTERM);
                kill(second_player_pid, SIGTERM);
                exit(0);
            }

            if (turn % 2 == 0) {
                send_message(player1_socket, "your_turn");
                receive_message(player1_socket);

                send_message(player2_socket, "not_your_turn");
                receive_message(player2_socket);

                if (playerTurn(player1, player2, player1_socket, player2_socket)) {
                    if (gameOver()) {
                        send_message(player1_socket, "win");
                        receive_message(player1_socket);
                        send_message(player2_socket, "lose");
                        receive_message(player2_socket);
                        break;
                    }
                    continue;
                } else {
                    turn++;
                }
            } else {
                send_message(player2_socket, "your_turn");
                receive_message(player2_socket);

                send_message(player1_socket, "not_your_turn");
                receive_message(player1_socket);

                if (playerTurn(player2, player1, player2_socket, player1_socket)) {
                    if (gameOver()) {
                        // Победил 2
                        send_message(player2_socket, "win");
                        receive_message(player2_socket);
                        send_message(player1_socket, "lose");
                        receive_message(player1_socket);
                        break;
                    }
                    continue;
                } else {
                    turn++;
                }
            }
        }
        std::cout << "[Server] Игра завершена!\n";
    }

private:

    bool gameOver() const {
        return allShipsDead(player1) || allShipsDead(player2);
    }

    bool allShipsDead(const Player& pl) const {
        for (auto& row : pl.board) {
            for (auto c : row) {
                if (c == 'O') {
                    return false;
                }
            }
        }
        return true;
    }


    bool playerTurn(Player& attacker, Player& defender,
                    zmq::socket_t& attacker_socket,
                    zmq::socket_t& defender_socket)
    {
        send_message(attacker_socket, "shoot");
        std::string recv = receive_message(attacker_socket);

        std::stringstream ss(recv);
        std::string cmd, sx, sy;
        std::getline(ss, cmd, ':'); // coords
        std::getline(ss, sx, ':');
        std::getline(ss, sy, ':');
        if (sx.empty() || sy.empty()) {
            // Неверный формат
            send_message(attacker_socket, "miss");
            receive_message(attacker_socket);
            send_message(defender_socket, "miss");
            receive_message(defender_socket);
            return false;
        }

        int x = std::stoi(sx);
        int y = std::stoi(sy);

        // Проверка
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
            // Мимо
            send_message(attacker_socket, "miss");
            receive_message(attacker_socket);
            send_message(defender_socket, "miss");
            receive_message(defender_socket);
            return false;
        }

        // Попал?
        if (defender.board[x][y] == 'O') {
            defender.board[x][y] = 'X';
            send_message(attacker_socket, "shooted");
            receive_message(attacker_socket);
            send_message(defender_socket, "shooted");
            receive_message(defender_socket);
            return true; 
        } else {
            // Промах
            if (defender.board[x][y] == ' ') {
                defender.board[x][y] = '*';
            }
            send_message(attacker_socket, "miss");
            receive_message(attacker_socket);

            send_message(defender_socket, "miss");
            receive_message(defender_socket);

            // Покажем поле
            send_message(attacker_socket, "board" + defender.getClearBoard());
            receive_message(attacker_socket);

            send_message(defender_socket, "board" + defender.getBoard());
            receive_message(defender_socket);

            return false;
        }
    }
};
