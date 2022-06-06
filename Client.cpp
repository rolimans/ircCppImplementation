#include "Client.hpp"
#include "Socket.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>

Client::Client(std::string address) {
    this->address = address;
    this->socket = new Socket(AF_INET, SOCK_STREAM, 0);
}
int Client::start() {
    socket->connect(address, DEFAULT_PORT);
    std::cout << "Client connected on " << address << ":" << DEFAULT_PORT
              << std::endl;
    return 0;
}

std::string Client::readMessage() {
    std::string message;
    this->socket->socketRead(message, MAX_MSG_SIZE + 1);
    return message;
}

void Client::sendMessage(std::string message) {
    while (message.length() > MAX_MSG_SIZE) {
        std::string subMessage = message.substr(0, MAX_MSG_SIZE);
        message = message.substr(MAX_MSG_SIZE, message.length());
        this->socket->socketWrite(subMessage);
    }
    this->socket->socketWrite(message);
}

int Client::stop() {
    this->socket->close();
    return 0;
}
