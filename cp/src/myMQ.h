#pragma once

#include <zmq.hpp>
#include <iostream>
#include <signal.h>
#include <string>

extern int PORT_ITER;


inline std::string GetConPort(int port) {
    return "tcp://127.0.0.1:" + std::to_string(port);
}


inline bool send_message(zmq::socket_t& socket, const std::string& message_string) {
    zmq::message_t message(message_string.size());
    memcpy(message.data(), message_string.c_str(), message_string.size());
    return bool(socket.send(message, zmq::send_flags::none));
}


inline std::string receive_message(zmq::socket_t& socket) {
    zmq::message_t message;
    bool ok = false;
    try {
        ok = bool(socket.recv(message, zmq::recv_flags::none));
    } catch(...) {
        ok = false;
    }
    if (!ok) {
        return "";
    }
    return std::string(static_cast<char*>(message.data()), message.size());
}

inline bool try_recv(pid_t first_player_pid, pid_t second_player_pid) {
    if (kill(first_player_pid, 0) != 0 || kill(second_player_pid, 0) != 0) {
        return false;
    }
    return true;
}
